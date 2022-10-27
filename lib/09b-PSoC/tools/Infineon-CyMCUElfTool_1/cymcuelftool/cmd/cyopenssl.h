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

#ifndef INCLUDED_CYOPENSSL_H
#define INCLUDED_CYOPENSSL_H

#include <string>
#include "cyerr.h"
#include "cmd/cyelfcmd.h"

namespace cymcuelftool
{
    namespace cmd
    {
        cyelflib::CyErr RunEncrypt(CipherType cipher, const std::wstring &srcFile, const std::wstring &dstFile, const std::wstring &key, const std::wstring &initVec);
        cyelflib::CyErr RunEncrypt(CipherType cipher, HashType hash, const std::wstring &srcFile, const std::wstring &dstFile, const std::wstring &key, const std::wstring &initVec);
        cyelflib::CyErr RunHash(HashType hash, const std::wstring &file, std::vector<uint8_t> &computedHash);
        cyelflib::CyErr RunSecureHash(HashType hash, CipherType cipher, const std::wstring &key, const std::wstring &initVec, const std::wstring &file, std::vector<uint8_t> &computedHash);
        cyelflib::CyErr RunMac(SecureMacType mac, const std::wstring &footerKey, HashType hash, CipherType cipher, const std::wstring &file, std::vector<uint8_t> &computedMac);

    } // namespace cmd
} // namespace cymcuelftool

#endif
