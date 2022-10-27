/*
 *  CyElfLib, a library to facilitate ELF file post-processing
 *  Copyright (C) 2013-2017 - Cypress Semiconductor Corp.
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
#include "cyelfutil.h"
#include "hex/cypsochexfile.h"
#include "elfxx.h"
#include <algorithm>
#include <vector>
#include <cerrno>
#include <fcntl.h>
#include <libelf/libelf.h>
#include <libelf/gelf.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstdint>
#include <string>
#include <vector>

using cyelflib::hex::CyPsocHexFile;
using std::endl;
using std::fill_n;
using std::map;
using std::numeric_limits;
using std::set;
using std::string;
using std::vector;
using std::wcerr;
using std::wcout;

namespace cyelflib {
namespace elf {
    
// Constant names for the sections of the ELF file created for PSoC Devices
const char CONFIGECC_SECTION[] = ".cyconfigecc";
const char CUSTNVL_SECTION[] = ".cycustnvl";
const char WOLATCH_SECTION[] = ".cywolatch";
const char EEPROM_SECTION[] = ".cyeeprom";
const char FLASHPROT_SECTION[] = ".cyflashprotect";
const char META_SECTION[] = ".cymeta";
const char CHIPPROT_SECTION[] = ".cychipprotect";
const char LOADERMETA_SECTION[] = ".cyloadermeta";
const char LOADABLEMETA_SECTION[] = ".cyloadablemeta";
const char LOADABLE1META_SECTION[] = ".cyloadable1meta";
const char LOADABLE2META_SECTION[] = ".cyloadable2meta";
const char CHECKSUM_SECTION[] = ".cychecksum";
const char CYBOOTLOADER_SECTION[] = ".cybootloader";
const char CHECKSUM_EXCLUDE_SECTION[] = ".cy_checksum_exclude";

const Elf64_Addr INVALID_ADDR = 0xFFFFFFFFFFFFFFFFull;

// Used to order elf_sections by start address when adding to an ordered collection
bool operator<(const elf_section &a, const elf_section &b) { return a.m_paddr < b.m_paddr; }

// Adds a new string to the shared string table and returns its index
int InsertStringToShStrTab(Elf *elfInfo, const char *str)
{
    static unsigned char *newData = NULL;

    size_t shStrSectionIdx;
    Elf_Scn *section;
    GElf_Shdr hdr;
    Elf_Data *data;
    char *dataPtr;
    int startIdx;

    if ( elf_getshdrstrndx (elfInfo, &shStrSectionIdx ) != 0)
    {
        errx ( EXIT_FAILURE , " elf_getshdrstrndx() failed : %s.", elf_errmsg ( -1));
    }

    if (( section = elf_getscn (elfInfo, shStrSectionIdx )) == NULL )
    {
        errx ( EXIT_FAILURE , " getscn() failed : %s.", elf_errmsg ( -1));
    }
    
    if ( gelf_getshdr (section , &hdr ) != &hdr )
        errx ( EXIT_FAILURE , " getshdr() failed : %s.", elf_errmsg ( -1));

    // make buffer bigger, return index into buf where the name starts, 
    // the names are all contiguous with \0 at the end of each.

    // There is probably a more efficient way to accomplish this...
    
    data = NULL;
    if ((data = elf_getdata (section , data)) == NULL)
    {
        errx ( EXIT_FAILURE , " elf_getdata() failed : %s.", elf_errmsg ( -1));
    }

    // Just include the area at the end to put in the new string, plus NULL
    // The data coming in the first time from the library is owned by it, so
    // we make a new buffer and copy from the original to start.  After that,
    // we just realloc the area bigger each time.
    if (newData == NULL)
    {
        newData = (unsigned char *)malloc(data->d_size + strlen(str) + 1);
        if (newData == NULL)
        {
            errx ( EXIT_FAILURE , " malloc() failed in %s", "InsertStingToShStrTab");
        }

        memcpy(newData, data->d_buf, data->d_size);
    } 
    else
    {
        newData = (unsigned char *)realloc(newData, data->d_size + strlen(str) + 1);
        if (newData == NULL)
        {
            errx ( EXIT_FAILURE , " realloc() failed in %s", "InsertStingToShStrTab");
        }
    }

    data->d_buf = newData;

    dataPtr = (char *)newData;
    startIdx = data->d_size;
    data->d_size = data->d_size + strlen(str) + 1;
    hdr.sh_size = data->d_size;
    strcpy(&(dataPtr[startIdx]), str);

    elf_flagdata(data, ELF_C_SET, ELF_F_DIRTY);
    elf_flagshdr(section, ELF_C_SET, ELF_F_DIRTY);

    if (!gelf_update_shdr(section, &hdr)) 
        errx(EXIT_FAILURE, "gelf_update_shdr() failed: %s", elf_errmsg(-1));

    // Apply changes to in memory version of ELF
    if (elf_update(elfInfo, ELF_C_NULL) < 0)
        errx(EXIT_FAILURE, "elf_update() failed: %s", elf_errmsg(-1));

    return startIdx;
}

// Returns the pointer to the section, or NULL if fails to create the section
Elf_Scn *CreateNewSection(Elf *elfInfo, 
    const char *sectionName, 
    uint64_t sectionAddr, 
    size_t sectionSize)
{
    Elf_Scn *newSection;
    Elf_Data *newData;
    GElf_Shdr newHdr;
    uint8_t *buf = (uint8_t *)calloc(sectionSize, sizeof(uint8_t));

    if ((newSection = elf_newscn(elfInfo)) == NULL)
        errx(EXIT_FAILURE, "elf_newscn() failed: %s", elf_errmsg(-1));

    if ((newData = elf_newdata(newSection)) == NULL)
        errx(EXIT_FAILURE, "elf_newdata() failed: %s", elf_errmsg(-1));
    
    // Setup section data
    newData->d_align = 1; // Byte alignment
    newData->d_off = 0;
    newData->d_buf = buf;
    newData->d_size = sectionSize;
    newData->d_type = ELF_T_BYTE; 
    newData->d_version = EV_CURRENT;
    
    if ( gelf_getshdr (newSection , &newHdr ) != &newHdr )
        errx ( EXIT_FAILURE , " getshdr ()  failed : %s.", elf_errmsg ( -1));

    // All good, configure header
    memset(&newHdr, 0, sizeof(newHdr));
    newHdr.sh_name = InsertStringToShStrTab(elfInfo, sectionName);
    newHdr.sh_type = SHT_PROGBITS;
    newHdr.sh_addr = sectionAddr;
    newHdr.sh_size = sectionSize;
    newHdr.sh_entsize = 0;
    newHdr.sh_addralign = newData->d_align;

    if (!gelf_update_shdr(newSection, &newHdr)) 
        errx(EXIT_FAILURE, "gelf_update_shdr() failed: %s", elf_errmsg(-1));

    // Apply updates to in memory version of ELF
    if (elf_update(elfInfo, ELF_C_NULL) < 0)
            errx(EXIT_FAILURE, "elf_update() failed: %s", elf_errmsg(-1));

    return newSection;
}

// This function returns true if searchName is a substring of secName up to the last ocurrance of <space> ' '
// Needed for IAR/MDK support, whose linker appends a <space> ' ' with some trailing metadata characters to some section names
int CompareSecnNameEx(const std::string &secName, const std::string &searchName)
{
    std::string secNameCopy = secName.substr();
    size_t idx = secNameCopy.find_last_of(' ');
    if (idx != std::string::npos)
        secNameCopy = secNameCopy.substr(0, idx);
    return secNameCopy.compare(searchName);
}

Elf_Scn *GetSectionEx(const CyElfFile *elf, const std::string  &name)
{
    int i;
    for (i = 0; i < elf->ShdrCount(); i++)
    {
        std::string secName = elf->GetString(elf->GetShdr(i).sh_name);
        
        if (0 == CompareSecnNameEx(secName, name))
            return elf->GetSection(i);
    }

    return NULL;
}

std::pair<uint32_t, uint32_t> GetSectionStartEndAddr(const CyElfFile *elf, const std::string &name)
{
    std::pair<uint32_t, uint32_t> bounds = std::make_pair(0u, 0u);

    Elf_Scn *section = GetSectionEx(elf, name);
    if (section)
    {
        GElf_Shdr sectionShdrInfo;
        CyErr err = elf->GetShdr(section, &sectionShdrInfo);
        if (err.IsNotOK())
            errx(EXIT_FAILURE, L"Failed to look up checksum exclude section info in %ls. Error: %ls.", elf->Path().c_str(), err.Message().c_str());
        bounds = std::make_pair((uint32_t)sectionShdrInfo.sh_addr, (uint32_t)(sectionShdrInfo.sh_addr + sectionShdrInfo.sh_size));
    }

    return bounds;
}

Elf64_Addr GetPhdrAddressForSection(const CyElfFile &elf, const GElf_Shdr &shdr)
{
    Elf64_Addr paddr = INVALID_ADDR;
    for (int j = 0; j < elf.PhdrCount() && paddr == INVALID_ADDR; j++)
    {
        const GElf_Phdr &phdr = elf.GetPhdr(j);
        if (shdr.sh_addr >= phdr.p_vaddr && shdr.sh_addr < phdr.p_vaddr + phdr.p_filesz)
            paddr = phdr.p_paddr + (shdr.sh_addr - phdr.p_vaddr);
    }
    //MDK does not generate a dedicated section for RAM, so the comparison above does not work.
    //Instead we need a second pass to find it based on overlapping file offsets
    if (paddr == INVALID_ADDR)
    {
        for (int j = 0; j < elf.PhdrCount() && paddr == INVALID_ADDR; j++)
        {
            const GElf_Phdr &phdr = elf.GetPhdr(j);
            if (shdr.sh_offset >= phdr.p_offset && shdr.sh_offset < phdr.p_offset + phdr.p_filesz)
                paddr = phdr.p_paddr + (shdr.sh_offset - phdr.p_offset);
        }
    }
    return paddr;
}

CyErr AddSections(const CyElfFile &elf, const std::wstring &path, set<elf_section> &collection)
{
    for (int i = 0; i < elf.ShdrCount(); i++)
    {
        const GElf_Shdr &shdr = elf.GetShdr(i);
        Elf_Scn *scn = elf.GetSection(i);
        Elf_Data *data = scn ? elf.GetFirstData(scn) : nullptr;
        if ((shdr.sh_flags & SHF_ALLOC) && shdr.sh_type != SHT_NOBITS && data && data->d_buf)
        {
            // Find load address
            // Only the load address is checked for overlaps
            Elf64_Addr paddr = GetPhdrAddressForSection(elf, shdr);
            if (paddr == INVALID_ADDR){
                return CyErr(CyErr::FAIL, L"Load address not found for 0x%08llX", shdr.sh_addr);
            }
            set<elf_section>::const_iterator it;
            bool inserted;
            std::tie(it, inserted) = collection.emplace(&elf, path, paddr, shdr.sh_size, data, scn);
            if (!inserted)  // Existing segment has same start address, now check if the contained data is the same
            {
                string sname = elf.GetString(shdr.sh_name);
                if (sname == META_SECTION || sname == CHECKSUM_SECTION){
                    continue;
                }
                if (paddr != it->m_paddr || shdr.sh_size != it->m_size || data->d_size != it->m_data->d_size ||
                    !std::equal((uint8_t *)(data->d_buf), (uint8_t *)(data->d_buf) + data->d_size, (uint8_t *)(it->m_data->d_buf)))
                {
                    return CyErr(CyErr::FAIL, L"Merge error: Section 0x%08llX at %ls overlaps section 0x%08llX from %ls,\nbut it contains different data",
                        shdr.sh_addr, path.c_str(), it->m_paddr, it->m_path.c_str());
                }
            }
        }
    }
    return CyErr();
}

// This function takes an ordered set of elf_section objects (see cyelfutil.cpp for elf_section operator<() overload)
// ordered by start address. Given this order, we need only check adjacent elements of the collection for overlap.
CyErr CheckForOverlaps(const set<elf_section> &collection)
{
    // TODO: check virtual addr as well?
    CyErr err;
    if (collection.size() <= 1)
        return err;
    set<elf_section>::const_iterator it = collection.begin(), prev = it;
    for (++it; it != collection.end(); ++it)
    {
        Elf64_Addr start1 = prev->m_paddr, end1 = start1 + prev->m_size;  // end is exclusive
        Elf64_Addr start2 = it->m_paddr, end2 = start2 + it->m_size;
        if (end2 > start1 && start2 < end1)
        {
            err = CyErr(CyErr::FAIL, L"Merge error: section at 0x%08llX from %ls overlaps section at %08llX from %ls", 
                         start1, prev->m_path.c_str(), start2, it->m_path.c_str());
        }
        prev = it;
    }
    return err;
}

bool AlreadyHasSection(const CyElfFile &elf, uint32_t startAddress)
{
    for (int i = 0; i < elf.ShdrCount(); i++)
    {
        const GElf_Shdr &shdr = elf.GetShdr(i);
        Elf_Scn *scn = elf.GetSection(i);
        Elf_Data *data = scn ? elf.GetFirstData(scn) : nullptr;
        if ((shdr.sh_flags & SHF_ALLOC) && shdr.sh_type != SHT_NOBITS && data && data->d_buf)
        {
            // Find load address
            // Only the load address is checked for overlaps
            Elf64_Addr paddr = GetPhdrAddressForSection(elf, shdr);
            if (paddr == startAddress)
                return true;
        }
    }
    return false;
}

CyErr DuplicateSection(const CyElfFile &src, const elf_section &srcSection, CyElfFile &dest,
    std::unordered_set<std::string> &secNames, int *secIdx)
{
    GElf_Shdr srcHdr;
    CyErr err = src.GetShdr(srcSection.m_scn, &srcHdr);
    if (err.IsNotOK()){
        return err;
    }

    // Skip .cymeta and .cychecksum sections
    string sname = src.GetString(srcHdr.sh_name);
    if (sname == META_SECTION || sname == CHECKSUM_SECTION){
        return CyErr();
    }

    // Generate new section names
    char newName[24];
    do
    {
        sprintf(newName, ".merged%d", (*secIdx)++);
    } while (secNames.find(newName) != secNames.end());
    secNames.insert(newName);

    // Fix for JIRA CYELFTOOL-24
    // Here we can strip the virtual addres associated with a merged section because it will not be needed for debug. 
    // Only phys addr is needed for programming
    Elf64_Addr paddr = GetPhdrAddressForSection(src, srcHdr);
    if (paddr == INVALID_ADDR){
        return CyErr(CyErr::FAIL, L"ERROR %ls: could not find phdr entry for section %ls", src.Path().c_str(), str_to_wstr(sname).c_str());
    }
    srcHdr.sh_addr = paddr;

    Elf64_Xword flags = srcHdr.sh_flags & ~(SHF_INFO_LINK | SHF_LINK_ORDER);
    Elf_Scn *dstSection = dest.AddSection(
        newName, SHT_PROGBITS, srcHdr.sh_addr, flags, uint32_t(srcHdr.sh_addralign), size_t(srcHdr.sh_size));
    Elf_Data *srcData = src.GetFirstData(srcSection.m_scn), *dstData = dest.GetFirstData(dstSection);
    if (srcData && dstData)
    {
        memcpy(dstData->d_buf, srcData->d_buf, std::min(srcData->d_size, dstData->d_size));
        dstData->d_align = srcData->d_align;
        dstData->d_type = srcData->d_type;
        dest.DirtyData(dstData);
    }
    return err;
}

bool SectionHasData(const CyElfFile &elf, Elf_Scn *scn)
{
    GElf_Shdr shdr;
    if (elf.GetShdr(scn, &shdr).IsNotOK())
        return false;
    if (!(shdr.sh_flags & SHF_ALLOC) || shdr.sh_type == SHT_NOBITS || shdr.sh_size == 0)
        return false;
    const Elf_Data *data = elf.GetFirstData(scn);
    return data && data->d_size && data->d_buf;
}

std::vector<GElf_Phdr> MakePhdrsForSections(CyElfFile &output, const std::wstring &primaryInput, const set<elf_section> &sections)
{
    vector<GElf_Phdr> ret;
    map<Elf64_Addr, Elf64_Off> outputSectionOffsets;
    for (int secIdx = 0; secIdx < output.ShdrCount(); secIdx++)
    {
        const GElf_Shdr &shdr = output.GetShdr(secIdx);
        if (shdr.sh_flags & SHF_ALLOC)
        {
            outputSectionOffsets.emplace(shdr.sh_addr, shdr.sh_offset);
        }
    }

    for (auto &inputSec : sections)
    {
        const CyElfFile &input = *inputSec.m_elf;
        GElf_Shdr shdr;
        if (!SectionHasData(input, inputSec.m_scn))
            continue;
        CyErr err = input.GetShdr(inputSec.m_scn, &shdr);
        if (err.IsNotOK()){
            output.Cleanup();
            file_delete(output.Path().c_str());
            errx(EXIT_FAILURE, "%ls: %ls", inputSec.m_path.c_str(), err.Message().c_str());
        }

        Elf64_Addr paddr = GetPhdrAddressForSection(input, shdr);
        if (paddr == INVALID_ADDR) {
            output.Cleanup();
            file_delete(output.Path().c_str());
            errx(EXIT_FAILURE, L"%ls: phdr not found for %ls", input.Path().c_str(), str_to_wstr(input.GetString(shdr.sh_name)).c_str());
        }

        // Get valid offset for section. If elf file associated with section != primaryInput, then it came from a secondary input file during merge.
        Elf64_Addr vaddr = (input.Path() != primaryInput)? inputSec.m_paddr : shdr.sh_addr;
        auto it = outputSectionOffsets.find(vaddr);

        Elf64_Off offset = (it != outputSectionOffsets.end()) ? it->second : shdr.sh_offset;

        Elf_Data *data = input.GetFirstData(inputSec.m_scn);
        Elf64_Xword size = data ? data->d_size : shdr.sh_size;

        GElf_Phdr phdr;
        memset(&phdr, 0, sizeof(phdr));
        phdr.p_type = PT_LOAD;
        phdr.p_flags = PF_R | ((shdr.sh_flags & SHF_WRITE) ? PF_W : 0) | ((shdr.sh_flags & SHF_EXECINSTR) ? PF_X : 0);
        phdr.p_offset = offset;
        phdr.p_vaddr = vaddr;
        phdr.p_paddr = paddr;
        phdr.p_filesz = size;
        phdr.p_memsz = shdr.sh_size;
        phdr.p_align = shdr.sh_addralign;
        ret.push_back(phdr);
    }
    return ret;
}

CyErr CopySectionToHex(const CyElfFile &elfFile, cyelflib::hex::CyHexFile *hexFile,
    const string &sectionName, uint32_t maxSectionLength, uint32_t base)
{
    // maxSectionLength is the size to pad the section to (including padding starting at base)
    // base controls where the section in the hex file starts. The default is the section's address.

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

        if (base == numeric_limits<uint32_t>::max())
            base = (uint32_t)shdr.sh_addr;
        uint32_t size = (uint32_t)shdr.sh_size;

        if (shdr.sh_addr < base)
            return CyErr(CyErr::FAIL, L"Section %ls cannot start before 0x%08X",
            str_to_wstr(sectionName).c_str(), base);
        if (maxSectionLength != numeric_limits<uint32_t>::max())
        {
            if (shdr.sh_addr >= base + maxSectionLength)
                return CyErr(CyErr::FAIL, L"Section %ls cannot start after 0x%X",
                str_to_wstr(sectionName).c_str(), base);
            // Truncate if it is too long
            if (shdr.sh_addr + size - base > maxSectionLength)
                size = (uint32_t)(maxSectionLength - (shdr.sh_addr - base));
        }

        vector<uint8_t> data((uint32_t)(shdr.sh_addr + size - base));
        if ((err = elfFile.ReadData(scn, 0, size, &data[(uint32_t)(shdr.sh_addr - base)])).IsNotOK())
            return CyErr(CyErr::FAIL, L"Error reading section %ls: %ls",
            str_to_wstr(sectionName).c_str(), err.Message().c_str());

        if (maxSectionLength != numeric_limits<uint32_t>::max())
        {
            data.resize(maxSectionLength);
        }

        hexFile->AppendData(base, &data[0], data.size());
    }

    return CyErr();
}

void GetSizes(const std::wstring &elfFile, uint32_t sramStart, uint32_t sramEnd, const std::unordered_set<std::string> &excludedSections,
    uint64_t &totalFlash, uint64_t &totalSram, uint64_t &totalBootloaderFlash)
{
    CyElfFile elf(elfFile);
    CyErr err(elf.Read());
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"%ls: %ls", elfFile.c_str(), err.Message().c_str());

    totalFlash = totalSram = totalBootloaderFlash = 0;

    for (int i = 1; i < elf.ShdrCount(); i++)
    {
        const GElf_Shdr &shdr = elf.GetShdr(i);
        const char* sectionName = elf.GetString(shdr.sh_name);
        std::string sectionNameString(sectionName);

        if ((shdr.sh_flags & SHF_ALLOC) && !(excludedSections.find(sectionNameString) != excludedSections.end()))
        {
            if ((shdr.sh_type != SHT_NOBITS) &&
                (shdr.sh_addr < (CyPsocHexFile::ADDRESS_PROGRAM + CyPsocHexFile::MAX_SIZE_PROGRAM)))
            {
                totalFlash += shdr.sh_size;
                if (0 == CompareSecnNameEx(sectionName, cyelflib::elf::CYBOOTLOADER_SECTION))
                    totalBootloaderFlash += shdr.sh_size;
            }
            if (shdr.sh_addr >= sramStart && shdr.sh_addr <= sramEnd)
                totalSram += shdr.sh_size;
        }
    }
}

// Iterate over all the sections, grab any within the range of flash, reorder by address
// then iterate over all and compute the checksum
uint32_t
ComputeChecksumFromElf(const CyElfFile &elf, uint64_t flashOffset, size_t flashSize, uint8_t fill/* = 0*/)
{
    const uint64_t flashEnd = flashOffset + flashSize;  // exclusive
    uint32_t appChecksum = 0;

    // Read flash data
    vector<int> phdrs;
    for (int i = 0; i < elf.PhdrCount(); i++)
    {
        const GElf_Phdr &phdr = elf.GetPhdr(i);
        if ((phdr.p_type == PT_LOAD || phdr.p_type == PT_ARM_EXIDX) && phdr.p_filesz != 0 &&
            phdr.p_paddr >= flashOffset && phdr.p_paddr < flashEnd)
            phdrs.push_back(i);
    }
    std::stable_sort(phdrs.begin(), phdrs.end(),
        [&](int aIdx, int bIdx) { return elf.GetPhdr(aIdx).p_paddr < elf.GetPhdr(bIdx).p_paddr; });

    Elf64_Addr addr = flashOffset;
    for (auto phdrIdx : phdrs)
    {
        const GElf_Phdr &phdr = elf.GetPhdr(phdrIdx);
        // Gap
        appChecksum += fill * uint32_t(phdr.p_paddr - addr);
        // Data
        uint8_t buf[4096];
        for (size_t off = 0; off < phdr.p_filesz; off += sizeof(buf))
        {
            size_t readSize = std::min(size_t(phdr.p_filesz - off), sizeof(buf));
            if (elf.ReadPhdrData(&phdr, off, readSize, buf).IsNotOK()){
                file_delete(elf.Path().c_str());
                errx(EXIT_FAILURE, L"ERROR: %ls: could not read program header data", elf.Path().c_str());
            }
            for (size_t j = 0; j < readSize; j++)
                appChecksum += buf[j];
        }
        addr = phdr.p_paddr + phdr.p_filesz;
    }
    // Final gap
    if (addr < flashEnd)
        appChecksum += fill * uint32_t(flashEnd - addr);

    return appChecksum;
}

// Iterate over all the sections, grab any within the range of flash, reorder by address
// then iterate over all and compute the checksum
uint32_t
ComputeChecksumFromElf(const CyElfFile &elf, bool includeConfig/* = true*/)
{
    uint32_t appChecksum = 0;
    CyErr err;

    const uint32_t MAX_ADDR = CyPsocHexFile::ADDRESS_PROGRAM + CyPsocHexFile::MAX_SIZE_PROGRAM;

    // Read flash data using phdrs
    vector<uint8_t> data;
    for (int i = 0; i < elf.PhdrCount(); i++)
    {
        const GElf_Phdr &phdr = elf.GetPhdr(i);

        uint32_t pBegin = (uint32_t)phdr.p_paddr;
        uint32_t pEnd = (uint32_t)(phdr.p_paddr + phdr.p_filesz);
        if (phdr.p_type == PT_LOAD && pEnd > CyPsocHexFile::ADDRESS_PROGRAM && pBegin < MAX_ADDR && phdr.p_filesz)
        {
            pEnd = std::min(pEnd, MAX_ADDR);
            data.resize(pEnd - pBegin);
            fill_n(data.begin(), data.size(), 0);
            if ((err = elf.ReadPhdrData(&phdr, 0, data.size(), &data[0])).IsNotOK())
                errx(EXIT_FAILURE, L"%ls: Error reading data for phdr %d", elf.Path().c_str(), err.Message().c_str());
            for (uint32_t j = 0; j < data.size(); j++)
                appChecksum += data[j];
        }
    }

    if (includeConfig)
        appChecksum = AddEccSectionToChecksum(elf, appChecksum);

    return appChecksum;
}

CyErr StoreMetaChecksums(CyElfFile *elf, uint32_t appChecksum)
{
    // First, put the lower 16bits of the value into the checksum section
    Elf_Scn *checksumSection = GetSectionEx(elf, CHECKSUM_SECTION);
    if (checksumSection == NULL)
    {
        wcout << L"No ELF section " << str_to_wstr(CHECKSUM_SECTION) << L" found, creating one" << endl;
        // Create the checksum section since its never been added before
        checksumSection = CreateNewSection(elf->Handle(),
            CHECKSUM_SECTION,
            CyPsocHexFile::ADDRESS_CHECKSUM,
            sizeof(uint16_t));
    }

    Elf_Data *data = NULL;
    if ((data = elf_getdata(checksumSection, data)) == NULL)
        return CyErr(CyErr::FAIL, L"elf_getdata() failed : %s.", str_to_wstr(elf_errmsg(-1)).c_str());
    if (data->d_size < 2)
        return CyErr(CyErr::FAIL, L"%ls contains fewer than %u bytes", str_to_wstr(CHECKSUM_SECTION).c_str(), 2);
    uint8_t *dataPtr = (uint8_t *)(data->d_buf);

    dataPtr[0] = (uint8_t)(appChecksum >> 8);
    dataPtr[1] = (uint8_t)appChecksum;
    wcout << L"Application checksum calculated and stored in ELF section " << str_to_wstr(CHECKSUM_SECTION) << endl;

    // Then, update the location in the metadata section with the combined
    // checksum/siliconID
    Elf_Scn *metaSection = GetSectionEx(elf, META_SECTION);
    if (metaSection == NULL)
    {
        wcout << L"No ELF section " << str_to_wstr(META_SECTION) << L" found, creating one" << endl;
        // Create the checksum section since its never been added before
        metaSection = CreateNewSection(elf->Handle(),
            META_SECTION,
            CyPsocHexFile::ADDRESS_META,
            CyPsocHexFile::CyPSoCHexMetaData::META_SIZE);
    }

    data = NULL;
    if ((data = elf_getdata(metaSection, data)) == NULL)
        return CyErr(CyErr::FAIL, L"elf_getdata() failed : %s.", str_to_wstr(elf_errmsg(-1)).c_str());
    if (data->d_size < (size_t)CyPsocHexFile::CyPSoCHexMetaData::META_SIZE)
        return CyErr(CyErr::FAIL, L"%ls contains fewer than %u bytes", str_to_wstr(META_SECTION).c_str(),
        CyPsocHexFile::CyPSoCHexMetaData::META_SIZE);
    dataPtr = (uint8_t *)(data->d_buf);

    SetChecksumInMetaDataSection(dataPtr, appChecksum);
    wcout << L"Checksum calculated and stored in ELF section " << str_to_wstr(META_SECTION) << endl;

    elf_flagdata(data, ELF_C_SET, ELF_F_DIRTY);

    return CyErr();
}

void SetChecksumInMetaDataSection(uint8_t *dataPtr, uint32_t appChecksum)
{
    uint32_t siliconId = (uint32_t)(
        (dataPtr[CyPsocHexFile::CyPSoCHexMetaData::SILICONID_OFFSET] << 24) |
        (dataPtr[CyPsocHexFile::CyPSoCHexMetaData::SILICONID_OFFSET + 1] << 16) |
        (dataPtr[CyPsocHexFile::CyPSoCHexMetaData::SILICONID_OFFSET + 2] << 8) |
        (dataPtr[CyPsocHexFile::CyPSoCHexMetaData::SILICONID_OFFSET + 3]));

    appChecksum += siliconId;

    dataPtr[CyPsocHexFile::CyPSoCHexMetaData::CHECKSUM_OFFSET] = (uint8_t)(appChecksum >> 24);
    dataPtr[CyPsocHexFile::CyPSoCHexMetaData::CHECKSUM_OFFSET + 1] = (uint8_t)(appChecksum >> 16);
    dataPtr[CyPsocHexFile::CyPSoCHexMetaData::CHECKSUM_OFFSET + 2] = (uint8_t)(appChecksum >> 8);
    dataPtr[CyPsocHexFile::CyPSoCHexMetaData::CHECKSUM_OFFSET + 3] = (uint8_t)appChecksum;
}

/// \brief Remove phdrs that correspond to metadata sections.
// TODO: better commetn
CyErr AdjustHeaders(CyElfFile *elf, bool isBootloader /* = false*/, uint32_t flashSize/* = uint_32::max*/, bool isMcuElftool/* = false */)
{
    CyErr err;

    // Remove SHF_ALLOC from metadata sections.
    for (int i = 1; i < elf->ShdrCount(); i++)
    {
        GElf_Shdr shdr = elf->GetShdr(i);
        if (shdr.sh_flags & SHF_ALLOC)
        {
            switch (shdr.sh_addr)
            {
            case CyPsocHexFile::ADDRESS_CUSTNVLAT:
            case CyPsocHexFile::ADDRESS_WONVLAT:
            case CyPsocHexFile::ADDRESS_CHECKSUM:
            case CyPsocHexFile::ADDRESS_FLASH_PROTECT:
            case CyPsocHexFile::ADDRESS_META:
            case CyPsocHexFile::ADDRESS_CHIP_PROTECT:
            {
                shdr.sh_flags &= ~SHF_ALLOC;
                Elf_Scn *scn = elf->GetSection(i);
                if (scn)
                {
                    if ((err = elf->SetShdr(scn, &shdr)).IsNotOK())
                        return err;
                }
                break;
            }
            default:
                break;
            }
        }
        if (shdr.sh_flags & SHF_ALLOC)
        {
            // Some sections need to be searched by name.
            const char *secName = elf->GetString(shdr.sh_name);
            if (!isBootloader && 0 == CompareSecnNameEx(secName, LOADERMETA_SECTION) && shdr.sh_addr + shdr.sh_size > flashSize)
            {
                shdr.sh_flags &= ~SHF_ALLOC;
                Elf_Scn *scn = elf->GetSection(i);
                if (scn)
                {
                    if ((err = elf->SetShdr(scn, &shdr)).IsNotOK())
                        return err;
                }
            }
        }
    }

    // Regenerate phdrs based on shdrs
    // When libelf rearranges the layout, the original phdrs will not match.
    // Also, libelf will not automatically update the offset field in the phdrs.
    // We expect that the shdr offsets and phdrs in the ELF representation are the original values as
    // loaded from the file (or the last time AdjustHeaders was called).
    // Note: All non-PT_LOAD phdrs will be lost.
    vector<GElf_Phdr> newPhdrs;
    for (int i = 1; i < elf->ShdrCount(); i++)
    {
        const GElf_Shdr &shdr = elf->GetShdr(i);
        if ((shdr.sh_flags & SHF_ALLOC)
             && ( isMcuElftool  // fix JIRA CYELFTOOL-4. Ignore virtual address and create phdr if SHF_ALLOC is true
             || (shdr.sh_addr <= CyPsocHexFile::ADDRESS_CONFIG || shdr.sh_addr == CyPsocHexFile::ADDRESS_EEPROM)))
        {
            GElf_Phdr phdr;
            memset(&phdr, 0, sizeof(phdr));

            phdr.p_type = PT_LOAD;
            // We'll fill in the offset after the new phdr table has been inserted
            phdr.p_vaddr = shdr.sh_addr;
            phdr.p_paddr = shdr.sh_addr;    // This will be adjusted below if possible
            phdr.p_memsz = shdr.sh_size;
            //phdr.p_offset = shdr.sh_offset;
            phdr.p_flags = PF_R;
            if (shdr.sh_flags & SHF_WRITE)
                phdr.p_flags |= PF_W;
            if (shdr.sh_flags & SHF_EXECINSTR)
                phdr.p_flags |= PF_X;
            phdr.p_align = shdr.sh_addralign;

            if (shdr.sh_type != SHT_NOBITS)
            {
                phdr.p_filesz = shdr.sh_size;
                // Find the original load address
                for (int j = 0; j < elf->PhdrCount(); j++)
                {
                    // Compute the physical address based on the original offsets
                    const GElf_Phdr &oldPhdr = elf->GetPhdr(j);
                    if (oldPhdr.p_type == PT_LOAD &&
                        shdr.sh_offset >= oldPhdr.p_offset &&
                        shdr.sh_offset < oldPhdr.p_offset + oldPhdr.p_filesz)
                    {
                        phdr.p_paddr = oldPhdr.p_paddr + (shdr.sh_offset - oldPhdr.p_offset);
                        break;
                    }
                }
            }
            newPhdrs.push_back(phdr);

        }
    }

    // Store new phdr table
    if ((err = elf->NewPhdrTable(newPhdrs.size())).IsNotOK())
        return err;
    for (unsigned int i = 0; i < newPhdrs.size(); i++)
    {
        if ((err = elf->SetPhdr(i, &newPhdrs[i])).IsNotOK())
            return err;
    }
    elf->DirtyElf();

    // Make sure the shdr offsets are up to date
    // The offsets may change if the size of the phdr table changed
    if ((err = elf->Update()).IsNotOK())
        return err;

    // Fix the section offsets
    for (int i = 0; i < elf->PhdrCount(); i++)
    {
        GElf_Phdr phdr = elf->GetPhdr(i);
        if (phdr.p_type == PT_LOAD)
        {
            for (int j = 1; j < elf->ShdrCount(); j++)
            {
                const GElf_Shdr &shdr = elf->GetShdr(j);
                if (shdr.sh_addr == phdr.p_vaddr && shdr.sh_type != SHT_NOBITS && (shdr.sh_flags & SHF_ALLOC) &&
                    shdr.sh_size != 0)
                {
                    phdr.p_offset = shdr.sh_offset;
                    if ((err = elf->SetPhdr(i, &phdr)).IsNotOK())
                        return err;
                    break;
                }
            }
        }
    }

    return err;
}

uint32_t
AddEccSectionToChecksum(const CyElfFile &elf, uint32_t appChecksum)
{
    // Read flash data using phdrs
    vector<uint8_t> data;
    CyErr err;
    Elf_Scn *scn = GetSectionEx(&elf, CONFIGECC_SECTION);
    if (scn)
    {
        GElf_Shdr shdr;
        if ((err = elf.GetShdr(scn, &shdr)).IsNotOK())
            errx(EXIT_FAILURE, L"%ls: %ls", elf.Path().c_str(), err.Message().c_str());
        if (shdr.sh_type == SHT_PROGBITS && shdr.sh_size)
        {
            data.resize((uint32_t)shdr.sh_size);
            fill_n(data.begin(), data.size(), 0);
            if ((err = elf.ReadData(scn, 0, data.size(), &data[0])).IsNotOK())
                errx(EXIT_FAILURE, L"%ls: %ls", elf.Path().c_str(), err.Message().c_str());
            for (uint32_t j = 0; j < data.size(); j++)
                appChecksum += data[j];
        }
    }

    return appChecksum;
}

CyErr LoadCyMetaSection(CyElfFile &elf, CyPsocHexFile::CyPSoCHexMetaData **metadata)
{
    Elf_Scn *scn = GetSectionEx(&elf, META_SECTION);
    if (!scn)
        return CyErr(CyErr::FAIL, L"%ls not found in %ls", str_to_wstr(META_SECTION).c_str(), elf.Path().c_str());
    Elf_Data *data = elf.GetFirstData(scn);
    if (!data)
        return CyErr(CyErr::FAIL, L"%ls not found in %ls", str_to_wstr(META_SECTION).c_str(), elf.Path().c_str());
    vector<uint8_t> metaBytes(CyPsocHexFile::CyPSoCHexMetaData::META_SIZE);
    memcpy(&metaBytes[0], data->d_buf, std::min(metaBytes.size(), data->d_size));

    *metadata = new CyPsocHexFile::CyPSoCHexMetaData(metaBytes);
    return CyErr();
}

std::pair<uint32_t, uint32_t> GetApplicationBounds(const CyElfFile &elf, uint32_t flashRowSize, uint32_t meta_addr)
{
    CyErr err;
    uint32_t minAddress = 0xFFFFFFFFu, maxAddress = 0;

    // Find the minimum and maximum addresses
    for (int i = 0; i < elf.PhdrCount(); i++)
    {
        const GElf_Phdr &phdr = elf.GetPhdr(i);
        if ((phdr.p_type == PT_LOAD || phdr.p_type == PT_ARM_EXIDX) && phdr.p_paddr < meta_addr)
        {
            uint32_t endAddr = std::min(meta_addr - 1, uint32_t(phdr.p_paddr + phdr.p_filesz - 1));
            if (endAddr > maxAddress)
                maxAddress = endAddr;
            if (phdr.p_paddr < minAddress)
                minAddress = uint32_t(phdr.p_paddr);
        }
    }

    if (minAddress > maxAddress)
        return std::make_pair(0u, 0u);

    uint32_t firstLoadableRow = minAddress / flashRowSize,
        lastLoadableRow = maxAddress / flashRowSize;
    return std::make_pair(firstLoadableRow, lastLoadableRow);
}

} // namespace elf
} // namespace cyelflib
