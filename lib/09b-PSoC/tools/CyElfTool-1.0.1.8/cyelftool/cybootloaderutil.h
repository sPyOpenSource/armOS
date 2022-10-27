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

#ifndef INCLUDED_CYBOOTLOADERUTIL_H
#define INCLUDED_CYBOOTLOADERUTIL_H

#include "cyerr.h"
#include "utils.h"
#include <string>

using namespace cyelflib;

namespace cyelflib {
    namespace elf {
        class CyElfFile;
    }
}

namespace cyelftool {

/// Gets the NVL values from and ELF file (or a hex file with the same name for legacy support). PSoC 3/5 only.
CyErr GetNvlValues(const std::wstring &filename, uint8_t cunvl[4], uint8_t wonvl[4]);

/// Gets the bootloader metadata. Uses the hex file if the ELF file does not contain the data.
CyErr GetBootloaderMeta(cyelflib::elf::CyElfFile &elf, uint32_t flashSize, uint32_t flashRowSize, uint8_t btldrMeta[64]);

/// Gets the bootloader data found in the provided section.
/// Uses the hex file if the ELF file does not contain the data.
CyErr GetBootloaderData(cyelflib::elf::CyElfFile &elf, const char* sectionName, size_t maxSize, uint8_t* metaData, size_t* bytesRead);

/// Gets the last row of bootloader data from a bootloadable file.
uint32_t GetLastBootloaderRow(cyelflib::elf::CyElfFile &elf, uint32_t flashRowSize);

/// \brief Computes the checksum for the bootloader's checksum variable.
///
/// The bootloader checksum is a simple modulo-256 sum of the flash memory.
///
/// \param elf ELF file
/// \param bytesToIgnore Number of initial bytes to ignore.
/// \param blSize the size of the bootloader, added to checksum
/// \param checksum Receives the checksum value.
/// \return Status of the operation.
CyErr ComputeBootloaderChecksum(cyelflib::elf::CyElfFile *elf, uint32_t bytesToIgnore, uint32_t blSize, uint8_t *checksum);

/// \brief Computes the checksum for the bootloadable metadata.
///
/// The bootloadable checksum is the 2's complement of the sum of the flash bytes.
///
/// \param elf ELF file
/// \param minAddr Minimum address (inclusive)
/// \param maxAddr Maximum address (exclusive)
/// \param subtract Value to subtract (old checksum field value, if any)
/// \param includeEcc Whether to include ECC data.
/// \return checksum
uint8_t ComputeBootloadableChecksum(cyelflib::elf::CyElfFile &elf, uint32_t minAddr, uint32_t maxAddr, uint8_t subtract = 0,
    bool includeEcc = true);

/// Finds the end of the bootloader's code/rodata sections.
uint32_t GetBootloaderSize(cyelflib::elf::CyElfFile *elf, uint32_t flashSize, uint32_t flashRowSize);

/// Gets the value of the bootloader size variable.
CyErr LoadBootloaderSize(cyelflib::elf::CyElfFile *elf, const std::string &sizeName, uint32_t *size);

/// Gets the value of the bootloader checksum variable.
CyErr LoadBootloaderChecksum(cyelflib::elf::CyElfFile *elf, const std::string &checksumName, uint8_t *checksum);

/// Sets the value of the bootloader size variable.
CyErr StoreBootloaderSize(cyelflib::elf::CyElfFile *elf, const std::string &sizeName, uint32_t size);

/// Sets the value of the bootloader checksum variable.
CyErr StoreBootloaderChecksum(cyelflib::elf::CyElfFile *elf, const std::string &checksumName, uint8_t checksum);

std::pair<uint32_t, uint32_t> GetLoadableBounds(cyelflib::elf::CyElfFile &elf, uint32_t flashRowSize, uint32_t meta_addr);

bool LoadableMetaDataOverlap(cyelflib::elf::CyElfFile &elf, uint32_t flashRowSize, uint32_t meta_addr);

} // namespace cyelftool

#endif
