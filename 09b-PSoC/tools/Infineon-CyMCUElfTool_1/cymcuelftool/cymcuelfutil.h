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

#include "cyerr.h"
#include "commandline.h"
#include <string>
#include <vector>

#ifndef INCLUDED_MCUELFUTIL_H
#define INCLUDED_MCUELFUTIL_H

namespace cyelflib {
    namespace elf {
        class CyElfFile;
    }
}

namespace cymcuelftool
{
    // TODO: these are hardcoded for now, should come from symbols below
    #define FLASH_SIZE 1048576
    #define FLASH_OFFSET 0x10000000
    #define FLASH_ROW_SIZE 0x100

    //Sections used for security
    extern const char* SEC_NAME_KEY_STORE;
    extern const char* SEC_NAME_TOC;
    extern const char* SEC_NAME_EFUSE;
    //Sections used for application formating
    extern const char* SEC_NAME_APP_HEADER;
    //Symbols used for alternative application formating
    extern const char* SEC_NAME_APP_SIGNATURE;
    extern const char* SYM_NAME_APP_VERIFY_START;
    extern const char* SYM_NAME_APP_VERIFY_LEN;
    //Sections used for bootloading
    extern const char* SEC_NAME_BTLDR_META;
    //Symbols used for bootloading
    extern const char* SYM_NAME_BTLDR_META_START;
    extern const char* SYM_NAME_BTLDR_META_LEN;
    extern const char* SYM_NAME_APP_ID;
    extern const char* SYM_NAME_PRODUCT_ID;
    extern const char* SYM_NAME_CHECKSUM_TYPE;

    std::string SymNameMemStart(int idx);
    std::string SymNameMemLength(int idx);
    std::string SymNameMemRowSize(int idx);

    //Gets the <start, length, rowSize> for each of the different memories in the elf file
    std::vector<std::tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> GetMemories(const cyelflib::elf::CyElfFile &elf);
    //Gets the actual used bounds <start, end> of each memory
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> GetBounds(const cyelflib::elf::CyElfFile &elf, const std::vector<std::tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> &memories);
    //Gets the used regions [memory]<start, end> for each type of memory
    std::vector<std::vector<std::tuple<uint32_t, uint32_t>>> GetUsedRegions(const cyelflib::elf::CyElfFile &elf, const std::vector<std::tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> &memories);
    //Combines different regions together if they share a common memory row
    std::vector<std::vector<std::tuple<uint32_t, uint32_t>>> AlignRegionsToFullRows(const std::vector<std::vector<std::tuple<uint32_t, uint32_t>>> &regions, const std::vector<std::tuple<Elf64_Addr, Elf64_Addr, Elf64_Addr>> &memories);

    //Gets all of the data for the specified addresses
    cyelflib::CyErr GetMemoryData(const cyelflib::elf::CyElfFile &elf, std::vector<uint8_t> &allData, uint32_t minAddress, uint32_t maxAddress, uint32_t vecOffset = 0);

    //Generates a hex file from the provided elf file
    void GenerateHexFile(const std::wstring &elfFilePath, const std::wstring &hexFilePath, uint8_t fillValue);
    //Computes the application checksum and adds it to the elf file
    cyelflib::CyErr PopulateSectionsCymetaCychecksum(const std::wstring &file, const uint8_t fillValue);
    //Adds a the provided data to the named section of the elf file
    cyelflib::CyErr StoreDataInSection(cyelflib::elf::CyElfFile *elf, const std::string &sectionName, const std::vector<uint8_t> data);
    //Calculates a 32bit CRC for the application footer
    cyelflib::CyErr CalculateApplicationChecksumCRC32(cyelflib::elf::CyElfFile *elf, uint32_t *crc);
    //Updates entries in the table of contents (eg: CRC checksum)
    cyelflib::CyErr PatchTableOfContents(cyelflib::elf::CyElfFile *elf);
    //Adds entries to the table of contents
    //cyelflib::CyErr SetupTableOfContents(cyelflib::elf::CyElfFile *elf, uint32_t securityPolicyAddr, uint32_t keyStoreAddr, uint32_t keyStoreSize, uint32_t firstAppAddr);
    //Compute CRC32 checksum of bootloader metadata section and store in last 4 bytes of sectiion
    cyelflib::CyErr PatchBootloaderMetadata(cyelflib::elf::CyElfFile &elf);

} // namespace cymcuelftool

#endif
