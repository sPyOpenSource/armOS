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
#include "commandline.h"
#include "cmd/cyelfcmd.h"
#include "cymcuelfutil.h"
#include "version.h"
#include <iostream>
#ifdef _MSC_VER
#include <tchar.h>
#endif

using namespace cymcuelftool;
using namespace cymcuelftool::cmd;
using std::endl;
using std::wcout;
using std::wcerr;

#ifdef _MSC_VER
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char *argv[])
#endif
{
    const wchar_t *APPLICATION_NAME = L"cymcuelftool";
#ifdef ELFTOOL_VER
    const wchar_t *APPLICATION_VERSION = ELFTOOL_VER;
#else
    const wchar_t *APPLICATION_VERSION = L"1.0";
#endif

    CommandLine cmdLine;
    if (cmdLine.ReadArguments(argc, argv))
    {
        cmdLine.DisplayHelp();
        return 1;
    }

    switch (cmdLine.getAction())
    {
        case CommandLine::ActionType::VERSION:
        {
            cyelflib::DisplayVersion(APPLICATION_NAME, APPLICATION_VERSION);
            break;
        }
        case CommandLine::ActionType::SIZE:
        {
            DisplaySize(cmdLine);
            break;
        }
        case CommandLine::ActionType::MERGE:
        {
            MergeElfFiles(cmdLine);
            break;
        }
        case CommandLine::ActionType::SIGN:
        {
            SignElfFile(cmdLine);
            break;
        }
        case CommandLine::ActionType::PATCH:
        {
            GeneratePatchFile(cmdLine);
            break;
        }
        case CommandLine::ActionType::SHARE:
        {
            GenerateCodeShareFile(cmdLine);
            break;
        }
        case CommandLine::ActionType::UNSPECIFIED:
        case CommandLine::ActionType::HELP:
        default:
        {
            cmdLine.DisplayHelp();
            break;
        }
    }

    return 0;
}
