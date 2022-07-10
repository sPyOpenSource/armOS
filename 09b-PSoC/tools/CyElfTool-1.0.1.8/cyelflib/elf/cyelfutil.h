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

#ifndef INCLUDED_CYELFUTILS_H
#define INCLUDED_CYELFUTILS_H

#include "utils.h"
#include "cyerr.h"
#include "hex/cyhexfile.h"
#include "hex/cypsochexfile.h"
#include <unordered_set>
#include <vector>
#include <libelf/libelf.h>
#include <libelf/gelf.h>

namespace cyelflib {
namespace elf {

extern const char CONFIGECC_SECTION[];
extern const char CUSTNVL_SECTION[];
extern const char WOLATCH_SECTION[];
extern const char EEPROM_SECTION[];
extern const char FLASHPROT_SECTION[];
extern const char META_SECTION[];
extern const char CHIPPROT_SECTION[];
extern const char LOADERMETA_SECTION[];
extern const char LOADABLEMETA_SECTION[];
extern const char LOADABLE1META_SECTION[];
extern const char LOADABLE2META_SECTION[];
extern const char CHECKSUM_SECTION[];
extern const char CYBOOTLOADER_SECTION[];
extern const char CHECKSUM_EXCLUDE_SECTION[];

extern const Elf64_Addr INVALID_ADDR;

class CyElfFile;

struct elf_section
{
    const CyElfFile *m_elf;
    std::wstring m_path;
    Elf64_Addr m_paddr;
    Elf64_Addr m_size;
    Elf_Data *m_data;
    Elf_Scn *m_scn;
    elf_section(const CyElfFile *elf, const std::wstring &path, Elf64_Addr paddr, Elf64_Addr size, Elf_Data *data, Elf_Scn *scn)
        : m_elf(elf), m_path(path), m_paddr(paddr), m_size(size), m_data(data), m_scn(scn) { }
    // operator<() defined in cyelfutil.cpp
};

class CyElfFile;


CyErr AddSections(const CyElfFile &elf, const std::wstring &path, std::set<elf_section> &collection);
CyErr CheckForOverlaps(const std::set<elf_section> &collection);
CyErr DuplicateSection(const CyElfFile &src, const elf_section &srcSection, CyElfFile &dest, std::unordered_set<std::string> &secNames, int *secIdx);
bool AlreadyHasSection(const CyElfFile &elf, uint32_t startAddress);
bool SectionHasData(const CyElfFile &elf, Elf_Scn *sec);
//Checks if a section should be merged.  eg: SectionHasData() && !AlreadyHasSection()
bool SectionShouldBeMerged(const CyElfFile &dstElf, const CyElfFile &srcElf, int srcSecIdx);
std::vector<GElf_Phdr> MakePhdrsForSections(CyElfFile &elf, const std::wstring &primaryInput, const std::set<elf_section> &sections);
int InsertStringToShStrTab(Elf *elfInfo, const char *str);
Elf_Scn *CreateNewSection(Elf *elfInfo, const char *sectionName, uint64_t sectionAddr, size_t sectionSize);
/// GetSectionEx is used to search for a section with a name that starts with the substring "name". This
/// is designed to work with both standard Creator based elf file and IAR-based PSoC elf files. IAR appends
/// a space character and additional characters indicating the access type of a section (e.g. rw or ro).
Elf_Scn *GetSectionEx(const CyElfFile *elf, const std::string  &name);

std::pair<uint32_t, uint32_t> GetSectionStartEndAddr(const CyElfFile *elf, const std::string &name);

/// CompareSecnNameEx is used to compare to strings: searchName is the name of a known Cypress section.
/// secName is the name of a section found in the elf file. The function will remove any spaces and the
/// characters after them from secName before comparing. This allows us to work with IAR, which puts
/// access type letters and a space after our section names.
int CompareSecnNameEx(const std::string &secName, const std::string &searchName);

void GetSizes(const std::wstring &elfFile, uint32_t sramStart, uint32_t sramEnd, const std::unordered_set<std::string> &excludedSections,
    uint64_t &totalFlash, uint64_t &totalSram, uint64_t &totalBootloaderFlash);

uint32_t ComputeChecksumFromElf(const CyElfFile &elf, bool includeConfig = true);

uint32_t ComputeChecksumFromElf(const CyElfFile &elf, uint64_t flashOffset, size_t flashSize, uint8_t fill = 0);

uint32_t AddEccSectionToChecksum(const CyElfFile &elf, uint32_t appChecksum);

void SetChecksumInMetaDataSection(uint8_t *dataPtr, uint32_t appChecksum);

CyErr StoreMetaChecksums(CyElfFile *elf, uint32_t appChecksum);

CyErr AdjustHeaders(CyElfFile *elf, bool isBootloadr = false, uint32_t flashSize = std::numeric_limits<uint32_t>::max(), bool isMcuElftool = false);

CyErr LoadCyMetaSection(CyElfFile &elf, cyelflib::hex::CyPsocHexFile::CyPSoCHexMetaData **metadata);

CyErr CopySectionToHex(const CyElfFile &elfFile, cyelflib::hex::CyHexFile *hexFile, const std::string &sectionName, uint32_t maxSectionLength = std::numeric_limits<uint32_t>::max(), uint32_t base = std::numeric_limits<uint32_t>::max());

std::pair<uint32_t, uint32_t> GetApplicationBounds(const cyelflib::elf::CyElfFile &elf, uint32_t flashRowSize);

} // namespace elf
} // namespace cyelflib

#endif
