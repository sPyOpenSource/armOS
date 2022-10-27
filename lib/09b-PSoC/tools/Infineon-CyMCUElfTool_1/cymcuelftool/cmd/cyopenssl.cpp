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

#ifdef _MSC_VER
// required for ::CreateProcess()
#include <Windows.h>

#else
// required for fork() exec() on unix systems
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#include <fstream>
#include <string>
#include "cyopenssl.h"
#include "utils.h"

using namespace cyelflib;
using cyelflib::CyErr;
using std::vector;
using std::wstring;
using std::ifstream;

namespace cymcuelftool
{
    namespace cmd
    {
        static const wstring GetHashArg(HashType hash)
        {
            switch (hash)
            {
            case cymcuelftool::cmd::HashType::SHA1:
                return L"sha1";
            case cymcuelftool::cmd::HashType::SHA224:
                return L"sha224";
            case cymcuelftool::cmd::HashType::SHA256:
                return L"sha256";
            case cymcuelftool::cmd::HashType::SHA384:
                return L"sha384";
            case cymcuelftool::cmd::HashType::SHA512:
                return L"sha512";
            case cymcuelftool::cmd::HashType::CRC:
                errx(EXIT_FAILURE, L"Cannot run HMAC with CRC");
                return L"";
            default:
                errx(EXIT_FAILURE, L"No hash type specified");
                return L"";
            }
        }

        // TODO: remove hash arg? b/c we've already hashed input at this point
        static const wstring GetCipherArg(CipherType cipher, HashType hash)
        {
            switch (cipher)
            {
                /* Public-key Ciphers */
                case cymcuelftool::cmd::CipherType::RSAES_PKCS:
                    return L"rsautl -encrypt -pkcs";
                    //return (HashType::UNSPECIFIED == hash)
                    //    ? L"rsautl -encrypt -pkcs"
                    //    : L"pkeyutl -encrypt -pkeyopt rsa_padding_mode:pkcs1 -pkeyopt digest:" + GetHashArg(hash);
                case cymcuelftool::cmd::CipherType::RSASSA_PKCS:
                    return (HashType::UNSPECIFIED == hash)
                        ? L"rsautl -sign -pkcs"
                        : L"pkeyutl -sign -pkeyopt rsa_padding_mode:pkcs1 -pkeyopt digest:" + GetHashArg(hash);

                /* Symmetric-key Ciphers */
                case cymcuelftool::cmd::CipherType::DES_ECB:
                    return L"des-ecb";
                case cymcuelftool::cmd::CipherType::TDES_ECB:
                    return L"des-ede3";
                case cymcuelftool::cmd::CipherType::AES_128_ECB:
                    return L"aes-128-ecb";
                case cymcuelftool::cmd::CipherType::AES_192_ECB:
                    return L"aes-192-ecb";
                case cymcuelftool::cmd::CipherType::AES_256_ECB:
                    return L"aes-256-ecb";
                case cymcuelftool::cmd::CipherType::AES_128_CBC:
                    return L"aes-128-cbc";
                case cymcuelftool::cmd::CipherType::AES_192_CBC:
                    return L"aes-192-cbc";
                case cymcuelftool::cmd::CipherType::AES_256_CBC:
                    return L"aes-256-cbc";
                case cymcuelftool::cmd::CipherType::AES_128_CFB:
                    return L"aes-128-cfb";
                case cymcuelftool::cmd::CipherType::AES_192_CFB:
                    return L"aes-192-cfb";
                case cymcuelftool::cmd::CipherType::AES_256_CFB:
                    return L"aes-256-cfb";
                default:
                    errx(EXIT_FAILURE, L"No encryption type specified");
                    return L"";
            }
        }

        static CyErr RunOpenSSL(const std::wstring &command)
        {
            std::wcout << command << std::endl;
#ifdef _MSC_VER
            PROCESS_INFORMATION processInfo;
            STARTUPINFO startupInfo;
            ::ZeroMemory(&startupInfo, sizeof(startupInfo));
            startupInfo.cb = sizeof(startupInfo);
            if (::CreateProcess(NULL,
                (LPWSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL,
                NULL, &startupInfo, &processInfo))
            {
                DWORD exitCode;
                WaitForSingleObject(processInfo.hProcess, INFINITE);
                GetExitCodeProcess(processInfo.hProcess, &exitCode);
                if (0 != exitCode){
                    return CyErr(CyErr::FAIL, L"ERROR: OpenSSL failed with exit code %d", exitCode);
                }
            }
            else
            {
                return CyErr("  openssl error: " + GetLastError());
            }
#else
            uint32_t ret;
            ret = system(wstr_to_str(command).c_str()); 
            if(ret != 0){
                return CyErr(CyErr::FAIL, L"ERROR: OpenSSL failed with exit code: %d", ret);
            }
#endif
            return CyErr();
        }

        CyErr ParseBinFromFileAndDelete(vector<uint8_t> &computed, const wstring &binFile)
        {
            ifstream inFile(wstr_to_str(binFile).c_str(), std::ios_base::binary | std::ios_base::in);
            if (!inFile.good())
                return CyErr(CyErr::FAIL, L"%ls: Failed to open input file", binFile.c_str());

            //Get file size
            uint32_t fileSize;
            inFile.seekg(0, std::ios::end);
            fileSize = (uint32_t)inFile.tellg();
            inFile.seekg(0, std::ios::beg);

            // Transfer computed binary byte-wise to vector
            char * buffer = new char[fileSize];
            inFile.read(buffer, fileSize);
            for (uint32_t i = 0; i < fileSize; ++i) // CDT 266930
                computed.push_back((uint8_t)buffer[i]);
            inFile.close();
            file_delete(binFile.c_str());
            delete[] buffer;
            buffer = nullptr;
            return CyErr();
        }

        CyErr RunEncrypt(CipherType cipher, const wstring &srcFile, const wstring &dstFile, const wstring &key, const wstring &initVec)
        {
            return RunEncrypt(cipher, HashType::UNSPECIFIED, srcFile, dstFile, key, initVec);
        }

        CyErr RunEncrypt(CipherType cipher, HashType hash, const wstring &srcFile, const wstring &dstFile, const wstring &key, const wstring &initVec)
        {
            const wstring cipherType = GetCipherArg(cipher, hash);
            // initialization vector, used for certain ciphers
            const wstring argIV = (initVec == L"") ? initVec : (L" -iv " + initVec);
            wstring cmdLine;

            if (cipherType == L"ec"){
                return CyErr(CyErr::FAIL, L"%ls is not supported at this time", cipherType.c_str());
            }
            // Using RSA
            else if (cipherType.find(L"rsautl") != std::string::npos
                  || cipherType.find(L"pkeyutl") != std::string::npos){
                cmdLine = L"openssl " + cipherType + L" -inkey \"" + key + L"\" -in \"" + srcFile + L"\" -out \"" + dstFile + L"\"";
            }
            // Using non-RSA
            else{
                cmdLine = L"openssl enc -e -nosalt -" + cipherType + argIV + 
                          L" -in \"" + srcFile + L"\" -out \"" + dstFile + L"\" -K " + key; // do not quote key here, since it is explicit hex, not a file.
            }

            return RunOpenSSL(cmdLine);
        }

        CyErr RunHash_pvt(HashType hash, const wstring &file, wstring &outFile)
        {
            outFile = file + L".hash";
            const wstring cmdLine = L"openssl dgst -" + GetHashArg(hash) + L" -binary -out \"" + outFile + L"\" \"" + file + L"\"";

            return RunOpenSSL(cmdLine);
        }

        CyErr RunHash(HashType hash, const wstring &file, vector<uint8_t> &computedHash)
        {
            wstring outFile;
            CyErr err = RunHash_pvt(hash, file, outFile);
            if (err.IsOK()){
                err = ParseBinFromFileAndDelete(computedHash, outFile);
            }
            return err;
        }

        CyErr RunSecureHash(HashType hash, CipherType cipher, const std::wstring &key, const std::wstring &initVec, const wstring &file, vector<uint8_t> &computedHash)
        {
            const wstring encFile = file + L".enc";
            wstring hashFile;

            CyErr err = RunHash_pvt(hash, file, hashFile);

            if (err.IsOK())
            {
                err = RunEncrypt(cipher, hash, hashFile, encFile, key, initVec);
                file_delete(hashFile.c_str());
            }

            if (err.IsOK()){
                err = ParseBinFromFileAndDelete(computedHash, encFile);
            }

            return err;
        }

        CyErr RunMac(SecureMacType mac, const wstring &key, HashType hash, CipherType cipher, const wstring &file, vector<uint8_t> &computedMac)
        {
            const wstring outFile = file + L".mac";

            wstring cmdLine = (cmd::SecureMacType::HMAC == mac)
                ? L"openssl dgst -mac hmac -" + GetHashArg(hash) + L" -macopt hexkey:" + key + L" -binary -out " + outFile + L" " + file
                : L"openssl dgst -mac cmac -macopt cipher:" + GetCipherArg(cipher, HashType::UNSPECIFIED) + L" -macopt hexkey:" + key + L" -binary -out " + outFile + L" " + file;

            CyErr err = RunOpenSSL(cmdLine);
            err = (err.IsNotOK())
                ? CyErr(CyErr::FAIL, L"%ls: Failed to run openssl MAC", err.Message().c_str())
                : ParseBinFromFileAndDelete(computedMac, outFile);
            return err;
        }
    }
}
