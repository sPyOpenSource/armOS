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

#include "stdafx.h"
#include "cyelfcmd.h"
#include "elf/cyelfutil.h"
#include "elf/elfxx.h"
#include "elf2hex.h"
#include "utils.h"
#include "commandline.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>

using std::ofstream;
using std::ifstream;
using std::string;
using std::vector;
using std::unordered_set;
using std::unordered_map;
using std::wstring;
using std::getline;
using std::endl;
using std::cout;
using cyelflib::elf::CyElfFile;
using namespace cyelflib;

namespace cymcuelftool {
namespace cmd {

enum class ToolchainType
{
    UNSPECIFIED,//!< No argument found.
    GCC,        //!< GNU GCC toolchain
    ARMCC,      //!< ARM ARMCC toolchain
    IAR,        //!< IAR toolchain
};

static CyErr::CyErrStatus ReadToolchain(const wstring &arg, ToolchainType &toolchain)
{
    CyErr::CyErrStatus ret = CyErr::CyErrStatus::OK;
    toolchain = ToolchainType::UNSPECIFIED;

    if (L"GCC" == arg)
        toolchain = ToolchainType::GCC;
    else if (L"ARMCC" == arg)
        toolchain = ToolchainType::ARMCC;
    else if (L"IAR" == arg)
        toolchain = ToolchainType::IAR;
    else
        ret = CyErr::CyErrStatus::FAIL;

    return ret;
}

void GenerateCodeShareFile(const CommandLine &cmd)
{
    //read elf file
    CyElfFile elf(cmd.getInput(0));
    CyErr err(elf.Read());
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"%ls: %ls", elf.Path().c_str(), err.Message().c_str());

    //read symbols
    ifstream symbolsListFile(wstr_to_str(cmd.getInput(1)));
    if (!symbolsListFile.good())
        errx(EXIT_FAILURE, L"Error opening file: %ls", cmd.getInput(1).c_str());

    //read toolchain
    ToolchainType toolchain;
    CyErr::CyErrStatus status = ReadToolchain(cmd.getInput(2), toolchain);
    if (CyErr::CyErrStatus::OK != status)
        errx(EXIT_FAILURE, L"Invalid toolchain: %ls", cmd.getInput(2).c_str());

    ofstream outputFile(wstr_to_str(cmd.PrimaryOutput()));
    if (!outputFile.good())
        errx(EXIT_FAILURE, L"Error opening file: %ls", cmd.PrimaryOutput().c_str());
    outputFile << std::hex;

    //Read all symbols to generate
    unordered_set<string> symbolsList;
    while (!symbolsListFile.eof() && !symbolsListFile.fail())
    {
        string symbolName;
        std::getline(symbolsListFile, symbolName);
        if (symbolName.length())
            symbolsList.insert(symbolName);
    }
    symbolsListFile.close();

    // Get symtab and strtab section data, cleanup and exit on fail
    wstring errmsg = L"";
    Elf_Scn  *scn = nullptr, *strTabScn = nullptr;
    Elf_Data *data = nullptr, *strData = nullptr;
    scn = elf.GetSection(".symtab");
    data = elf_getdata(scn, NULL);
    strTabScn = elf.GetSection(".strtab");
    strData = elf_getdata(strTabScn, NULL);
    if (!scn)
        errmsg += L" .symtab section not found.";
    if (!data)
        errmsg += L" .symtab section has no data.";
    if (!strTabScn)
        errmsg += L" .strtab section not found.";
    if (!strData)
        errmsg += L" .strtab section has no data.";

    if (!scn || !strTabScn || !data || !strData){
        outputFile.close();
        file_delete(cmd.PrimaryOutput().c_str());
        errx(EXIT_FAILURE, L"%ls: %ls", elf.Path().c_str(), errmsg.c_str());
    }

    int index = 1;
    while (elf.IsInDataRange(data, index))
    {
        GElf_Sym symbol;
        if (elf.GetSymbol(data, index++, &symbol))
        {
            string symbolName = elf.GetSymbolName(strData, &symbol);
            if (symbolsList.find(symbolName) != symbolsList.end())
            {
                if (ToolchainType::GCC == toolchain)
                {
                    outputFile <<
                        ".global " << symbolName << '\n' <<
                        ".equ " << symbolName << ", 0x" << symbol.st_value << "\n\n";
                }
                else if (ToolchainType::ARMCC == toolchain) // CDT 266527
                {
                    // MDK and IAR assembly lines that start with directives must start with whitespaces
                    outputFile << "        EXPORT " << symbolName << " [CODE]\n";
                    outputFile << symbolName << " EQU 0x" << symbol.st_value << "\n";
                }
                else if (ToolchainType::IAR == toolchain)
                {
                    // MDK and IAR assembly lines that start with directives must start with whitespaces
                    outputFile << "        PUBLIC " << symbolName << "\n";
                    outputFile << symbolName << " EQU 0x" << symbol.st_value << '\n';
                }
            }
        }
    }

    if (ToolchainType::ARMCC == toolchain || ToolchainType::IAR == toolchain) // CDT 266527
    {
        // MDK and IAR assembly lines that start with directives must start with whitespaces
        outputFile << "        END";
    }

    outputFile.close();
}

} // namespace cmd
} // namespace cymcuelftool
