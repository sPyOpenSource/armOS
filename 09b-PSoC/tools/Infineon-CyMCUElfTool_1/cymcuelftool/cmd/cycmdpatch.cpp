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
#include <cstring>
#include "stdafx.h"
#include "cyelfcmd.h"
#include "elf/cyelfutil.h"
#include "elf/elfxx.h"
#include "hex/cypsocacd2file.h"
#include "cyopenssl.h"
#include "elf2hex.h"
#include "utils.h"
#include "commandline.h"

using std::wstring;
using cyelflib::CyErr;
using cyelflib::elf::CyElfFile;
using cyelflib::hex::CyPSoCACD2File;
using cyelflib::hex::CyACD2FileLine;
using namespace std;
using namespace cyelflib;

namespace cymcuelftool {
namespace cmd {

static CyErr EncryptAcd(const CommandLine &cmd, const CyPSoCACD2File *acdRaw, CyPSoCACD2File *& acdEnc, const wstring &acdFileName)
{
    const wstring acdFileNameRaw = acdFileName + L".raw";
    const wstring acdFileNameEnc = acdFileName + L".enc";

#ifdef _MSC_VER
    ofstream rawData(acdFileNameRaw.c_str(), ios_base::binary | ios_base::out);
#else
    ofstream rawData(wstr_to_str(acdFileNameRaw).c_str(), ios_base::binary | ios_base::out);
#endif
    if (!rawData.good())
        return CyErr(CyErr::FAIL, L"%ls: Failed to open output file", acdFileName.c_str());

    for (uint32_t i = 0; i < acdRaw->Lines().size(); i++)
    {
        const std::unique_ptr<CyACD2FileLine> &line = acdRaw->Lines()[i];
        rawData.write((char*)&line->Data()[0], line->Data().size());
    }
    rawData.close();

    CyErr err = RunEncrypt(cmd.getCipher(), acdFileNameRaw, acdFileNameEnc, cmd.getCipherKey(), cmd.getInitVector());

#ifdef _MSC_VER
    ifstream encData(acdFileNameEnc.c_str(), ios_base::binary | ios_base::in);
#else
    ifstream encData(wstr_to_str(acdFileNameEnc).c_str(), ios_base::binary | ios_base::in);
#endif
    if (encData.good())
    {
        vector<std::unique_ptr<CyACD2FileLine>> lines;
        for (uint32_t i = 0; i < acdRaw->Lines().size(); i++)
        {
            const std::unique_ptr<CyACD2FileLine> &line = acdRaw->Lines()[i];
            vector<uint8_t> rowData(line->Data().size());
            encData.read((char*)&rowData[0], rowData.size());
            lines.push_back(std::make_unique<CyACD2FileLine>(line->Address(), rowData));
        }
        encData.close();

        acdEnc = new CyPSoCACD2File(
            acdRaw->Version(), acdRaw->DeviceID(), acdRaw->DeviceRev(), acdRaw->CheckSumType(), 
            acdRaw->AppId(), acdRaw->ProductId(), std::move(lines), cmd.getInitVector(), acdRaw->AppInfo());
    }
    else
    {
        err = CyErr(CyErr::FAIL, L"%ls: Failed to open input file", acdFileName.c_str());
    }
    file_delete(acdFileNameRaw.c_str()); // CDT fix 263925
    file_delete(acdFileNameEnc.c_str()); // CDT fix 263925

    return CyErr();
}

void GeneratePatchFile(const CommandLine &cmd)
{
    CyElfFile elf(cmd.getInput(0));
    CyErr err(elf.Read());

    CyPSoCACD2File *acd = NULL;
    wstring acdFileName = cmd.PrimaryOutput();
    if (err.IsOK()){
        err = elf2acd(elf, cmd.getFillValue(), cmd.getInitVector(), acd);
    }
    if (err.IsOK())
    {
        if (cmd::CipherType::UNSPECIFIED != cmd.getCipher())
        {
            CyPSoCACD2File *acdEnc = NULL;
            err = EncryptAcd(cmd, acd, acdEnc, acdFileName);
            delete acd;
            acd = acdEnc;
        }

        if (err.IsOK())
        {
            // Generate CyAcd2 file
#ifdef _MSC_VER
            std::ofstream outFile(acdFileName.c_str());
#else
            std::ofstream outFile(wstr_to_str(acdFileName).c_str());
#endif
            if (outFile.fail()){
                delete acd;
                file_delete(acdFileName.c_str());
                errx(EXIT_FAILURE, L"%ls: Failed to open output file", acdFileName.c_str());
            }

            err = acd->Write(outFile);
            outFile.close();
        }
    }

    delete acd;

    if (err.IsNotOK())
    {
        file_delete(acdFileName.c_str());
        errx(EXIT_FAILURE, L"Failed to generate %ls: %ls", acdFileName.c_str(), err.Message().c_str());
    }
}

} // namespace cmd
} // namespace cymcuelftool
