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
#include "cybootloaderutil.h"
#include "elf/elfxx.h"
#include "elf/cyelfutil.h"
#include "utils.h"
#include "hex/cypsochexfile.h"
#include <algorithm>
#include <vector>
#include <iostream>
#include <utility>

namespace cyelftool {

using std::string;
using std::wstring;
using std::vector;
using cyelflib::hex::CyPsocHexFile;
using std::fill_n;
using namespace cyelflib::elf;

CyErr GetNvlValues(const std::wstring &filename, uint8_t cunvl[4], uint8_t wonvl[4])
{
    CyErr err;
    bool useHexFile = false;

    {
        CyElfFile elf(filename);
        if ((err = elf.Read()).IsNotOK())
            errx(EXIT_FAILURE, L"Error reading %ls: %ls", filename.c_str(), err.Message().c_str());
        Elf_Scn *scn = GetSectionEx(&elf, CUSTNVL_SECTION);
        if (scn)
        {
            if ((err = elf.ReadData(scn, 0, 4, &cunvl[0])).IsNotOK())
                errx(EXIT_FAILURE, L"Error reading %ls from %ls: %ls",
                    str_to_wstr(CUSTNVL_SECTION).c_str(), filename.c_str(), err.Message().c_str());
        }
        else
            useHexFile = true;

        scn = GetSectionEx(&elf, WOLATCH_SECTION);
        if (scn)
        {
            if ((err = elf.ReadData(scn, 0, 4, &wonvl[0])).IsNotOK())
                errx(EXIT_FAILURE, L"Error reading %ls from %ls: %ls",
                    str_to_wstr(WOLATCH_SECTION).c_str(), filename.c_str(), err.Message().c_str());
        }
        else
            useHexFile = true;
    }

    if (useHexFile)
    {
        wstring hexFileName = ReplaceExtension(filename, L".hex");
        CyPsocHexFile *hex;
        if ((err = CyPsocHexFile::Read(hexFileName, hex)).IsNotOK())
            errx(EXIT_FAILURE, L"Error reading %ls: %ls", hexFileName.c_str(), err.Message().c_str());

        vector<uint8_t> *hexCunvl = hex->CustNvlData();
        for (unsigned int i = 0; i < std::min((size_t)4, hexCunvl->size()); i++)
            cunvl[i] = (*hexCunvl)[i];
        delete hexCunvl;

        vector<uint8_t> *hexWonvl = hex->WoNvlData();
        for (unsigned int i = 0; i < std::min((size_t)4, hexWonvl->size()); i++)
            wonvl[i] = (*hexWonvl)[i];
        delete hexWonvl;

        delete hex;
    }

    return err;
}

CyErr GetElfData(CyElfFile* elf, Elf_Scn* scn, size_t buffSize, uint8_t* buff, size_t* bytesRead)
{
    CyErr err;
    *bytesRead = 0;
    Elf_Data* eData = elf->GetFirstData(scn);
    if (eData != 0)
    {
        if (eData->d_size <= buffSize)
        {
            *bytesRead += eData->d_size;
            memcpy(&buff[0], eData->d_buf, eData->d_size);
        }
        else
        {
            GElf_Shdr* header = 0;
            elf->GetShdr(scn, header);
            const char* scnName;
            elf->GetShdr(scn, header);
            scnName = elf->GetString(header->sh_name);
            return CyErr(CyErr::FAIL, L"Section %ls has data this is too large, max=%d, actual=%d", scnName, buffSize, eData->d_size);
        }
    }
    return err;
}

const vector<uint8_t>* GetDataBySectionName(CyPsocHexFile* hex, const char* sectionName)
{
    const vector<uint8_t>* data;
    if (0 == CompareSecnNameEx(sectionName, META_SECTION))
        data = hex->MetaData();
    else if (0 == CompareSecnNameEx(sectionName, CUSTNVL_SECTION))
        data = hex->CustNvlData();
    else if (0 == CompareSecnNameEx(sectionName, WOLATCH_SECTION))
        data = hex->WoNvlData();
    else if (0 == CompareSecnNameEx(sectionName, EEPROM_SECTION))
        data = hex->EepromData();
    else if (0 == CompareSecnNameEx(sectionName, FLASHPROT_SECTION))
        data = hex->FlashProtectData();
    else if (0 == CompareSecnNameEx(sectionName, CHIPPROT_SECTION))
        data = hex->ChipProtectData();
    else
    {
        assert(false);
        data = 0;
    }

    return data;
}

CyErr GetBootloaderData(CyElfFile &elf, const char* sectionName, size_t maxSize, uint8_t* buff, size_t* bytesRead)
{
    CyErr err;
    bool useHexFile = false;

    memset(buff, 0, maxSize);

    Elf_Scn* scn = GetSectionEx(&elf, sectionName);
    if (scn)
    {
        GetElfData(&elf, scn, maxSize, buff, bytesRead);
    }
    else
        useHexFile = true;

    if (useHexFile)
    {
        wstring hexFileName = ReplaceExtension(elf.Path(), L".hex");
        CyPsocHexFile *hex;
        if ((err = CyPsocHexFile::Read(hexFileName, hex)).IsNotOK())
            return err;
        const vector<uint8_t> *data = GetDataBySectionName(hex, sectionName);
        *bytesRead = data->size();
        if (*bytesRead != 0 && *bytesRead <= maxSize)
            memcpy(buff, &(*data)[0], *bytesRead);
        delete data;
        delete hex;
    }

    return err;
}

CyErr GetBootloaderMeta(CyElfFile &elf, uint32_t flashSize, uint32_t /* unused: flashRowSize */, uint8_t btldrMeta[64])
{
    CyErr err;
    bool found = false;
    static const uint32_t META_SIZE = CyPsocHexFile::ACD_META_DATA_LEN;
    uint32_t blMetaAddr = flashSize - META_SIZE;
    memset(btldrMeta, 0, META_SIZE);

    for (int i = 0; !found && i < elf.ShdrCount(); i++)
    {
        const GElf_Shdr &shdr = elf.GetShdr(i);
        if (shdr.sh_type == SHT_PROGBITS && (shdr.sh_flags & SHF_ALLOC) &&
            blMetaAddr >= shdr.sh_addr && blMetaAddr < shdr.sh_addr + shdr.sh_size)
        {
            Elf_Scn *scn = elf.GetSection(i);
            uint32_t offset = (uint32_t)(blMetaAddr - shdr.sh_addr);
            if ((err = elf.ReadData(scn, offset, META_SIZE, btldrMeta)).IsNotOK())
                return err;
            found = true;
        }
    }

    if (!found)
    {
        wstring hexFileName = ReplaceExtension(elf.Path(), L".hex");
        CyPsocHexFile *hex;
        if ((err = CyPsocHexFile::Read(hexFileName, hex)).IsNotOK())
            return err;
        const vector<uint8_t> *data = hex->ProgramData();
        if (data->size() >= blMetaAddr + META_SIZE)
            memcpy(btldrMeta, &(*data)[blMetaAddr], META_SIZE);
        delete data;
        delete hex;
    }

    return err;
}

uint32_t GetLastBootloaderRow(CyElfFile &elf, uint32_t flashRowSize)
{
    GElf_Shdr shdr;
    Elf_Scn* scn;
    if (0 == (scn = GetSectionEx(&elf, CYBOOTLOADER_SECTION)))
        errx(EXIT_FAILURE, L"%ls not found in %ls", str_to_wstr(CYBOOTLOADER_SECTION).c_str(), elf.Path().c_str());
    elf.GetShdr(scn, &shdr);
    uint32_t lastAddr = (uint32_t)(shdr.sh_addr + shdr.sh_size - 1);
    return lastAddr / flashRowSize;
}

CyErr ComputeBootloaderChecksum(CyElfFile *elf, uint32_t bytesToIgnore, uint32_t blSize, uint8_t *checksum)
{
    CyErr err;
    vector<uint8_t> data;
    *checksum = 0;
    for (int i = 0; i < elf->PhdrCount() && err.IsOK(); i++)
    {
        const GElf_Phdr &phdr = elf->GetPhdr(i);

        uint32_t pBegin = (uint32_t)phdr.p_paddr;
        uint32_t pEnd = (uint32_t)(pBegin + phdr.p_filesz);
        if (phdr.p_type == PT_LOAD &&
            pBegin < blSize &&
            pEnd > bytesToIgnore &&
            phdr.p_filesz != 0)
        {
            // On m0s8 the CPUSS remaps the first 256 bytes of the program space for some kind of interrupt table
            // hack. The bootloader excludes this portion when computing its checksum.
            pBegin = std::max(pBegin, bytesToIgnore);
            data.resize(pEnd - pBegin);
            fill_n(data.begin(), data.size(), 0);

            err = elf->ReadPhdrData(&phdr,
                (uint32_t)(pBegin - phdr.p_paddr),  // Offset
                data.size(),    // Length
                &data[0]);
            if (err.IsNotOK())
                return err;
            for (size_t j = 0; j < data.size(); j++)
                *checksum += data[j];
        }
    }

    // Two's complement
    *checksum = (uint8_t)(~*checksum + 1);

    return err;
}

uint8_t ComputeBootloadableChecksum(CyElfFile &elf, uint32_t minAddr, uint32_t maxAddr, uint8_t subtract/* = 0*/,
    bool includeEcc/* = true*/)
{
    uint8_t checksum = (uint8_t)-subtract;

    vector<uint8_t> flashBytes;
    for (int i = 0; i < elf.PhdrCount(); i++)
    {
        const GElf_Phdr &phdr = elf.GetPhdr(i);

        uint32_t pBegin = (uint32_t)phdr.p_paddr;
        uint32_t pEnd = (uint32_t)(pBegin + phdr.p_filesz);
        if (phdr.p_type == PT_LOAD &&
            pBegin < maxAddr &&
            pEnd > minAddr &&
            phdr.p_filesz != 0)
        {
            pBegin = std::max(pBegin, minAddr);
            pEnd = std::min(pEnd, maxAddr);
            flashBytes.resize(pEnd - pBegin);
            fill_n(flashBytes.begin(), flashBytes.size(), 0);
            CyErr err = elf.ReadPhdrData(&phdr,
                (uint32_t)(pBegin - phdr.p_paddr),  // Offset
                flashBytes.size(),    // Length
                &flashBytes[0]);
            if (err.IsNotOK())
                errx(EXIT_FAILURE, L"%ls: %ls", elf.Path().c_str(), err.Message().c_str());
            for (size_t j = 0; j < flashBytes.size(); j++)
                checksum += flashBytes[j];
        }
    }

    if (includeEcc)
    {
        for (int i = 1; i < elf.ShdrCount(); i++)
        {
            const GElf_Shdr &shdr = elf.GetShdr(i);
            if (shdr.sh_type == SHT_PROGBITS && shdr.sh_size &&
                shdr.sh_addr >= CyPsocHexFile::ADDRESS_CONFIG &&
                shdr.sh_addr < CyPsocHexFile::ADDRESS_CONFIG + CyPsocHexFile::MAX_SIZE_CONFIG)
            {
                Elf_Scn *scn = elf.GetSection(i);
                Elf_Data *data = elf.GetFirstData(scn);
                while (data)
                {
                    const uint8_t *buf = (const uint8_t *)data->d_buf;
                    for (uint32_t b = 0; b < data->d_size; b++)
                        checksum += buf[b];
                    data = elf.GetNextData(scn, data);
                }
            }
        }
    }

    return (uint8_t)(~checksum + 1u);
}

uint32_t GetBootloaderSize(CyElfFile *elf, uint32_t flashSize, uint32_t flashRowSize)
{
    // Find the maximum section end address in the PROGRAM region, excluding sections that begin in the last
    // flash row.

    const uint32_t ADDR_MIN = CyPsocHexFile::ADDRESS_PROGRAM;
    const uint32_t ADDR_MAX = ADDR_MIN + flashSize - CyPsocHexFile::ACD_META_DATA_LEN;

    // First entry is reserved.
    uint32_t blSize = 0;
    for (int i = 0; i < elf->PhdrCount(); i++)
    {
        const GElf_Phdr &phdr = elf->GetPhdr(i);

        if (phdr.p_type == PT_LOAD && /*phdr.p_paddr >= ADDR_MIN &&*/ phdr.p_paddr < ADDR_MAX)
        {
            uint32_t size = (uint32_t)(phdr.p_paddr - ADDR_MIN + phdr.p_filesz);
            if (size > blSize)
                blSize = size;
        }
    }
    blSize = ((blSize + flashRowSize - 1) / flashRowSize) * flashRowSize;

    return blSize;
}

static CyErr GetBootloaderSym(const CyElfFile *elf,
    const string &variableName,
    GElf_Sym *variableSym)
{
    if (!elf->GetSymbol(variableName.c_str(), variableSym))
        return CyErr(CyErr::FAIL, L"Symbol %ls not found in %ls",
            str_to_wstr(variableName).c_str(), elf->Path().c_str());

    return CyErr();
}

CyErr LoadBootloaderSize(CyElfFile *elf,
    const string &sizeName,
    uint32_t *size)
{
    CyErr err;
    
    *size = 0;

    GElf_Sym sizeSym;
    err = GetBootloaderSym(elf, sizeName, &sizeSym);
    if (err.IsNotOK())
        return err;

    // Entry 0 is reserved
    bool sizeFound = false;
    for (int i = 1; i < elf->ShdrCount() && !sizeFound; i++)
    {
        Elf_Scn *scn = elf->GetSection(i);
        GElf_Shdr shdr;
        if ((err = elf->GetShdr(scn, &shdr)).IsNotOK())
            return err;
        if (shdr.sh_type == SHT_PROGBITS &&
            (shdr.sh_flags & SHF_ALLOC) != 0 &&
            shdr.sh_size != 0)
        {
            if (sizeSym.st_value >= shdr.sh_addr && sizeSym.st_value < shdr.sh_addr + shdr.sh_size)
            {
                size_t offset = (size_t)(sizeSym.st_value - shdr.sh_addr);
                Elf_Data *data = elf->GetFirstData(scn);
                if (data && offset < data->d_size)
                {
                    *size = ReadWithEndian32(elf->Endian(), &(((uint8_t *)data->d_buf)[offset]));
                    sizeFound = true;
                }
            }
        }
    }

    if (!sizeFound)
        return CyErr(CyErr::FAIL, L"Section not found for sizeBytes in %ls", elf->Path().c_str());
    return err;
}

CyErr StoreBootloaderSize(CyElfFile *elf,
    const string &sizeName,
    uint32_t size)
{
    GElf_Sym sizeSym;
    CyErr err(GetBootloaderSym(elf, sizeName, &sizeSym));
    if (err.IsNotOK())
        return err;

    // Entry 0 is reserved
    bool sizeFound = false, checksumFound = false;
    for (int i = 1; i < elf->ShdrCount() && (!sizeFound || !checksumFound); i++)
    {
        Elf_Scn *scn = elf->GetSection(i);
        GElf_Shdr shdr;
        if ((err = elf->GetShdr(scn, &shdr)).IsNotOK())
            return err;
        if (shdr.sh_type == SHT_PROGBITS &&
            (shdr.sh_flags & SHF_ALLOC) != 0 &&
            shdr.sh_size != 0)
        {
            if (sizeSym.st_value >= shdr.sh_addr && sizeSym.st_value < shdr.sh_addr + shdr.sh_size)
            {
                size_t offset = (size_t)(sizeSym.st_value - shdr.sh_addr);
                Elf_Data *data = elf->GetFirstData(scn);
                if (data && offset < data->d_size)
                {
                    WriteWithEndian32(elf->Endian(), &(((uint8_t *)data->d_buf)[offset]), size);
                    sizeFound = true;
                }
                elf->DirtyData(data);
            }
        }
    }
    if (!sizeFound)
        return CyErr(CyErr::FAIL, L"Section not found for sizeBytes in %ls", elf->Path().c_str());
    return err;
}

CyErr LoadBootloaderChecksum(CyElfFile *elf,
    const string &checksumName,
    uint8_t *checksum)
{
    CyErr err;
    
    *checksum = 0;

    GElf_Sym checksumSym;
    err = GetBootloaderSym(elf, checksumName, &checksumSym);
    if (err.IsNotOK())
        return err;

    // Entry 0 is reserved
    bool checksumFound = false;
    for (int i = 1; i < elf->ShdrCount() && !checksumFound; i++)
    {
        Elf_Scn *scn = elf->GetSection(i);
        GElf_Shdr shdr;
        if ((err = elf->GetShdr(scn, &shdr)).IsNotOK())
            return err;
        if (shdr.sh_type == SHT_PROGBITS &&
            (shdr.sh_flags & SHF_ALLOC) != 0 &&
            shdr.sh_size != 0)
        {
            if (checksumSym.st_value >= shdr.sh_addr && checksumSym.st_value < shdr.sh_addr + shdr.sh_size)
            {
                size_t offset = (size_t)(checksumSym.st_value - shdr.sh_addr);
                Elf_Data *data = elf->GetFirstData(scn);
                if (data && offset < data->d_size)
                {
                    *checksum = ((uint8_t *)data->d_buf)[offset];
                    checksumFound = true;
                }
            }
        }
    }

    if (!checksumFound)
        return CyErr(CyErr::FAIL, L"Section not found for checksum in %ls", elf->Path().c_str());
    return err;
}

CyErr StoreBootloaderChecksum(CyElfFile *elf,
    const string &checksumName,
    uint8_t checksum)
{
    GElf_Sym checksumSym;
    CyErr err(GetBootloaderSym(elf, checksumName, &checksumSym));
    if (err.IsNotOK())
        return err;

    // Entry 0 is reserved
    bool checksumFound = false;
    for (int i = 1; i < elf->ShdrCount() && !checksumFound; i++)
    {
        Elf_Scn *scn = elf->GetSection(i);
        GElf_Shdr shdr;
        if ((err = elf->GetShdr(scn, &shdr)).IsNotOK())
            return err;
        if (shdr.sh_type == SHT_PROGBITS &&
            (shdr.sh_flags & SHF_ALLOC) != 0 &&
            shdr.sh_size != 0)
        {
            if (checksumSym.st_value >= shdr.sh_addr && checksumSym.st_value < shdr.sh_addr + shdr.sh_size)
            {
                size_t offset = (size_t)(checksumSym.st_value - shdr.sh_addr);
                Elf_Data *data = elf->GetFirstData(scn);
                if (data && offset < data->d_size)
                {
                    ((uint8_t *)data->d_buf)[offset] = checksum;
                    checksumFound = true;
                }
                elf->DirtyData(data);
            }
        }
    }

    if (!checksumFound)
        return CyErr(CyErr::FAIL, L"Section not found for checksum in %ls", elf->Path().c_str());
    return err;
}

std::pair<uint32_t, uint32_t> GetLoadableBounds(CyElfFile &elf, uint32_t flashRowSize, uint32_t meta_addr)
{
    CyErr err;
    uint32_t minAddress = 0xFFFFFFFFu, maxAddress = 0;

    // Find the end of the bootloader
    uint32_t lastBootloaderRow = GetLastBootloaderRow(elf, flashRowSize);
    uint32_t bootloaderEnd = (lastBootloaderRow + 1) * flashRowSize;
    std::pair<uint32_t, uint32_t> checksumExcludeBounds = GetSectionStartEndAddr(&elf, CHECKSUM_EXCLUDE_SECTION);

    // Find the minimum and maximum addresses
    for (int i = 0; i < elf.PhdrCount(); i++)
    {
        const GElf_Phdr &phdr = elf.GetPhdr(i);

        // Skip sections which are before the end of the bootloader, or in the metadata or checksum exclude sections
        bool isBootloader = phdr.p_paddr + phdr.p_filesz <= bootloaderEnd;
        bool isMeta = phdr.p_paddr >= meta_addr;
        bool isChecksum = checksumExcludeBounds.first > 0 && phdr.p_paddr == checksumExcludeBounds.first;
        bool isIgnoredSection = isBootloader || isMeta || isChecksum;
        if (phdr.p_type == PT_LOAD && !isIgnoredSection)
        {
            uint32_t endAddr = std::min(meta_addr - 1, (uint32_t)(phdr.p_paddr + phdr.p_filesz - 1));
            if (endAddr > maxAddress)
                maxAddress = endAddr;
        }
    }

    for (int i = 1; i < elf.ShdrCount(); ++i)
    {
        const GElf_Shdr &shdr = elf.GetShdr(i);
        if (shdr.sh_type != SHT_NOBITS &&
            (shdr.sh_flags & SHF_ALLOC) &&
            0 != CompareSecnNameEx(elf.GetString(shdr.sh_name), CYBOOTLOADER_SECTION))
        {
            uint32_t startAddr = (uint32_t)shdr.sh_addr;
            if (startAddr < minAddress)
                minAddress = startAddr;
        }
    }

    if (minAddress > maxAddress)
        return std::make_pair(0u, 0u);

    uint32_t firstLoadableRow = minAddress / flashRowSize,
             lastLoadableRow = maxAddress / flashRowSize;
    return std::make_pair(firstLoadableRow, lastLoadableRow);
}

//CDT 206242
//find where the bootloadable memory section and make sure that it does not overlap with metadata
bool LoadableMetaDataOverlap(CyElfFile &elf, uint32_t flashRowSize, uint32_t meta_addr)
{
    //get the entire meta data row not, just the actual meta data
    uint32_t metaDataRowStartByte = (meta_addr / flashRowSize) * flashRowSize;
    for (int i = 0; i < elf.ShdrCount(); i++)
    {
        const GElf_Shdr &shdr = elf.GetShdr(i);
        uint32_t sectionEndAddress = (uint32_t)(shdr.sh_addr + shdr.sh_size - 1);
        if ((shdr.sh_flags & SHF_ALLOC) &&
            shdr.sh_addr < metaDataRowStartByte && sectionEndAddress >= metaDataRowStartByte)
        {
            return true;
        }
    }
    return false;
}

} // namespace cyelftool
