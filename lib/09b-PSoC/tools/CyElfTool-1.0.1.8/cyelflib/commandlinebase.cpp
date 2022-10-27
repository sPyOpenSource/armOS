/*
 *  CyElfLib, a library to facilitate ELF file post-processing
 *  Copyright (C) 2013-2017 - Cypress Semiconductor Corp.
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
#include "commandlinebase.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using std::vector;
using std::string;
using std::wstring;
using std::wcout;
using std::wcerr;
using std::endl;

namespace cyelflib
{

const std::wstring CommandLineBase::EmptyString;

void CommandLineBase::CopyToInputs(const vector<wstring> &src, int start, int count, vector<wstring> &dest)
{
    dest.resize(count);
    std::copy(src.begin() + start, src.begin() + start + count, dest.begin());
    CheckForDuplicateInputs();
}

const void CommandLineBase::CheckForDuplicateInputs()
{
    for (vector<wstring>::iterator it1 = m_inputs.begin(); it1 < m_inputs.end(); ++it1)
    {
        for (vector<wstring>::iterator it2 = it1 + 1; it2 < m_inputs.end(); ++it2)
        {
            if (*it1 == *it2){
                errx(EXIT_FAILURE, L"Detected duplicate input file: %ls", (*it1).c_str());
            }
        }
    }
}

CyErr::CyErrStatus CommandLineBase::CheckArgs(const std::wstring &arg, int actual, int expected)
{
    if (actual != expected)
    {
        wcerr << L"Expected " << expected << " argument(s) for " << arg << " but got " << actual << endl;
        return CyErr::CyErrStatus::FAIL;
    }
    return CyErr::CyErrStatus::OK;
}

uint8_t CommandLineBase::ReadUint8Arg(const vector<wstring> &argv, unsigned int prevIdx)
{
    if (prevIdx + 1 >= argv.size())
        errx(EXIT_FAILURE, L"%ls requires an argument", argv[prevIdx].c_str());
    const wchar_t *str = argv[prevIdx + 1].c_str();
    return str_to_uint8(str);
}

uint32_t CommandLineBase::ReadUintArg(const vector<wstring> &argv, unsigned int prevIdx)
{
    if (prevIdx + 1 >= argv.size())
        errx(EXIT_FAILURE, L"%ls requires an argument", argv[prevIdx].c_str());
    const wchar_t *str = argv[prevIdx + 1].c_str();
    return str_to_uint32(str);
}

const wstring &CommandLineBase::ReadStringArg(const vector<wstring> &argv, unsigned int prevIdx)
{
    if (prevIdx + 1 >= argv.size())
        errx(EXIT_FAILURE, L"%ls requires an argument", argv[prevIdx].c_str());
    return argv[prevIdx + 1];
}

int CommandLineBase::ReadArguments(int argc, char *argv[])
{
    vector<wstring> argList;
    argList.reserve(argc);
    for (int i = 0; i < argc; i++)
        argList.push_back(str_to_wstr(argv[i]));
    return _ReadArguments(argList);
}

int CommandLineBase::ReadArguments(int argc, wchar_t *argv[])
{
    vector<wstring> argList;
    argList.reserve(argc);
    for (int i = 0; i < argc; i++)
        argList.push_back(wstring(argv[i]));
    return _ReadArguments(argList);
}

} // namespace cyelflib
