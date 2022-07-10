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

#ifndef INCLUDED_CYELFCMD_H
#define INCLUDED_CYELFCMD_H

#include <string>

namespace cymcuelftool {

class CommandLine;

namespace cmd {

enum class CipherType
{
    UNSPECIFIED,  // No argument found.
    RSAES_PKCS,   // Asymmetric RSA public-key encryption using PKCS.
    RSASSA_PKCS,  // Asymmetric RSA public-key signature using PKCS.
    ECC,          // Asymmetric (Elliptic curve cryptography) public-key encryption.
    DES_ECB,      // DES block cipher, an encryption function that works on fixed-size blocks of data.
    TDES_ECB,     // (Triple-DES or 3DES) DES algorithm invoked three times with three different keys.
    AES_128_ECB,  // AES in ECB mode operation works on a 128-bit block with a 128 bit key.
    AES_192_ECB,  // AES in ECB mode operation works on a 128-bit block with a 192 bit key.
    AES_256_ECB,  // AES in ECB mode operation works on a 128-bit block with a 256 bit key.
    AES_128_CBC,  // AES in CBC mode operation works on a 128-bit block with a 128 bit key.
    AES_192_CBC,  // AES in CBC mode operation works on a 128-bit block with a 192 bit key.
    AES_256_CBC,  // AES in CBC mode operation works on a 128-bit block with a 256 bit key.
    AES_128_CFB,  // AES in CFB mode operation works on a 128-bit block with a 128 bit key.
    AES_192_CFB,  // AES in CFB mode operation works on a 128-bit block with a 192 bit key.
    AES_256_CFB,  // AES in CFB mode operation works on a 128-bit block with a 256 bit key.
    AES_128_CTR,  // AES in CTR mode operation works on a 128-bit block with a 128 bit key.
    AES_192_CTR,  // AES in CTR mode operation works on a 128-bit block with a 192 bit key.
    AES_256_CTR,  // AES in CTR mode operation works on a 128-bit block with a 256 bit key.
};

enum class HashType
{
    UNSPECIFIED,  // No argument found.
    SHA1,         // Message size: 2^64, Block size: 512, Word size: 32, Message digest size: 160.
    SHA224,       // Message size: 2^64, Block size: 512, Word size: 32, Message digest size: 224.
    SHA256,       // Message size: 2^64, Block size: 512, Word size: 32, Message digest size: 256.
    SHA384,       // Message size: 2^128, Block size: 1024, Word size: 64, Message digest size: 284.
    SHA512,       // Message size: 2^128, Block size: 1024, Word size: 64, Message digest size: 512.
    SHA512_224,   // Message size: 2^128, Block size: 1024, Word size: 64, Message digest size: 224.
    SHA512_256,   // Message size: 2^128, Block size: 1024, Word size: 64, Message digest size: 256.
    CRC,          // A CRC remainder with a programmable polynomial of up to 32 bits.
};

enum class SecureMacType
{
    UNSPECIFIED,  // No argument found.
    HMAC,         // Hashed Message Authentication Code.
    CMAC_AES_128, // Cipher-based Message Authentication Code in CBC. 128-bit
    CMAC_AES_192, // 192-bit
    CMAC_AES_256  // 256-bit
};

//void RenameSymbolsCmd(const CommandLine &cmd);
void CompleteElfFile(const CommandLine& cmd);
void DisplaySize(const CommandLine& cmd);
void MergeElfFiles(const CommandLine &cmd);
void SignElfFile(const CommandLine& cmd);
void GeneratePatchFile(const CommandLine& cmd);
void GenerateCodeShareFile(const CommandLine& cmd);

void PerformElfFileMerge(
    const std::wstring &primaryInput,
    const std::vector<std::wstring>::const_iterator inputBegin,
    const std::vector<std::wstring>::const_iterator inputEnd,
    const std::wstring &primaryOutput);

} // namespace cmd
} // namespace cymcuelftool

#endif
