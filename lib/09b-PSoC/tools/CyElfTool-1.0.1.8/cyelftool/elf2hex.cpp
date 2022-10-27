/*
 *  CyElfTool: a command line tool for ELF file post-processing
 *  Copyright (C) 2013-2016 - Cypress Semiconductor Corp.
 * 
 *  CyElfTool is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CyElfTool is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public License
 *  along with CyElfTool; if not, write to the Free Software Foundation, 
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
#include "cybootloaderutil.h"
#include "hex/cypsochexfile.h"
#include "hex/cypsocacdfile.h"
#include "utils.h"
#include <algorithm>
#include <fstream>
#include <vector>
#include <cassert>
#include <limits>

using namespace cyelflib::elf;

using std::copy;
using std::numeric_limits;
using std::string;
using std::wstring;
using std::vector;
using std::fill_n;
using cyelflib::hex::CyHexFile;
using cyelflib::hex::CyHexFileLine;
using cyelflib::hex::CyPsocHexFile;
using cyelflib::hex::CyPSoCACDFile;
using cyelflib::hex::CyACDFileLine;
using cyelflib::elf::CyElfFile;

namespace cyelftool {

static CyErr SetChecksumMetaDataSection(const CyElfFile &elfFile, CyHexFile *hexFile,
    const string &sectionName, uint32_t checkSum)
{
    Elf_Scn *scn = GetSectionEx(&elfFile, sectionName);
    if (scn)
    {
        GElf_Shdr shdr;
        CyErr err(elfFile.GetShdr(scn, &shdr));
        if (err.IsNotOK())
            return err;

        if (shdr.sh_size > 1048576)
            return CyErr(CyErr::FAIL,
#ifdef _MSC_VER
            L"Section %ls is too large (0x%I64X)",
#else
            L"Section %ls is too large (0x%llX)",
#endif
            str_to_wstr(sectionName).c_str(), (uint64_t)shdr.sh_size);

        uint32_t base = (uint32_t)shdr.sh_addr;
        uint32_t size = (uint32_t)shdr.sh_size;

        if (shdr.sh_addr < base)
            return CyErr(CyErr::FAIL, L"Section %ls cannot start before 0x%08X",
            str_to_wstr(sectionName).c_str(), base);

        vector<uint8_t> data((uint32_t)(shdr.sh_addr + size - base));
        if (sectionName == META_SECTION)
        {
            if ((err = elfFile.ReadData(scn, 0, size, &data[(uint32_t)(shdr.sh_addr - base)])).IsNotOK())
                return CyErr(CyErr::FAIL, L"Error reading section %ls: %ls",
                str_to_wstr(sectionName).c_str(), err.Message().c_str());

            SetChecksumInMetaDataSection(data.data(), checkSum);
        }
        else
        {
            data[0] = (uint8_t)(checkSum >> 8);
            data[1] = (uint8_t)checkSum;
        }

        hexFile->AppendData(base, &data[0], data.size());
    }

    return CyErr();
}

uint32_t RecalculateChecksum(CyElfFile &elf, vector<uint8_t> data)
{
    uint32_t recalculatedChecksum = 0;
    for (uint32_t j = 0; j < data.size(); j++)
        recalculatedChecksum += data[j];

    return AddEccSectionToChecksum(elf, recalculatedChecksum);
}

CyErr elf2hex(const std::wstring &elfFile,
    const std::wstring &hexFile,
    uint32_t flashSize,
    uint32_t flashOffset,
    uint8_t defaultPadding)
{
    CyElfFile elf(elfFile);
    CyErr err(elf.Read());
    if (err.IsNotOK())
        return err;

    return cyelftool::elf2hex(elf, hexFile, flashSize, flashOffset, defaultPadding);
}

CyErr elf2hex(CyElfFile &elf,
    const std::wstring &hexFile,
    uint32_t flashSize,
    uint32_t flashOffset,
    uint8_t defaultPadding)
{
    CyErr err;
    CyHexFile hex;

    const uint32_t FLASH_PER_ECC = 8;
    const uint32_t eccSize = flashSize / FLASH_PER_ECC;

    // Load data from phdrs, excluding Cypress metadata if it is present.
    vector<uint8_t> data(flashSize);
    std::fill_n(data.begin(), data.size(), defaultPadding);
    for (int i = 0; i < elf.PhdrCount(); i++)
    {
        const GElf_Phdr &phdr = elf.GetPhdr(i);

        if (phdr.p_type == PT_LOAD &&
            phdr.p_paddr >= (flashOffset) &&
            phdr.p_paddr < (flashOffset + data.size()) &&
            phdr.p_filesz != 0)
        {
            uint32_t start = (uint32_t)(phdr.p_paddr - flashOffset);
            err = elf.ReadPhdrData(&phdr,
                0,  // Offset
                std::min((uint32_t)phdr.p_filesz, (uint32_t)(data.size() - start)),    // Length
                &data[start]);
            if (err.IsNotOK())
                return err;
        }
    }

    uint32_t nonZeroPaddingChecksum = 0;
    if (defaultPadding != 0)
        nonZeroPaddingChecksum = RecalculateChecksum(elf, data);

    hex.AppendData(flashOffset, &data[0], data.size());

    if ((err = cyelflib::elf::CopySectionToHex(elf, &hex, CONFIGECC_SECTION, eccSize, CyPsocHexFile::ADDRESS_CONFIG)).IsNotOK())
        return err;
    if ((err = cyelflib::elf::CopySectionToHex(elf, &hex, CUSTNVL_SECTION)).IsNotOK())
        return err;
    if ((err = cyelflib::elf::CopySectionToHex(elf, &hex, WOLATCH_SECTION)).IsNotOK())
        return err;
    if ((err = cyelflib::elf::CopySectionToHex(elf, &hex, EEPROM_SECTION)).IsNotOK())
        return err;

    err = (defaultPadding == 0)
        ? cyelflib::elf::CopySectionToHex(elf, &hex, CHECKSUM_SECTION)
        : SetChecksumMetaDataSection(elf, &hex, CHECKSUM_SECTION, nonZeroPaddingChecksum);

    if (err.IsNotOK())
        return err;

    if ((err = cyelflib::elf::CopySectionToHex(elf, &hex, FLASHPROT_SECTION)).IsNotOK())
        return err;

    err = (defaultPadding == 0)
        ? cyelflib::elf::CopySectionToHex(elf, &hex, META_SECTION)
        : SetChecksumMetaDataSection(elf, &hex, META_SECTION, nonZeroPaddingChecksum);

    if (err.IsNotOK())
        return err;

    if ((err = cyelflib::elf::CopySectionToHex(elf, &hex, CHIPPROT_SECTION, 1)).IsNotOK())
        return err;

    /* We are done with the elf data structre. Because it has a handle on the .elf file of the
       project, we need to close that handle before closing the hex file handle in order to be
       able to detect wether the hex file is up-to-date with respect the elf file. Cleanup
       accomplishes this for us.
       */
    elf.Cleanup();

#ifdef _MSC_VER
    _wunlink(hexFile.c_str());
#else
    unlink(wstr_to_str(hexFile).c_str());
#endif
#ifdef _MSC_VER
    std::ofstream hexStrm(hexFile.c_str());
#else
    std::ofstream hexStrm(wstr_to_str(hexFile).c_str());
#endif
    if (hexStrm.fail())
        return CyErr(CyErr::FAIL, L"Unable to open %ls for writing.", hexFile.c_str());
    err = hex.Write(hexStrm);
    hexStrm.close();
    if (err.IsNotOK())
#ifdef _MSC_VER
        _wunlink(hexFile.c_str());
#else
        unlink(wstr_to_str(hexFile).c_str());
#endif
    return err;
}

CyErr elf2acd(
    uint32_t flashSize,
    uint32_t flashArraySize,
    uint32_t flashRowSize,
    uint32_t metaAddr,
    bool includeEeprom,
    uint32_t eeRowSize,
    uint32_t eeArraySize,
    uint8_t eeArray,
    bool offset,
    CyElfFile &elf,
    vector<CyACDFileLine *> &acdLines)
{
    CyErr err;
    uint32_t firstLoadableRow, lastLoadableRow;

    std::pair<uint32_t, uint32_t> bounds = GetLoadableBounds(elf, flashRowSize, metaAddr);
    // these variables are 0-based
    firstLoadableRow = bounds.first;
    lastLoadableRow = bounds.second;

    uint32_t minAddress = firstLoadableRow * flashRowSize;
    uint32_t maxAddress = (lastLoadableRow + 1) * flashRowSize;

    uint32_t deviceFlashRows = flashSize / flashRowSize;
    uint32_t loadableRowCount = lastLoadableRow - firstLoadableRow + 1;
    vector<uint8_t> loadableFlash(flashSize - minAddress);  // FLASH data starting from firstLoadableRow

    // Get bootloadable flash data
    for (int i = 0; i < elf.PhdrCount(); i++)
    {
        const GElf_Phdr phdr = elf.GetPhdr(i);

        if (phdr.p_type == PT_LOAD && phdr.p_filesz != 0 &&
            ((phdr.p_paddr + phdr.p_filesz > minAddress && phdr.p_paddr < maxAddress) ||
             (phdr.p_paddr + phdr.p_filesz > metaAddr && phdr.p_paddr < flashSize)))
        {
            uint32_t pBegin = (uint32_t)phdr.p_paddr;
            uint32_t pEnd = std::min(flashSize, (uint32_t)(pBegin + phdr.p_filesz));

            uint32_t pOffset = 0;
            if (pBegin < minAddress)
            {
                pOffset = minAddress - pBegin;
                pBegin += pOffset;
            }
            uint32_t arrOffset = (uint32_t)(pBegin - firstLoadableRow*flashRowSize);
            if ((err = elf.ReadPhdrData(&phdr,
                pOffset,
                pEnd - pBegin,
                &loadableFlash[arrOffset])).IsNotOK())
                return err;
        }
    }

    vector<uint8_t> rowData(flashRowSize);

    // Get CONFIG data (if any)
    static const uint32_t FLASH_PER_ECC = 8;
    uint32_t eccPerRow = flashRowSize / FLASH_PER_ECC;
    uint32_t eccStart = firstLoadableRow * eccPerRow;
    vector<uint8_t> eccData;
    Elf_Scn *eccScn = GetSectionEx(&elf, CONFIGECC_SECTION);
    if (eccScn)
    {
        GElf_Shdr eccShdr;
        if ((err = elf.GetShdr(eccScn, &eccShdr)).IsNotOK())
            return err;
        if (eccShdr.sh_type != SHT_NOBITS)
        {
            if (eccShdr.sh_addr < CyPsocHexFile::ADDRESS_CONFIG ||
                eccShdr.sh_addr >= CyPsocHexFile::ADDRESS_CONFIG + CyPsocHexFile::MAX_SIZE_CONFIG)
                return CyErr(CyErr::FAIL, L"%ls: %ls data out of range",
                    elf.Path().c_str(), str_to_wstr(CONFIGECC_SECTION).c_str());
            uint32_t dataStart = (uint32_t)(eccShdr.sh_addr - CyPsocHexFile::ADDRESS_CONFIG);
            uint32_t dataEnd = (uint32_t)(dataStart + eccShdr.sh_size);
            dataStart = std::max(dataStart, eccStart);
            // Round up to nearest row
            uint32_t lastEccRow = (dataEnd - 1) / eccPerRow;
            dataEnd = (lastEccRow + 1) * eccPerRow;
            eccData.resize(dataEnd);
            // If flash has fewer rows than ECC, pad it
            if (lastEccRow >= deviceFlashRows)
                return CyErr(CyErr::FAIL, L"%ls: 0x%04X bytes of ECC data starting at ECC offset 0x%04X "
                    L"will not fit in memory", elf.Path().c_str(), (uint32_t)eccShdr.sh_size, dataStart);
            if (lastLoadableRow < lastEccRow)
            {
                uint32_t diff = lastEccRow - lastLoadableRow;
                loadableRowCount += diff;
                lastLoadableRow += diff;
            }

            if ((err = elf.ReadData(eccScn,
                (uint32_t)(CyPsocHexFile::ADDRESS_CONFIG + dataStart - eccShdr.sh_addr),    // offset
                dataEnd - dataStart,    // length
                &eccData[dataStart])).IsNotOK())
                return err;

            // If ECC data is present, it will appear at the end of each Flash row.
            rowData.resize(flashRowSize + eccPerRow);
        }
    }

    const uint32_t rowsPerArray = flashArraySize / flashRowSize;

    // offset implies that we are a 'Stack' (bootloader+bootloadable) based project.  We are a 
    // need ot offset our acd file so we don't overwrite the current stack image.  We know we
    // have a multi-app type architecture and that the 'stack' must be less than 1/2 the size
    // of the flash not consumed by the launcher. So the offset location is:
    // (# Rows in Dev - First Stack Row - 2 Metadata Rows) / 2. Add 1 before dividing to make sure we round up
    const uint32_t rowOffset = ((deviceFlashRows - firstLoadableRow - 2) + 1) / 2;

    uint32_t lastACDRow = lastLoadableRow;

    for (uint32_t row = firstLoadableRow; row <= lastACDRow; row++)
    {
        uint32_t relativeRow = row - firstLoadableRow;

        memcpy(&rowData[0], &loadableFlash[relativeRow*flashRowSize], flashRowSize);
        // ECC data (if any) goes at the end of each flash row
        if ((row + 1) * eccPerRow <= eccData.size())
            memcpy(&rowData[flashRowSize], &eccData[row*eccPerRow], eccPerRow);
        else if (rowData.size() > flashRowSize)
            memset(&rowData[flashRowSize], 0, eccPerRow);

        // if we are using a launcher/copier, we need to offset the target row to after the loadable
        uint32_t targetRow = row;
        if (offset)
            targetRow = targetRow + rowOffset;
        
        // if we offset the data into metadata row then exit with error.
        if (targetRow >= deviceFlashRows)
        {
            errx(EXIT_FAILURE, L"Error: flash row: %d is either already used or does not exist.", targetRow);
        }
        acdLines.push_back(new CyACDFileLine((uint8_t)(targetRow * flashRowSize / flashArraySize),
            (uint16_t)(targetRow % rowsPerArray), rowData));
    }

    // The metadata is usually outside the range of normal bootloadable rows, so add it here if not already added
    // All ECC should already be taken care of
    if (rowData.size() > flashRowSize)
        memset(&rowData[flashRowSize], 0, eccPerRow);
    {
        uint32_t metaRow = metaAddr / flashRowSize;
        if (metaRow < deviceFlashRows && metaRow > lastLoadableRow)
        {
            uint32_t relativeRow = metaRow - firstLoadableRow;
            memcpy(&rowData[0], &loadableFlash[relativeRow*flashRowSize], flashRowSize);

            // now that we have copied the data from its true flash location, check if we need to offset it.
            // If m_offset is true, then this .cyacd file is actually offset from its real location so that
            // it is copied by the bootloader into a higher memory range. Then it is copied from that higher
            // memory range into the correct location. During this, we need to not over write any pre-existing
            // metadata for the previous revision of the loadable, so we offset the metadata by one row. It
            // will be copied to the correct location by the launcher/copier.
            if (offset)
            {
                metaRow -= 1;
                //Update our copy flag for ACD file if our image is offset, image needs to be copied to right location
                rowData[rowData.size() - CyPsocHexFile::ACD_META_DATA_LEN + CyPsocHexFile::BootloaderMetaData::COPIER_OFFSET] = offset ? 1 : 0;
            }

            acdLines.push_back(new CyACDFileLine((uint8_t)(metaRow * flashRowSize / flashArraySize),
                (uint16_t)(metaRow % rowsPerArray), rowData));
        }
    }

    // Get EEPROM data (if any)
    Elf_Scn *eeScn = GetSectionEx(&elf, EEPROM_SECTION);
    if (includeEeprom && eeScn)
    {
        assert(eeRowSize);
        assert(eeArraySize % eeRowSize == 0);
        GElf_Shdr eeShdr;
        if ((err = elf.GetShdr(eeScn, &eeShdr)).IsNotOK())
            return err;
        if (eeShdr.sh_type == SHT_PROGBITS && eeShdr.sh_size != 0)
        {
            if (eeShdr.sh_addr < CyPsocHexFile::ADDRESS_EEPROM)
                return CyErr(CyErr::FAIL, L"%ls: %ls out of range",
                    elf.Path().c_str(), str_to_wstr(META_SECTION).c_str());
            uint32_t eeStart = (uint32_t)(eeShdr.sh_addr - CyPsocHexFile::ADDRESS_EEPROM);
            uint32_t eeEnd = (uint32_t)(eeStart + eeShdr.sh_size);
            uint32_t eeFirstRow = eeStart / eeRowSize;
            uint32_t eeLastRow = (eeEnd - 1) / eeRowSize;
            uint32_t eeSize = (eeLastRow - eeFirstRow + 1) * eeRowSize;

            uint32_t eeRowPerArray = eeArraySize / eeRowSize;

            vector<uint8_t> eeData(eeSize);
            if ((err = elf.ReadData(eeScn, 0, (uint32_t)eeShdr.sh_size, &eeData[eeStart % eeRowSize])).IsNotOK())
                return err;
            vector<uint8_t> eeRowData(eeRowSize);
            for (uint32_t eeRow = 0; eeRow <= eeLastRow - eeFirstRow; eeRow++)
            {
                fill_n(eeRowData.begin(), eeRowData.size(), 0);
                memcpy(&eeRowData[0], &eeData[eeRow*eeRowSize], eeRowSize);
                acdLines.push_back(new CyACDFileLine(
                    eeArray + (uint8_t)(eeRow / eeRowPerArray), (uint16_t)eeRow % eeRowPerArray, eeRowData));
            }
        }
    }

    return err;
}

} // namespace cyelftool
