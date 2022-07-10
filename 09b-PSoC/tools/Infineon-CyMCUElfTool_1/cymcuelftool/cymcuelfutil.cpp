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
#include "elf/cyelfutil.h"
#include "cymcuelfutil.h"
#include "elf/elfxx.h"
#include "elf2hex.h"
#include "hex/cypsocacd2file.h"
#include <iostream>

using namespace cyelflib;
using namespace cyelflib::elf;
using std::string;
using std::tuple;
using std::vector;
using std::wstring;

namespace cymcuelftool {

const char* SEC_NAME_TOC_PART2 = ".cy_toc_part2"; // cdt 276018 - duplicate TOC section for secure boot verification in Si *A
const char* SEC_NAME_RTOC_PART2 = ".cy_rtoc_part2"; // cdt 276018
const char* SEC_NAME_EFUSE = ".cy_efuse";

const char* SEC_NAME_APP_HEADER = ".cy_app_header";

const char* SEC_NAME_APP_SIGNATURE = ".cy_app_signature";
const char* SYM_NAME_APP_VERIFY_START = "__cy_app_verify_start";
const char* SYM_NAME_APP_VERIFY_LEN = "__cy_app_verify_length";

const char* SEC_NAME_BTLDR_META = ".cy_boot_metadata";
const char* SYM_NAME_BTLDR_META_START = "__cy_boot_metadata_addr";
const char* SYM_NAME_BTLDR_META_LEN = "__cy_boot_metadata_length";
const char* SYM_NAME_APP_ID = "__cy_app_id";
const char* SYM_NAME_PRODUCT_ID = "__cy_product_id";
const char* SYM_NAME_CHECKSUM_TYPE = "__cy_checksum_type";

string SymNameMemStart(int idx) { return "__cy_memory_" + std::to_string(idx) + "_start"; }
string SymNameMemLength(int idx) { return "__cy_memory_" + std::to_string(idx) + "_length"; }
string SymNameMemRowSize(int idx) { return "__cy_memory_" + std::to_string(idx) + "_row_size"; }

vector<tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> GetMemories(const CyElfFile &elf)
{
    // absolute start, absolute length, row_size
    vector<tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> memories;
    bool valid;
    do
    {
        int idx = memories.size();

        string nameStart = SymNameMemStart(idx);
        string nameLength = SymNameMemLength(idx);
        string nameRowSize = SymNameMemRowSize(idx);

        GElf_Sym symbolStart, symbolLength, symbolRowSize;
        valid =
            elf.GetSymbol(nameStart.c_str(), &symbolStart) &&
            elf.GetSymbol(nameLength.c_str(), &symbolLength) &&
            elf.GetSymbol(nameRowSize.c_str(), &symbolRowSize);
        if (valid)
            memories.push_back(std::make_tuple(symbolStart.st_value, symbolLength.st_value, symbolRowSize.st_value));
    } while (valid);

    return memories;
}

// returns actual used bounds of memories
// <minAddress, maxAddress, memory row size>
vector<tuple<uint32_t, uint32_t, uint32_t>> GetBounds(
    const CyElfFile &elf, const vector<tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> &memories)
{
    // allocated start, allocated end
    vector<tuple<uint32_t, uint32_t, uint32_t>> bounds;
    for (uint32_t m = 0; m < memories.size(); m++)
    {
        uint32_t minAddress = 0xFFFFFFFFu, maxAddress = 0;
        Elf64_Addr start = std::get<0>(memories[m]);
        Elf64_Addr end = start + std::get<1>(memories[m]) - 1;

        // Find the minimum and maximum addresses
        for (int i = 0; i < elf.PhdrCount(); i++)
        {
            const GElf_Phdr &phdr = elf.GetPhdr(i);

            if ((phdr.p_type == PT_LOAD || phdr.p_type == PT_ARM_EXIDX) &&
                phdr.p_paddr + phdr.p_filesz > start && phdr.p_paddr <= end)
            {
                uint32_t endAddr = uint32_t(phdr.p_paddr + phdr.p_filesz - 1);
                if (endAddr > maxAddress)
                    maxAddress = endAddr;
                if (phdr.p_paddr < minAddress)
                    minAddress = uint32_t(phdr.p_paddr);
            }
        }

        if (minAddress > maxAddress)
            minAddress = maxAddress = 0;
        else
            bounds.push_back(std::make_tuple(minAddress, maxAddress, (uint32_t)std::get<2>(memories[m])));
    }
    return bounds;
}

vector<vector<tuple<uint32_t, uint32_t>>> GetUsedRegions(
    const CyElfFile &elf, const vector<tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> &memories)
{
    // first allocated address, region length
    vector<vector<tuple<uint32_t, uint32_t>>> regionsAll;
    for (uint32_t m = 0; m < memories.size(); m++)
    {
        vector<tuple<uint32_t, uint32_t>> regionsByMemory;
        Elf64_Addr start = std::get<0>(memories[m]);
        Elf64_Addr end = start + std::get<1>(memories[m]) - 1;

        // Find the minimum and maximum addresses
        for (int i = 0; i < elf.PhdrCount(); i++)
        {
            const GElf_Phdr &phdr = elf.GetPhdr(i);

            if ((phdr.p_type == PT_LOAD || phdr.p_type == PT_ARM_EXIDX) &&
                phdr.p_paddr + phdr.p_filesz > start && phdr.p_paddr <= end)
            {
                regionsByMemory.push_back(std::make_tuple(uint32_t(phdr.p_paddr), uint32_t(phdr.p_filesz)));
            }
        }
        if (regionsByMemory.size() > 0)
            regionsAll.push_back(regionsByMemory);
    }
    return regionsAll;
}

vector<vector<tuple<uint32_t, uint32_t>>> AlignRegionsToFullRows(
    const vector<vector<tuple<uint32_t, uint32_t>>> &regions, const vector<tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> &memories)
{
    // region = <startAddr, length>
    vector<vector<tuple<uint32_t, uint32_t>>> consolidatedRegionsAll;
    for (uint32_t i = 0; i < regions.size(); i++)
    {
        uint32_t minAddress, maxAddress;
        uint32_t rowSize = (uint32_t)std::get<2>(memories[i]);
        vector<tuple<uint32_t, uint32_t>> consolidatedRegionsForMemory;
        vector<tuple<uint32_t, uint32_t>> usedRegionsForMemory = regions[i];

        std::stable_sort(usedRegionsForMemory.begin(), usedRegionsForMemory.end(),
            [&](tuple<uint32_t, uint32_t> a, tuple<uint32_t, uint32_t> b) { return std::get<0>(a) < std::get<0>(b); });

        minAddress = 0xFFFFFFFF;
        maxAddress = 0;
        for (uint32_t j = 0; j < usedRegionsForMemory.size(); j++)
        {
            uint32_t localMinAddress = RoundToMultiple(std::get<0>(usedRegionsForMemory[j]), rowSize, false);
            uint32_t regionEndAddr = std::get<0>(usedRegionsForMemory[j]) + std::get<1>(usedRegionsForMemory[j]) - 1;
            uint32_t localMaxAddress = RoundToMultiple(regionEndAddr, rowSize, true);

            if (localMinAddress < minAddress)
                minAddress = localMinAddress;
            if (localMaxAddress > maxAddress)
                maxAddress = localMaxAddress;

            if (j + 1 >= usedRegionsForMemory.size() || localMaxAddress < std::get<0>(usedRegionsForMemory[j + 1]))
            {
                consolidatedRegionsForMemory.push_back(std::make_tuple(minAddress, maxAddress - minAddress));
                minAddress = 0xFFFFFFFF;
                maxAddress = 0;
            }
        }
        consolidatedRegionsAll.push_back(consolidatedRegionsForMemory);
    }
    return consolidatedRegionsAll;
}

CyErr GetMemoryData(const CyElfFile &elf, vector<uint8_t> &allData, uint32_t minAddress, uint32_t maxAddress, uint32_t vecOffset /*= 0*/)
{
    CyErr err = CyErr();
    for (int i = 0; i < elf.PhdrCount(); i++)
    {
        const GElf_Phdr &phdr = elf.GetPhdr(i);

        if ((phdr.p_type == PT_LOAD || phdr.p_type == PT_ARM_EXIDX) && phdr.p_filesz != 0)
        {
            if (phdr.p_paddr + phdr.p_filesz > minAddress && phdr.p_paddr < maxAddress)
            {
                uint32_t pBegin = std::max(minAddress, (uint32_t)phdr.p_paddr);
                uint32_t pEnd = std::min(maxAddress, (uint32_t)(phdr.p_paddr + phdr.p_filesz - 1));

                uint32_t pOffset = (uint32_t)(pBegin - phdr.p_paddr);
                uint32_t pLen = (pEnd - pBegin + 1);

                err = elf.ReadPhdrData(&phdr, pOffset, pLen, &allData[vecOffset + pBegin - minAddress]);
                if (err.IsNotOK())
                    return err;
            }
        }
    }
    return err;
}

// This function pupulates the .cymeta and .cychecksum sections of an elf file, which are required by
// Cypress Programmer. Should run this last after modifying other sections.
CyErr PopulateSectionsCymetaCychecksum(const wstring &file, const uint8_t fillValue)
{
    using cyelflib::hex::CyPsocHexFile;

    CyElfFile elf(file);
    uint32_t appChecksum = 0;
    CyErr err;

    err = elf.Read(true);

    if (err.IsOK()){
        appChecksum = ComputeChecksumFromElf(elf, FLASH_OFFSET, FLASH_SIZE, fillValue);
   }

    if (err.IsOK()){
        err = StoreMetaChecksums(&elf, appChecksum);
    }

    if (err.IsOK()){
        err = AdjustHeaders(&elf, false, std::numeric_limits<unsigned int>::max(), true);
    }

    // Update the ELF file on disk
    if (err.IsOK()){
        err = elf.Write();
    }

    elf.Cleanup();  // Close file to ensure that modification time is correct wrt hex/acd files

    return err;
}

void GenerateHexFile(const wstring &elfFilePath, const wstring &hexFilePath, uint8_t fillValue)
{
    CyErr err;

    CyElfFile elf2(elfFilePath);   // Open for reading
    if ((err = elf2.Read()).IsNotOK())
        errx(EXIT_FAILURE, L"open of ELF file %ls failed", elf2.Path().c_str());

    err = elf2hex(elf2, hexFilePath, HexFormatType::NONE, fillValue);
    if (err.IsNotOK())
    {
        file_delete(hexFilePath.c_str());
        errx(EXIT_FAILURE, L"Failed to generate %ls: %ls", hexFilePath.c_str(), err.Message().c_str());
    }
}

CyErr StoreDataInSection(CyElfFile *elf, const std::string &sectionName, const vector<uint8_t> data)
{
    Elf_Scn *section = GetSectionEx(elf, sectionName);
    if (section == NULL)
        return CyErr(CyErr::FAIL, L"Section %ls of ELF file %ls does not exist", str_to_wstr(sectionName).c_str(), elf->Path().c_str());

    Elf_Data *sectionData = NULL;
    if ((sectionData = elf_getdata(section, sectionData)) == NULL)
        return CyErr(CyErr::FAIL, L"elf_getdata() failed : %s.", str_to_wstr(elf_errmsg(-1)).c_str());
    if (sectionData->d_size < data.size())
        return CyErr(CyErr::FAIL, L"%ls contains fewer than %u bytes", str_to_wstr(sectionName).c_str(), data.size());

    uint8_t *dataPtr = (uint8_t *)(sectionData->d_buf);
    if (NULL == dataPtr)
        return CyErr(CyErr::FAIL, L"%ls does not have a loadable section", str_to_wstr(sectionName).c_str()); // fix CDT 266001

    copy(data.begin(), data.end(), dataPtr);
    elf->DirtyData(sectionData);

    return CyErr();
}

// This function is used to find all application data and generate a checksum over it.
// Use SYM_NAME_APP_VERIFY_START and SYM_NAME_APP_VERIFY_LEN for app region
CyErr CalculateApplicationChecksumCRC32(CyElfFile *elf, uint32_t *crc)
{
    CyErr err;
    // absolute start, absolute length, row_size
    vector<tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> memories = GetMemories(*elf);
    // allocated start, allocated end
    vector<tuple<uint32_t, uint32_t, uint32_t>> bounds = GetBounds(*elf, memories);

    vector<uint8_t> allBytes;
    uint32_t totalSize = 0;
    for (uint32_t i = 0; i < bounds.size(); i++)
    {
        uint32_t minAddress = std::get<0>(bounds[i]);
        uint32_t maxAddress = std::get<1>(bounds[i]);
        totalSize += (maxAddress - minAddress + 1);
    }

    uint32_t offset = 0;
    vector<uint8_t> allData(totalSize, 0);
    for (uint32_t i = 0; i < bounds.size(); i++)
    {
        uint32_t minAddress = std::get<0>(bounds[i]);
        uint32_t maxAddress = std::get<1>(bounds[i]);

        err = GetMemoryData(*elf, allData, minAddress, maxAddress, offset);
        if (err.IsNotOK())
            return err;

        offset += (maxAddress - minAddress + 1);
    }

    *crc = Compute32bitCRC(allData.begin(), allData.end());

    return err;
}

CyErr PatchTableOfContents(CyElfFile *elf)
{
    vector<string> tocs = { SEC_NAME_TOC_PART2, SEC_NAME_RTOC_PART2 }; // cdt 276018
    for (const string &toc : tocs)
    {
        Elf_Scn *tocSection = GetSectionEx(elf, toc);
        if (tocSection != NULL)
        {
            Elf_Data *data = NULL;
            if ((data = elf_getdata(tocSection, data)) == NULL){
                return CyErr(CyErr::FAIL, L"elf_getdata() failed: %s.", str_to_wstr(elf_errmsg(-1)).c_str());
            }
            if (data->d_size < sizeof(uint8_t) + sizeof(uint32_t)){ // min 1 byte plus 4 bytes for checksum
                return CyErr(CyErr::FAIL, L"Failed to patch TOC: section %ls contains fewer than %u bytes", str_to_wstr(toc).c_str(), sizeof(uint32_t) + sizeof(uint8_t));
            }

            uint8_t *dataPtr = (uint8_t *)(data->d_buf);
            if (!dataPtr){
                return CyErr(CyErr::FAIL, L"Failed to compute checksum: Section %ls has space allocated, but no data. Check .ld file", str_to_wstr(toc).c_str());
            }

            //All TOC entries are 4 bytes, but TOC only stores 2 in the top 2 bytes, bottom 2 are 0's
            uint32_t crcOffset = data->d_size - sizeof(uint32_t);
            uint32_t crcAddress = data->d_size - sizeof(uint16_t);

            std::vector<uint8_t> buffer;
            for (uint32_t i = 0; i < crcOffset; i++)
                buffer.push_back(dataPtr[i]);
            uint16_t crc = ComputeCRC16CCITT(buffer);

            WriteWithEndian16(CyEndian::CYENDIAN_LITTLE, dataPtr + crcAddress, crc);
            std::wcout << L"Calculated and stored CRC-16-CCITT over section " << str_to_wstr(toc) << std::endl;

            elf_flagdata(data, ELF_C_SET, ELF_F_DIRTY);
        }
    }

    return CyErr();
}

} // namespace cymcuelftool
