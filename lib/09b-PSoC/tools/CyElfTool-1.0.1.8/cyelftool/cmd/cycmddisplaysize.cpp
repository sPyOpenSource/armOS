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
#include "cyelfcmd.h"
#include "commandline.h"
#include "utils.h"
#include "elf/cyelfutil.h"

using std::wstring;
using cyelflib::CyErr;
using cyelflib::hex::CyPsocHexFile;
using namespace cyelflib;
using namespace cyelflib::elf;

namespace cyelftool {
namespace cmd {

void DisplaySize(const CommandLine &cmd)
{
    static const uint32_t SRAM_MIN =  0x1FFF8000u;
    static const uint32_t SRAM_MAX =  0x3FFFFFFFu;

    uint64_t totalFlash, totalSram, bootloaderFlash;
    GetSizes(cmd.PrimaryInput().c_str(), SRAM_MIN, SRAM_MAX, cmd.ExcludeFromSize(), totalFlash, totalSram, bootloaderFlash);

#ifdef _MSC_VER
    printf("code:%I64u\tsram:%I64u\tbootloader:%I64u\n", totalFlash, totalSram, bootloaderFlash);
#else
    printf("code:%llu\tsram:%llu\tbootloader:%llu\n",
        (unsigned long long)totalFlash,
        (unsigned long long)totalSram,
        (unsigned long long)bootloaderFlash);
#endif
}

} // namespace cmd
} // namespace cyelftool
