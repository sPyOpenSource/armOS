/*
 *  CyMCUElfTool, an ELF file post-processing utility
 *  Copyright (C) 2016-2017 - Cypress Semiconductor Corp.
 * 
 *  This file is part of CyMCUElfTool
 *
 *  CyMCUElfTool is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CyMCUElfTool is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public License
 *  along with CyMCUElfTool; if not, write to the Free Software Foundation, 
 *  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *  Contact
 *  Email: customercare@cypress.com
 *  USPS:  Cypress Semiconductor
 *         198 Champion Court,
 *         San Jose CA 95134 USA
 */

#include "stdafx.h"
#include "elf2hex.h"
#include "elf/cyelfutil.h"
#include "elf/elfxx.h"
#include "hex/cypsochexfile.h"
#include "cymcuelfutil.h"
#include "utils.h"
#include <algorithm>
#include <fstream>
#include <vector>
#include <cassert>
#include <limits>

using namespace cyelflib;
using namespace cyelflib::elf;

using std::copy;
using std::numeric_limits;
using std::string;
using std::wstring;
using std::tuple;
using std::vector;
using std::fill_n;
using cyelflib::hex::CyHexFile;
using cyelflib::hex::CyHexFileLine;
using cyelflib::hex::CyPsocHexFile;
using cyelflib::hex::CyPSoCACD2File;
using cyelflib::hex::CyACD2FileLine;
using cyelflib::elf::CyElfFile;

namespace cymcuelftool {

    static CyErr FinishFile(CyHexFile &hex, const std::wstring &hexFile)
    {
        file_delete(hexFile.c_str());
#ifdef _MSC_VER
        std::ofstream hexStrm(hexFile.c_str());
#else
        std::ofstream hexStrm(wstr_to_str(hexFile).c_str());
#endif
        if (hexStrm.fail())
            return CyErr(CyErr::FAIL, L"Unable to open %ls for writing.", hexFile.c_str());
        CyErr err = hex.Write(hexStrm);
        hexStrm.close();
        if (err.IsNotOK())
            file_delete(hexFile.c_str());
        return err;
    }

    CyErr GetAndAppendData(const CyElfFile &elf, CyHexFile *hex, uint32_t startAddress, uint32_t length, uint8_t fill)
    {
        vector<uint8_t> data(length, fill);
        CyErr err = GetMemoryData(elf, data, startAddress, startAddress + length - 1);
        if (err.IsOK())
            hex->AppendData(startAddress, &data[0], data.size());
        return err;
    }

    CyErr elf2hex(const CyElfFile &elf,
        const std::wstring &hexFile,
        HexFormatType format,
        uint8_t fill)
    {
        CyErr err;
        CyHexFile *hex = new CyHexFile();

        // absolute start, absolute length, row_size
        vector<tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> memories = GetMemories(elf);

        //TODO: Remove bullshit hack, PDL STILL (CDT 252308) does not have necessary symbols defined
        if (0 == memories.size())
            memories.push_back(std::make_tuple(FLASH_OFFSET, FLASH_SIZE, FLASH_ROW_SIZE));

        // first allocated address, region length
        vector<vector<tuple<uint32_t, uint32_t>>> usedRegions = GetUsedRegions(elf, memories);
        usedRegions = AlignRegionsToFullRows(usedRegions, memories);

        // Get flash data
        vector<vector<uint8_t>> allData;
        for (uint32_t i = 0; i < usedRegions.size(); i++)
        {
            uint32_t minAddress, length;

            if ((format & HexFormatType::PAD) == HexFormatType::PAD)
            {
                minAddress = (uint32_t)(std::get<0>(memories[i]));
                length = (uint32_t)std::get<1>(memories[i]);

                GetAndAppendData(elf, hex, minAddress, length, fill);
            }
            else
            {
                vector<tuple<uint32_t, uint32_t>> usedRegionsForMemory = usedRegions[i];
                for (uint32_t j = 0; j < usedRegionsForMemory.size(); j++)
                {
                    minAddress = std::get<0>(usedRegionsForMemory[j]);
                    length = std::get<1>(usedRegionsForMemory[j]);
                    GetAndAppendData(elf, hex, minAddress, length, fill);
                }
            }

            if ((format & HexFormatType::SPLIT) == HexFormatType::SPLIT)
            {
                wstring newFileName = ReplaceExtension(hexFile, std::to_wstring(i) + L".hex");

                err = FinishFile(*hex, newFileName);
                if (err.IsNotOK())
                    return err;

                delete hex;
                hex = new CyHexFile();
            }
        }

        // TODO: Are there other sections that must be included like SMIF?
        if ((err = CopySectionToHex(elf, hex, CHECKSUM_SECTION)).IsNotOK())
            return err;
        if ((err = CopySectionToHex(elf, hex, FLASHPROT_SECTION)).IsNotOK())
            return err;
        if ((err = CopySectionToHex(elf, hex, META_SECTION)).IsNotOK())
            return err;
        if ((err = CopySectionToHex(elf, hex, CHIPPROT_SECTION, 1)).IsNotOK())
            return err;

        if ((format & HexFormatType::SPLIT) != HexFormatType::SPLIT)
            err = FinishFile(*hex, hexFile);

        delete hex;
        return err;
    }

    CyErr elf2acd(
        const CyElfFile &elf,
        uint8_t fill,
        const wstring &initVec,
        CyPSoCACD2File* &acdFile)
    {
        CyErr err;

        // absolute start, absolute length, row_size
        vector<tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> memories = GetMemories(elf);

        if (memories.size() == 0){
            return CyErr(CyErr::FAIL, L"ERROR: no symbols __cy_memory_N_start, __cy_memory_N_length, or __cy_memory_N_row_size found. Cannot generate acd2 file");
        }

        // allocated start, allocated end
        vector<tuple<uint32_t, uint32_t, uint32_t>> bounds = GetBounds(elf, memories);

        GElf_Sym symbolBtldrMetaStart, symbolBtldrMetaLength;
        bool hasBtldrSymbols = elf.GetSymbol(SYM_NAME_BTLDR_META_START, &symbolBtldrMetaStart);
        hasBtldrSymbols &= elf.GetSymbol(SYM_NAME_BTLDR_META_LEN, &symbolBtldrMetaLength);

        // Get app info metadata
        GElf_Sym symbolAppVerifyStart, symbolAppVerifyLength;
        bool hasAppVerifySymbols = elf.GetSymbol(SYM_NAME_APP_VERIFY_START, &symbolAppVerifyStart);
        hasAppVerifySymbols &= elf.GetSymbol(SYM_NAME_APP_VERIFY_LEN, &symbolAppVerifyLength);
        std::stringstream appInfo("");
        if (hasAppVerifySymbols){
            appInfo << "APPINFO:0x" << std::hex << symbolAppVerifyStart.st_value
                    << ",0x" << symbolAppVerifyLength.st_value << std::dec;
        }

        // Get bootloadable flash data
        vector<std::unique_ptr<CyACD2FileLine>> lines;
        for (uint32_t j = 0; j < bounds.size(); j++)
        {
            uint32_t minAddress = std::get<0>(bounds[j]);
            uint32_t maxAddress = std::get<1>(bounds[j]);
            uint32_t rowSize = std::get<2>(bounds[j]);

            uint32_t rows = (maxAddress - minAddress + rowSize - 1) / rowSize;
            vector<uint8_t> allData(rows * rowSize, fill);
            err = GetMemoryData(elf, allData, minAddress, maxAddress);
            if (err.IsNotOK()){
                return err;
            }

            for (uint32_t i = 0; i < allData.size(); i += rowSize)
            {
                vector<uint8_t> rowData(rowSize);
                memcpy(&rowData[0], &allData[i], rowSize);

                if (!hasBtldrSymbols || i != (uint32_t)symbolBtldrMetaStart.st_value)
                    lines.push_back(std::unique_ptr<CyACD2FileLine>(new CyACD2FileLine(minAddress + i, rowData)));
            }
        }

        // Get header information
        GElf_Sym symbolAppId, symbolProductId;
        bool valid = elf.GetSymbol(SYM_NAME_APP_ID, &symbolAppId);
        valid &= elf.GetSymbol(SYM_NAME_PRODUCT_ID, &symbolProductId);

        if (!valid){
            return CyErr(CyErr::FAIL, L"ERROR: could not find app/product symbols");
        }

        // cyacd2 format is documented in DESP-36
        //static const uint8_t CYACD_VERSION = 1; // not currently used, commented to suppress warning
        uint32_t devId;
        uint8_t devRev;
        uint8_t checksumType = 0; // default
        uint8_t appId;
        uint32_t productId;

        // Get checksum type
        GElf_Sym symbolChecksumType;
        if (elf.GetSymbol(SYM_NAME_CHECKSUM_TYPE, &symbolChecksumType)){
            checksumType = (uint8_t)symbolChecksumType.st_value;
        }

        Elf_Scn *metaScn = GetSectionEx(&elf, META_SECTION);
        Elf_Data *metaData = metaScn ? elf.GetFirstData(metaScn) : nullptr;
        if (metaData == nullptr || metaData->d_size < CyPsocHexFile::CyPSoCHexMetaData::META_SIZE || metaData->d_buf == nullptr){
            return CyErr(CyErr::FAIL, L"ERROR: %ls: section %ls not found", elf.Path().c_str(), str_to_wstr(META_SECTION).c_str());
        }
        uint8_t *metaDataBuf = reinterpret_cast<uint8_t *>(metaData->d_buf);
        devId =
            metaDataBuf[CyPsocHexFile::CyPSoCHexMetaData::SILICONID_OFFSET] |
            metaDataBuf[CyPsocHexFile::CyPSoCHexMetaData::SILICONID_OFFSET + 1] << 8 |
            metaDataBuf[CyPsocHexFile::CyPSoCHexMetaData::SILICONID_OFFSET + 2] << 16 |
            metaDataBuf[CyPsocHexFile::CyPSoCHexMetaData::SILICONID_OFFSET + 3] << 24;
        memcpy(&devRev, metaDataBuf + CyPsocHexFile::CyPSoCHexMetaData::SILICONREV_OFFSET, 1);
        appId = (uint8_t)symbolAppId.st_value;
        productId = (uint32_t)symbolProductId.st_value;

        acdFile = new CyPSoCACD2File(
            CyPSoCACD2File::VERSION, devId, devRev, checksumType, appId, productId, 
            std::move(lines), initVec, str_to_wstr(appInfo.str()));

        return err;
    }

} // namespace cymcuelftool
