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
using std::getline;
using std::endl;
using std::cout;
using cyelflib::elf::CyElfFile;
using namespace cyelflib;

namespace cyelftool {
namespace cmd {

void GenerateCodeshareProvidesCmd(const CommandLine &cmd)
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

    ofstream outputFile(wstr_to_str(cmd.PrimaryOutput()));
    if (!outputFile.good())
        errx(EXIT_FAILURE, L"Error opening file: %ls", cmd.PrimaryOutput().c_str());
    outputFile << std::hex;

    string toolchain = cmd.Toolchain();
    toolchain = toolchain.substr(0, toolchain.find_last_not_of('\0') + 1);
    
    if (toolchain.compare("gcc") != 0 && toolchain.compare("mdk") != 0)
        errx(EXIT_FAILURE, L"Error toolcahin not supported: %ld", toolchain.c_str());

    unordered_set<string> symbolsList;
    string symbolName;
    while (symbolsListFile >> symbolName)
    {
        symbolsList.insert(symbolName);
    }
    symbolsListFile.close();

    int index = 1;

    Elf_Scn *scn = elf.GetSection(".symtab");
    if (!scn)
        return;
    Elf_Data *data = elf_getdata(scn, NULL);
    if (!data)
        return;
    Elf_Scn *strTabScn = elf.GetSection(".strtab");
    if (!strTabScn)
        return;
    Elf_Data *strData = elf_getdata(strTabScn, NULL);
    if (!strData)
        return;

    while (symbolsList.size() != 0)
    {
        GElf_Sym symbol;
        if (elf.GetSymbol(data, index++, &symbol))
        {
            symbolName = elf.GetSymbolName(strData, &symbol);
            if (symbolsList.find(symbolName) != symbolsList.end())
            {
                if (toolchain.compare("gcc") == 0)
                {
                    outputFile << "PROVIDE(" << symbolName << " = 0x" << symbol.st_value << ");" << endl;
                }
                if (toolchain.compare("mdk") == 0)
                {
                    outputFile << "#define " << symbolName << " 0x" << symbol.st_value << endl;
                }
                symbolsList.erase(symbolName);
            }
        }
        if (!elf.IsInDataRange(data, index))
            break;
    }
    
    outputFile.close();
}

void RenameSymbolsCmd(const CommandLine &cmd)
{
    std::wstring symbolFile = cmd.getInput(0);
    ifstream symList(wstr_to_str(symbolFile));
    if (!symList.good())
        errx(EXIT_FAILURE, L"Error opening file: %ls", symbolFile.c_str());

    ifstream renameList(wstr_to_str(cmd.getInput(1)));
    if (!renameList.good())
        errx(EXIT_FAILURE, L"Error opening file: %ls", cmd.getInput(1).c_str());

    string line;
    string newName, oldName;
    vector<string> symbols = vector<string>();
    unordered_map<string, string> renames;

    //get the list of all the renames that need to occur
    while (renameList >> newName && renameList >> oldName)
    {
        renames[oldName] = newName;
    }
    renameList.close();
    
    std::wstring ext = symbolFile.substr(symbolFile.find_last_of('.') + 1);

    while (getline(symList, line))
    {
        string symbol;
        
        if (ext.compare(L"obj") == 0)
        {
            //assume symdef file
            int index = line.find_last_of(' ');
            symbol = line.substr(index + 1);
            //# = comment
            if (line.at(0) != '#' && renames.find(symbol) != renames.end())
            {
                symbols.push_back(line.substr(0, index + 1) + renames[symbol]);
            }
            else
            {
                symbols.push_back(line);
            }
        }
        else if (ext.compare(L"ld") == 0)
        {
            //assume gcc linker script
            int indexLeft = line.find_first_of('(');
            int indexRight = line.find_last_of('=');
            symbol = line.substr(indexLeft + 1, indexRight - indexLeft - 2);
            if (renames.find(symbol) != renames.end())
            {
                symbols.push_back(line.substr(0, indexLeft + 1) + renames[symbol] + " " + line.substr(indexRight));
            }
            else
            {
                symbols.push_back(line);
            }
        }
        else if (ext.compare(L"scat") == 0)
        {
            //assume header file
            int indexLeft = line.find_first_of(' ');
            int indexRight = line.find_last_of(' ');
            symbol = line.substr(indexLeft + 1, indexRight - indexLeft - 1);
            if (renames.find(symbol) != renames.end())
            {
                symbols.push_back(line.substr(0, indexLeft + 1) + renames[symbol] + " " + line.substr(indexRight));
            }
            else
            {
                symbols.push_back(line);
            }
        }
        else
        {
            //unexpected type
            assert(false);
        }
        
    }
    symList.close();

    //we are overwritting a file so we cannot open our output till symList is closed
    ofstream output(wstr_to_str(cmd.PrimaryOutput()));
    if (!output.good())
        errx(EXIT_FAILURE, L"Error opening file: %ls", cmd.PrimaryOutput().c_str());

    for (unsigned int i = 0; i < symbols.size(); i++)
    {
        line = symbols[i];
        output << line << endl;
    }
    output.close();
}



} // namespace cmd
} // namespace cyelftool