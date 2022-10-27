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

#ifndef INCLUDED_ELF2HEX_H
#define INCLUDED_ELF2HEX_H

#include "cyerr.h"
#include "hex/cypsocacd2file.h"
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

namespace cymcuelftool
{

enum class HexFormatType
{
    NONE = 0,   //!< No special formatting.
    PAD = 1,    //!< Pad out hex file to full flash size.
    SPLIT = 2,  //!< Split hex file between different memories.
};
inline HexFormatType operator|(HexFormatType a, HexFormatType b)
{
    return static_cast<HexFormatType>(static_cast<int>(a) | static_cast<int>(b));
}
inline HexFormatType operator&(HexFormatType a, HexFormatType b)
{
    return static_cast<HexFormatType>(static_cast<int>(a) & static_cast<int>(b));
}

//cyelflib::CyErr elf2hex(const std::wstring &elfFile, const std::wstring &hexFile, HexFormatType format, uint8_t fill);
cyelflib::CyErr elf2hex(const cyelflib::elf::CyElfFile &elf, const std::wstring &hexFile, HexFormatType format, uint8_t fill);
cyelflib::CyErr elf2acd(const cyelflib::elf::CyElfFile &elf, uint8_t fill, const std::wstring &initVec, cyelflib::hex::CyPSoCACD2File* &acd);

} // namespace cymcuelftool

#endif
