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

#ifndef INCLUDED_COMMANDLINE_H
#define INCLUDED_COMMANDLINE_H

#include <string>
#include <vector>
#include <unordered_set>
#include "utils.h"
#include "commandlinebase.h"

namespace cyelftool
{

class CommandLine : public cyelflib::CommandLineBase
{
public:
    enum ActionType
    {
        UNSPECIFIED,    //!< No argument found.
        HELP,       //!< Print help information.
        VERSION,    //!< Print version information.
        CHECKSUM,   //!< Update checksum, bootloader metadata, create output ELF/HEX files.
        GET_NVL,    //!< Print NVL values from a completed ELF (or ELF+HEX) file.
        GET_SIZE,   //!< Print size information from a completed ELF file.
        PATCH_BOOTLOADER,   //!< Build bootloader. Updates checksums, sets size and checksum variables.
        EXTRACT_BOOTLOADER, //!< Create cybootloader.c in the current directory from the specified ELF (or ELF+HEX) file.
        BUILD_LOADABLE, //!< Build bootloadable ELF, hex file, and cyacd file.
        MERGE_LOADABLES,    //!< Merge two bootloadable ELF files into a single ELF and hex file.
        GENERATE_PROVIDES, //!< Generate LD include file with provides directives for symbols in a bootloader image.
        RENAME_SYMBOLS, //!< Renames symbols in a symdef file.
    };

    CommandLine();
    virtual ~CommandLine();

    void DisplayHelp() const override;

    ActionType getAction() const { return m_action; }
    /// Gets the size of a flash array in bytes. Required for --build_bootloadable and --multiapp_merge.
    uint32_t FlashArraySize() const { return m_flashArraySize; }
    /// Gets the total flash size in bytes.
    uint32_t FlashSize() const { return m_flashSize; }
    /// Gets the offset in bytes for the start of flash memory.
    uint32_t FlashOffset() const { return m_flashOffset; }
    /// Gets the size of a flash row.
    uint32_t FlashRowSize() const { return m_flashRowSize; }
    /// \brief EEPROM row size.
    uint32_t EepromRowSize() const { return m_eeRowSize; }
    /// \brief EEPROM bytes per array
    uint32_t EepromArraySize() const { return m_eeArraySize; }
    /// \brief EEPROM starting array number for the ACD file.
    uint8_t EepromArray() const { return m_eeArrayId; }
    /// Gets the name of the bootloader checksum variable. Required for --build_bootloader.
    const std::string &VarChecksumName() const { return m_varChecksumName; }
    /// Gets the name of the bootloader size variable. Required for --build_bootloader.
    const std::string &VarSizeName() const { return m_varSizeName; }
    /// Number of bytes to ignore for bootloader checksum.
    uint32_t BytesToIgnore() const { return m_ignoreBytes; }
    /// True if the .cyacd file should be offset for launcher/copier.
    bool OffsetBootloadable() const { return m_offset; }
    /// Set of sections to exlude from flash size calculation.
    const std::unordered_set<std::string> &ExcludeFromSize() const { return m_excludeFromSize; }
    /// The toolcahin to geneate code share output file for.
    const std::string &Toolchain() const { return m_toolchain; }

protected:
    cyelflib::CyErr::CyErrStatus _ReadArguments(const std::vector<std::wstring> &argv) override;

private:
    ActionType m_action;
    uint32_t m_flashArraySize;
    uint32_t m_flashSize;
    uint32_t m_flashOffset;
    uint32_t m_flashRowSize;
    uint32_t m_eeRowSize;
    uint32_t m_eeArraySize;
    uint8_t m_eeArrayId;
    std::string m_varChecksumName;
    std::string m_varSizeName;
    uint32_t m_ignoreBytes;
    std::unordered_set<std::string> m_excludeFromSize;
    std::string m_toolchain;
    bool m_offset;

    enum ArgFlags
    {
        ARGF_FLASHSIZE =    0x00000001,
        ARGF_ROWSIZE =      0x00000002,
        ARGF_ARRSIZE =      0x00000004,
        ARGF_IGNOREBYTES =  0x00000008,
        ARGF_BLSYMNAMES =   0x00000010,
        ARGF_EEPROM =       0x00000020,
        ARGF_TOOLCHAIN =    0x00000040,
        ARGF_FLASHOFFSET =  0x00000080,
    };

    cyelflib::CyErr::CyErrStatus ReadFlashArgs(const std::vector<std::wstring> &argv, ArgFlags required =
        (ArgFlags)(ARGF_FLASHSIZE | ARGF_ROWSIZE));
};

} // namespace cyelftool

#endif
