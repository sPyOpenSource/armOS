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

#include <iostream>
#include <fstream>
#include "stdafx.h"
#include "cyelfcmd.h"
#include "elf/elfxx.h"
#include "elf/cyelfutil.h"
#include "commandline.h"
#include "cyopenssl.h"
#include "cymcuelfutil.h"

using cyelflib::elf::CyElfFile;
using std::vector;
using std::wstring;
using cyelflib::CyErr;
using namespace std;
using namespace cyelflib;

namespace cymcuelftool {
    namespace cmd {

        const uint64_t INVALID_OFFSET = 0xFFFFFFFFFFFFFFFF;

        CyErr PatchBootloaderMetadata(CyElfFile &elf)
        {
            CyErr err;
            GElf_Sym symbolBtldrMetaStart, symbolBtldrMetaLength;
            Elf_Scn *section = GetSectionEx(&elf, SEC_NAME_BTLDR_META);

            if (section) //Not all files have bootloader metadata section, if not present ignore
            {
                bool valid = elf.GetSymbol(SYM_NAME_BTLDR_META_START, &symbolBtldrMetaStart);
                valid &= elf.GetSymbol(SYM_NAME_BTLDR_META_LEN, &symbolBtldrMetaLength);

                if (!valid) {
                    return CyErr(CyErr::FAIL, L"ERROR patching bootloader metadata: ELF section %ls found, but one or both symbols %ls and %ls are missing", 
                        str_to_wstr(SEC_NAME_BTLDR_META).c_str(), str_to_wstr(SYM_NAME_BTLDR_META_START).c_str(), str_to_wstr(SYM_NAME_BTLDR_META_LEN).c_str());
                }

                uint32_t start = (uint32_t)symbolBtldrMetaStart.st_value;
                uint32_t length = (uint32_t)symbolBtldrMetaLength.st_value;

                Elf_Data *data = elf.GetFirstData(section);
                uint32_t checksumOffset = length - sizeof(uint32_t);

                if (data && data->d_buf && data->d_size >= length)
                {
                    uint32_t checksumVal;
                    CyErr err = elf.ReadData(section, checksumOffset, 4, &checksumVal);
                    if (err.IsOK())
                    {
                        uint8_t *dataPtr = (uint8_t *)(data->d_buf);
                        // CDT 267913
                        uint32_t checksum = Compute32bitCRC(&dataPtr[0], &dataPtr[std::min(data->d_size, (size_t)checksumOffset)]); // CRC-32C checksum of section
                        WriteWithEndian32(CyEndian::CYENDIAN_LITTLE, dataPtr + checksumOffset, checksum); // Checksum stored in last 4 bytes of same section
                        wcout << L"SUCCESS: calculated CRC-32C over ELF section " << SEC_NAME_BTLDR_META << " and stored" << endl;
                        elf.DirtyData(data);
                    }
                    else
                    {
                        uint32_t checksumAddr = start + length - sizeof(uint32_t);
                        return CyErr(CyErr::CyErrStatus::FAIL, L"ERROR: section %ls: Unable to access checksum memory at 0x%lx", 
                            str_to_wstr(SEC_NAME_BTLDR_META).c_str(), checksumAddr);
                    }
                }
                else
                {
                    uint32_t checksumAddr = start + length - sizeof(uint32_t);
                    return CyErr(CyErr::CyErrStatus::FAIL, L"No allocated block of memory to update with bootloader checksum at 0x%lx", checksumAddr);
                }
            }
            return CyErr();
        }

        CyErr SignAppStoreInSignatureSection(CyElfFile &elf, cmd::SecureMacType macType, cmd::HashType hash, cmd::CipherType cipher, const wstring &key, const wstring &initVec, uint8_t fillValue)
        {
            CyErr err;
            // Only sign application if hash given
            if (cmd::SecureMacType::UNSPECIFIED == macType
                && cmd::HashType::UNSPECIFIED == hash){
                if (elf.GetSection(SEC_NAME_APP_SIGNATURE)){
                    wcout << L"ELF section " << str_to_wstr(SEC_NAME_APP_SIGNATURE) 
                          << L" found, but no hash specified. Skipping application signature" << endl;
                }
                return err;
            }
            else if (!elf.GetSection(SEC_NAME_APP_SIGNATURE)){
                return err = CyErr(CyErr::FAIL, L"ERROR: A digital signature request was made, but the %s ELF section does not exist",
                    str_to_wstr(SEC_NAME_APP_SIGNATURE).c_str());
            }
            wcout << L"Found section " << str_to_wstr(SEC_NAME_APP_SIGNATURE) << L", attempting to sign application" << endl;

            vector<uint8_t> computedData;
            const wstring rawAppDataFile = elf.Path() + L".raw";

            GElf_Sym verifyStart, verifyLength;
            bool valid = elf.GetSymbol(SYM_NAME_APP_VERIFY_START, &verifyStart);
            valid &= elf.GetSymbol(SYM_NAME_APP_VERIFY_LEN, &verifyLength);

            uint64_t start = 0, end = 0;
            if (valid) //Not all files have bootloader meta section, if not present ignore 
            {
                start = verifyStart.st_value;
                end = start + verifyLength.st_value;
            }
            else
            {
                return CyErr(CyErr::FAIL, L"ERROR: attempting to sign application, but one or both  of '%ls' and '%ls' symbols are missing",
                    str_to_wstr(SYM_NAME_APP_VERIFY_START).c_str(), str_to_wstr(SYM_NAME_APP_VERIFY_LEN).c_str());
            }

            // Adjust Phdr table so each loadable section has its own phdr.
            // Occasionally, a phdr will contain more than one section such that one is in app_verify region
            // and the other is not. (see JIRA CYELFTOOL-9) This used to cause some app_verify segments to be ignored.
            // These two lines ensure all app_verify sections get included.
            err = AdjustHeaders(&elf, false, numeric_limits<unsigned int>::max(), true);
            elf.Write();

            //Get all program data
            uint32_t size = (uint32_t)(end - start);
            vector<uint8_t> data(size);
            std::fill(data.begin(), data.end(), fillValue);
            for (int i = 0; i < elf.PhdrCount(); i++)
            {
                const GElf_Phdr &phdr = elf.GetPhdr(i);

                if ((phdr.p_type == PT_LOAD || phdr.p_type == PT_ARM_EXIDX)
                    && phdr.p_paddr >= start
                    && phdr.p_paddr < end
                    && phdr.p_filesz != 0)
                {
                    uint32_t offset = (uint32_t)(phdr.p_paddr - start);
                    err = elf.ReadPhdrData(&phdr,
                        0,  // Offset
                        std::min((uint32_t)phdr.p_filesz, (uint32_t)(data.size() - offset)),    // Length
                        &data[offset]);
                    if (err.IsNotOK())
                        return err;
                }
            }

            //Store app data in temporary file
            ofstream rawData(wstr_to_str(rawAppDataFile).c_str(), ios_base::binary | ios_base::out);
            if (rawData.fail())
                errx(EXIT_FAILURE, L"%ls: Failed to open output file", rawAppDataFile.c_str());
            if (data.size() > 0){
                rawData.write((char*)&data[0], size);
            }
            rawData.close();

            // All hashes performed by openssl except CRC
            if (cmd::SecureMacType::UNSPECIFIED != macType){
                err = RunMac(macType, key, hash, cipher, rawAppDataFile, computedData);
            }
            else if (cmd::HashType::CRC != hash)
            {
                err = (cmd::CipherType::UNSPECIFIED != cipher)
                    ? RunSecureHash(hash, cipher, key, initVec, rawAppDataFile, computedData)
                    : RunHash(hash, rawAppDataFile, computedData);
            }
            else // using CRC hash
            {
                if (cmd::CipherType::UNSPECIFIED != cipher){
                    wcerr << L"WARNING: this tool does not currently support secure signing using CRC checksums" << endl
                          << L"         Signing application with un-encrypted CRC checksum" << endl;
                }
                uint32_t crc = Compute32bitCRC(data.begin(), data.end());
                computedData = ConvertToBytes(crc);
            }

            if (err.IsOK()) {
                err = StoreDataInSection(&elf, SEC_NAME_APP_SIGNATURE, computedData);
            }

            file_delete(rawAppDataFile.c_str());

            if (err.IsOK()){
                wcout << L"SUCCESS: application signature calculated and stored in section .cy_app_signature" << endl;
            }
            if (err.IsNotOK()){
                elf.Cleanup();
                file_delete(elf.Path().c_str());
            }

            return err;
        }

        CyErr ReadBinaryDataFromFile(const wstring &securityDataFile, vector<uint8_t> &data)
        {
            ifstream file(wstr_to_str(securityDataFile).c_str(), ios_base::binary | ios_base::in);
            if (!file.good())
                return CyErr(CyErr::FAIL, L"%ls: Failed to open input file", securityDataFile.c_str());

            //Get file size
            uint32_t fileSize;
            file.seekg(0, ios::end);
            fileSize = (uint32_t)file.tellg();
            file.seekg(0, ios::beg);

            data.resize(fileSize);
            file.read((char*)&data[0], fileSize);
            file.close();
            return CyErr();
        }

        // Adds hashes to TOC, bootloader metadata, and app_signature sections if present
        // Optionally creates secure (encrypted) signatures when passed a cipher and key.
        // Also adds checksum of flash to .cychecksum section (required for Cypress Programmer)
        void SignElfFile(const CommandLine &cmd)
        {
            const wstring outputFile = cmd.PrimaryOutput();

            CyErr err;
            GenerateInTempFile(outputFile, 
                [&err, &cmd](const std::wstring & tmpFile)
                {
                    // Generate intermediate copy of input file with .tmp extension
                    const wstring inputFile = cmd.PrimaryInput();
                    if (!file_copy(inputFile.c_str(), tmpFile.c_str())){
                        file_delete(tmpFile.c_str());
                        errx(EXIT_FAILURE, L"Failed to move %ls to %ls", inputFile.c_str(), tmpFile.c_str());
                    }

                    CyElfFile elf(tmpFile);
                    if ((err = elf.Read(true)).IsNotOK())
                        errx(EXIT_FAILURE, L"open of ELF file %ls failed", elf.Path().c_str());

                    // Sign bootloader meta
                    err = PatchBootloaderMetadata(elf);

                    // Sign Application
                    if (err.IsOK()){
                        err = SignAppStoreInSignatureSection(elf, cmd.getMacType(), cmd.getHash(), 
                            cmd.getCipher(), cmd.getCipherKey(), cmd.getInitVector(), cmd.getFillValue());
                    }
                    // Sign TOC
                    if (err.IsOK()){
                        err = PatchTableOfContents(&elf);
                    }

                    // Update Phdrs to match new section offsets
                    if (err.IsOK()){
                        err = AdjustHeaders(&elf);
                    }

                    // Update output file on disk
                    if (err.IsOK()){
                        elf.Write();
                    }

                    elf.Cleanup();

                    if (err.IsOK()){
                        // Sign elf. Populates .cymeta and .cychecksum section
                        err = PopulateSectionsCymetaCychecksum(tmpFile, cmd.getFillValue());
                    }

                    if (err.IsNotOK()){
                        file_delete(elf.Path().c_str());
                        errx(EXIT_FAILURE, L"%ls: %ls", tmpFile.c_str(), err.Message().c_str());
                    }
                }
            );

            if (err.IsNotOK())
                errx(EXIT_FAILURE, L"Failed to sign elf file: %ls", err.Message().c_str());
            else if (!cmd.HexFile().empty())
            {
                GenerateHexFile(outputFile, cmd.HexFile(), cmd.getFillValue());
            }
        }

    } // namespace cmd
} // namespace cymcuelftool
