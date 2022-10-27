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
#include "version.h"
#include "cmd/cyelfcmd.h"
#include <iostream>
#ifdef _MSC_VER
#include <tchar.h>
#endif

using namespace cyelftool;
using namespace cyelftool::cmd;
using std::endl;
using std::wcout;
using std::wcerr;

static const wchar_t *APPLICATION_NAME = L"cyelftool";

#ifdef _MSC_VER
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char *argv[])
#endif
{
    CommandLine cmdLine;
    if (cmdLine.ReadArguments(argc, argv))
    {
        cmdLine.DisplayHelp();
        return 1;
    }

    switch (cmdLine.getAction())
    {
        case CommandLine::CHECKSUM:
        {
            ChecksumCommand(cmdLine);
            break;
        }
        case CommandLine::GET_NVL:
        {
            DisplayNvlValues(cmdLine.PrimaryInput());
            break;
        }
        case CommandLine::GET_SIZE:
        {
            DisplaySize(cmdLine);
            break;
        }
        case CommandLine::PATCH_BOOTLOADER:
        {
            BuildBootloaderCommand(cmdLine);
            break;
        }
        case CommandLine::EXTRACT_BOOTLOADER:
        {
            ExtractBootloaderCommand(cmdLine);
            break;
        }
        case CommandLine::BUILD_LOADABLE:
        {
            BuiltBootloadableCommand(cmdLine);
            break;
        }
        case CommandLine::MERGE_LOADABLES:
        {
            MultiAppMergeCommand(cmdLine);
            break;
        }
        case CommandLine::GENERATE_PROVIDES:
        {
            GenerateCodeshareProvidesCmd(cmdLine);
            break;
        }
        case CommandLine::RENAME_SYMBOLS:
        {
            RenameSymbolsCmd(cmdLine);
            break;
        }
        case CommandLine::VERSION:
        {
            cyelflib::DisplayVersion(APPLICATION_NAME, APPLICATION_VERSION);
            break;
        }
        case CommandLine::UNSPECIFIED:
        case CommandLine::HELP:
        default:
        {
            cmdLine.DisplayHelp();
            break;
        }
    }

    return 0;
}
