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

#ifndef INCLUDED_COMMANDLINE_H
#define INCLUDED_COMMANDLINE_H

#include <string>
#include <vector>
#include <unordered_set>
#include "utils.h"
#include "commandlinebase.h"
#include "cmd/cyelfcmd.h"

namespace cymcuelftool
{

class CommandLine : public cyelflib::CommandLineBase
{
public:
    enum class ActionType
    {
        UNSPECIFIED,  // No argument found.
        HELP,         // Print help information.
        VERSION,      // Print version information.
        COMPLETE,     // Merge multiple core projects into one file
        MERGE,        // Merge multiple applications into one file
        SIGN,         // Sign an application with optional secure signature
        SECURE,       // Secure a complete or merged file
        PATCH,        // Generates an application patch file
        SHARE,        // Generates a code sharing symbol file
        SIZE,         // Displays the amount of flash/ram used by the file
    };

    // Describes supported optional command line flags
    enum ArgFlags
    {
        ARGF_NONE = 0x00000000,
        ARGF_OUTPUT = 0x00000001,
        ARGF_HEXFILE = 0x00000002,
        ARGF_KEYDATA = 0x00000004,
        ARGF_ENCRYPT = 0x00000008,
        ARGF_INITVEC = 0x00000010,
    };

    CommandLine();
    virtual ~CommandLine();

    void DisplayHelp() const override;

    ActionType getAction() const { return m_action; }
    const std::wstring &HexFile() const { return m_hexFile; }

    /// Gets the algorithm for encryption
    cmd::CipherType getCipher() const { return m_cipher; }
    /// Gets the hash algorithm to use for signing the application
    cmd::HashType getHash() const { return m_hash; }
    // Gets the message authentication code type (MAC) to use for calculating the application signature
    cmd::SecureMacType getMacType() const { return m_macType; }
    // Gets the key to use for calculating the application signature
    std::wstring getCipherKey() const { return m_cipherKey; }
    // Gets the initialization vector to use for calculating the application signature (only some ciphers)
    std::wstring getInitVector() const { return m_initVector; }

protected:
    cyelflib::CyErr::CyErrStatus _ReadArguments(const std::vector<std::wstring> &argv) override;

private:
    ActionType m_action;
    cmd::HashType m_hash;
    cmd::CipherType m_cipher;
    std::wstring m_cipherKey;
    std::wstring m_initVector;
    std::wstring m_hexFile;
    cmd::SecureMacType m_macType;
    ArgFlags m_requiredArgs;
    ArgFlags m_supportedArgs;

    cyelflib::CyErr::CyErrStatus ReadOptionalArgs(const std::vector<std::wstring> &argv);
    cyelflib::CyErr::CyErrStatus ReadCipherAlgorithm(const std::wstring &arg);
    cyelflib::CyErr ReadKeyFromFile(const std::wstring &keyFile, std::wstring &dest);
    cyelflib::CyErr::CyErrStatus ConfirmKeyLength(cmd::CipherType, unsigned keylen);
    cyelflib::CyErr::CyErrStatus ReadOptionalHashAlgorithm(const std::vector<std::wstring> &argv, uint32_t idx);
    cyelflib::CyErr::CyErrStatus ReadHashAlgorithm(const std::wstring &arg);
    cyelflib::CyErr::CyErrStatus ReadMacAlgorithm(const std::wstring &arg);
    void ReadKey(const std::vector<std::wstring> &argv, uint32_t &idx);
    uint32_t FindNextArgIdx(const std::vector<std::wstring> &argv, uint32_t startIdx);
};

} // namespace cymcuelftool

#endif
