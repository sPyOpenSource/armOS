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
#include "elf/cyelfutil.h"
#include "cyelfcmd.h"
#include "hex/cypsochexfile.h"
#include "elf2hex.h"
#include "utils.h"
#include "elf/elfxx.h"
#include <fcntl.h>
#include <cctype>
#include <vector>
#include "commandline.h"

using std::wstring;
using std::vector;
using cyelflib::CyErr;
using cyelflib::hex::CyPsocHexFile;
using namespace cyelflib;
using namespace cyelflib::elf;

namespace cyelftool {
namespace cmd {

void
ChecksumCommand(const CommandLine& cmd)
{
    CyElfFile elf(cmd.PrimaryInput());
    unsigned int appChecksum;
    CyErr err;

    if ((err = elf.Read(true)).IsNotOK())
    {
        errx(EXIT_FAILURE, L"open of ELF file %ls failed", elf.Path().c_str());
    }

    appChecksum = ComputeChecksumFromElf(elf);

    if ((err = StoreMetaChecksums(&elf, appChecksum)).IsNotOK())
        errx(EXIT_FAILURE, L"Failed to store checksums in %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    if ((err = AdjustHeaders(&elf)).IsNotOK())
        errx(EXIT_FAILURE, L"Failed to update program headers: %ls", err.Message().c_str());

    // Update the ELF file on disk
    if ((err = elf.Write()).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());

    wstring hexFileName = ReplaceExtension(elf.Path(), L".hex");
    err = elf2hex(elf, hexFileName, cmd.FlashSize(), cmd.FlashOffset(), cmd.getFillValue());
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"Failed to generate %ls: %ls", hexFileName.c_str(), err.Message().c_str());
}

} // namespace cmd
} // namespace cyelftool
