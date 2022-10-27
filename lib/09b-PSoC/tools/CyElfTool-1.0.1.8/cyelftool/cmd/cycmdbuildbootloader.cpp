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
#include "commandline.h"
#include "cybootloaderutil.h"
#include "cyelfcmd.h"
#include "elf/cyelfutil.h"
#include "elf/elfxx.h"
#include "elf2hex.h"
#include "utils.h"

using std::wstring;
using namespace cyelflib::elf;

namespace cyelftool {
namespace cmd {

/// \brief Performs bootloader post-build steps.
///
/// Stores the bootloader size and checksum in the `$INSTANCE_NAME`_sizeBytes
/// and `$INSTANCE_NAME`_checksum variables, respectively. The bootloader
/// checksum is the modulo-256 sum of all of the bytes in flash (bootloaders
/// cannot have CONFIG data). On m0s8, the bootloader checksum does not include
/// the first 256 bytes of flash as they may be remapped at runtime.
/// Bootloaders also have some data stored in the last flash row.
/// The filename, flash size, row size, m0s8 workaround, and variable name
/// arguments are required.
///
/// \param cmd Command line arguments.
void BuildBootloaderCommand(const CommandLine &cmd)
{
    CyElfFile elf(cmd.PrimaryInput());
    CyErr err(elf.Read(true));
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"Error reading %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Get the size
    uint32_t blSize = GetBootloaderSize(&elf, cmd.FlashSize(), cmd.FlashRowSize());

    // Update the size variable value
    if ((err = StoreBootloaderSize(&elf, cmd.VarSizeName(), blSize)).IsNotOK())
        errx(EXIT_FAILURE, L"Error updating bootloader data for %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    if ((err = AdjustHeaders(&elf, false, cmd.FlashSize())).IsNotOK())
        errx(EXIT_FAILURE, L"Failed to update program headers: %ls", err.Message().c_str());

    if ((err = elf.Write()).IsNotOK())
        errx(EXIT_FAILURE, L"Error writing %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Get the checksum
    uint8_t blChecksum;
    if ((err = ComputeBootloaderChecksum(&elf, cmd.BytesToIgnore(), blSize, &blChecksum)).IsNotOK())
        errx(EXIT_FAILURE, L"Error computing bootloader checksum for %ls: %ls",
            elf.Path().c_str(), err.Message().c_str());

    // Update the variable values
    if ((err = StoreBootloaderChecksum(&elf, cmd.VarChecksumName(), blChecksum)).IsNotOK())
        errx(EXIT_FAILURE, L"Error updating bootloader data for %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    if ((err = AdjustHeaders(&elf, false,  cmd.FlashSize())).IsNotOK())
        errx(EXIT_FAILURE, L"Failed to update program headers: %ls", err.Message().c_str());

    if ((err = elf.Write()).IsNotOK())
        errx(EXIT_FAILURE, L"Error writing %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Compute the regular checksums *after* the bootloader checksum/size have been inserted
    uint32_t checksum = ComputeChecksumFromElf(elf);
    if ((err = StoreMetaChecksums(&elf, checksum)).IsNotOK())
        errx(EXIT_FAILURE, L"Failed to store checksums in %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    if ((err = AdjustHeaders(&elf, false, cmd.FlashSize())).IsNotOK())
        errx(EXIT_FAILURE, L"Failed to update program headers: %ls", err.Message().c_str());

    if ((err = elf.Write()).IsNotOK())
        errx(EXIT_FAILURE, L"Error writing %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Generate hex file
    wstring hexFileName = ReplaceExtension(elf.Path(), L".hex");
    err = elf2hex(elf, hexFileName, cmd.FlashSize(), cmd.FlashOffset(), cmd.getFillValue());
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"Failed to generate %ls: %ls", hexFileName.c_str(), err.Message().c_str());
}

} // namespace cmd
} // namespace cyelftool
