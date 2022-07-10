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

#ifndef INCLUDED_ELF2HEX_H
#define INCLUDED_ELF2HEX_H

#include "cyerr.h"
#include <string>
#include <vector>

namespace cyelflib {
    namespace hex {
        class CyACDFileLine;
    }
    namespace elf {
        class CyElfFile;
    }
}
namespace cyelftool {

cyelflib::CyErr elf2hex(const std::wstring &elfFile, const std::wstring &hexFile, uint32_t flashSize, uint32_t flashOffset, uint8_t fill);
cyelflib::CyErr elf2hex(cyelflib::elf::CyElfFile &elfFile, const std::wstring &hexFile, uint32_t flashSize, uint32_t flashOffset, uint8_t fill);
cyelflib::CyErr elf2acd(
    uint32_t flashSize,
    uint32_t flashArraySize,
    uint32_t flashRowSize,
    uint32_t metaAddr,
    bool includeEeprom,
    uint32_t eeRowSize,
    uint32_t eeArraySize,
    uint8_t eeArray,
    bool offset,
    cyelflib::elf::CyElfFile &elf,
    std::vector<cyelflib::hex::CyACDFileLine *> &acdLines);

} // namespace cyelftool

#endif
