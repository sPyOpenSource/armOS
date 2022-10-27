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
#include "cyelfcmd.h"
#include "commandline.h"
#include "utils.h"
#include "elf/cyelfutil.h"

using std::wstring;
using cyelflib::CyErr;
using namespace cyelflib;
using namespace cyelflib::elf;

namespace cymcuelftool {
namespace cmd {

void DisplaySize(const CommandLine &cmd)
{
    static const uint32_t SRAM_MIN = 0x08000000;
    static const uint32_t SRAM_MAX = 0x0FFFFFFF;

    uint64_t totalFlash, totalSram, bootloaderFlash;
    std::unordered_set<std::string> excludedSections; //Don't exclude anything
    GetSizes(cmd.PrimaryInput().c_str(), SRAM_MIN, SRAM_MAX, excludedSections, totalFlash, totalSram, bootloaderFlash);
    assert(0 == bootloaderFlash); //We should not have any bootloader data

#ifdef _MSC_VER
    printf("code:%I64u\tsram:%I64u\n", totalFlash, totalSram);
#else
    printf("code:%llu\tsram:%llu\n",
        (unsigned long long)totalFlash,
        (unsigned long long)totalSram);
#endif
}

} // namespace cmd
} // namespace cymcuelftool
