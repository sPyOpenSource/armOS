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
#include "utils.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using std::vector;
using std::string;
using std::wstring;
using std::wcout;
using std::wcerr;
using std::endl;
using cyelflib::CyErr;
using namespace cyelflib;

namespace cyelftool
{

CommandLine::CommandLine() :
    m_action(UNSPECIFIED),
    m_flashArraySize(0),
    m_flashSize(0),
    m_flashOffset(0),
    m_flashRowSize(0),
    m_eeRowSize(0),
    m_eeArraySize(0),
    m_eeArrayId(0),
    m_ignoreBytes(0),
    m_offset(0)
{
}

CommandLine::~CommandLine()
{
}

void CommandLine::DisplayHelp() const
{
    wcerr << L"Usage:\n\n"
        << L"Display help:\n"
        << L"   cyelftool -h/--help\n"
        << L"Display version information:\n"
        << L"   cyelftool -V/--version\n"
        << L"Display Flash/SRAM size information:\n"
        << L"   cyelftool -S/--size\n"
        << L"Update checksum and metadata:\n"
        << L"   cyelftool -C/--checksum file.elf --flash_size <num> --flash_row_size <num>\n"
        << L"             [--flash_offset <addr>]\n"
        << L"Update checksum, bootloader checksum, and bootloader size:\n"
        << L"   cyelftool -P file.elf --flash_size <num> --flash_row_size <num>\n"
        << L"             --size_var_name <name> --checksum_var_name <name> [--ignore <num>]\n"
        << L"Generate cybootloader.c and linker configuration file:\n"
        << L"   cyelftool -E/--extract_bootloader file.elf --flash_size <num>\n"
        << L"             --flash_row_size <num>\n"
        << L"Create bootloadable output file:\n"
        << L"   cyelftool -B/--build_bootloadable file.elf --flash_size <num>\n"
        << L"             --flash_array_size <num> --flash_row_size <num>\n"
        << L"             [--ee_row_size <num> --ee_array <num>]\n"
        << L"             [--offset]\n"
        << L"Create combined multi-application bootloader file:\n"
        << L"   cyelftool -M/--multiapp_merge app1.elf app2.elf output.hex\n"
        << L"             --flash_size <num> --flash_row_size <num>\n"
        << L"             [--ee_row_size <num> --ee_array <num>]\n"
        << L"Create LD include file with provides directives for selected symbols\n"
        << L"   cyelftool -R/--shared_code_provides file.elf symlist.txt output_file --toolchain <toolchain>\n"
        << L"   cyelftool -A/--rename_symbols symbolFile renameFile\n"
        << endl;
}

CyErr::CyErrStatus CommandLine::_ReadArguments(const vector<wstring> &argv)
{
    assert(m_action == UNSPECIFIED);

    if (argv.size() < 2)    // No arguments
        return CyErr::CyErrStatus::OK;

    const wstring &cmd = argv[1];
    CyErr::CyErrStatus ret = CyErr::CyErrStatus::OK;
    if (cmd == L"-h" || cmd == L"--help")
    {
        m_action = HELP;
        ret = CheckArgs(cmd, argv.size() - 2, 0);
    }
    else if (cmd == L"-V" || cmd == L"--version")
    {
        m_action = VERSION;
        ret = CheckArgs(cmd, argv.size() - 2, 0);
    }
    else if (cmd == L"-C" || cmd == L"--checksum")
    {
        m_action = CHECKSUM;
        ret = ReadFlashArgs(argv, (ArgFlags)(ARGF_FLASHSIZE | ARGF_ROWSIZE));
        if (CyErr::CyErrStatus::OK == ret)
            ret = CheckArgs(cmd, m_inputs.size(), 1);
    }
    else if (cmd == L"-N" || cmd == L"--nvl")
    {
        m_action = GET_NVL;
        CopyToInputs(argv, 2, argv.size() - 2, m_inputs);
        ret = CheckArgs(cmd, argv.size() - 2, 1);
    }
    else if (cmd == L"-S" || cmd == L"--size")
    {
        m_action = GET_SIZE;
        CopyToInputs(argv, 2, argv.size() - 2, m_inputs);
        ret = CheckArgs(cmd, argv.size() - 2, 1);
    }
    else if (cmd == L"-P" || cmd == L"--patch_bootloader")
    {
        m_action = PATCH_BOOTLOADER;
        ret = ReadFlashArgs(argv, (ArgFlags)(ARGF_FLASHSIZE | ARGF_ROWSIZE | ARGF_BLSYMNAMES));
        if (CyErr::CyErrStatus::OK == ret)
            ret = CheckArgs(cmd, m_inputs.size(), 1);
    }
    else if (cmd == L"-E" || cmd == L"--extract_bootloader")
    {
        m_action = EXTRACT_BOOTLOADER;
        ret = ReadFlashArgs(argv, (ArgFlags)(ARGF_FLASHSIZE | ARGF_ROWSIZE));
        if (CyErr::CyErrStatus::OK == ret)
            ret = CheckArgs(cmd, m_inputs.size(), 1);
    }
    else if (cmd == L"-B" || cmd == L"--build_bootloadable")
    {
        m_action = BUILD_LOADABLE;
        ret = ReadFlashArgs(argv, (ArgFlags)(ARGF_FLASHSIZE | ARGF_ROWSIZE | ARGF_ARRSIZE));
        if (CyErr::CyErrStatus::OK == ret)
            ret = CheckArgs(cmd, m_inputs.size(), 1);
    }
    else if (cmd == L"-M" || cmd == L"--multiapp_merge")
    {
        m_action = MERGE_LOADABLES;
        ret = ReadFlashArgs(argv, (ArgFlags)(ARGF_FLASHSIZE | ARGF_ROWSIZE));
        if (CyErr::CyErrStatus::OK == ret)
            ret = CheckArgs(cmd, m_inputs.size(), 3);
        if (CyErr::CyErrStatus::OK == ret)
        {
            m_output = m_inputs[2];
            m_inputs.pop_back();
        }
    }
    else if (cmd == L"-R" || cmd == L"--shared_code_provides")
    {
        m_action = GENERATE_PROVIDES;
        ret = ReadFlashArgs(argv, ARGF_TOOLCHAIN);
        if (CyErr::CyErrStatus::OK == ret)
            ret = CheckArgs(cmd, argv.size() - 2, 5); // elf, symlist, output
        if (CyErr::CyErrStatus::OK == ret)
        {
            CopyToInputs(argv, 2, 3, m_inputs);
            m_output = m_inputs[2];
        }
    }
    else if (cmd == L"-A" || cmd == L"--rename_symbols")
    {
        m_action = RENAME_SYMBOLS;
        ret = CheckArgs(cmd, argv.size() - 2, 2); // symlist, renamelist
        if (CyErr::CyErrStatus::OK == ret)
        {
            CopyToInputs(argv, 2, 2, m_inputs);
            m_output = m_inputs[0];
        }
    }
    else
    {
        wcerr << L"Unrecognized command: " << cmd << endl;
        m_action = HELP;
        ret = CyErr::CyErrStatus::FAIL;
    }

    return ret;
}

CyErr::CyErrStatus CommandLine::ReadFlashArgs(const vector<wstring> &argv, ArgFlags required/* = ARGF_FLASHSIZE | ARGF_ROWSIZE*/)
{
    static const wchar_t ARG_FLASH_SIZE[] = L"--flash_size";
    static const wchar_t ARG_FLASH_OFFSET[] = L"--flash_offset";
    static const wchar_t ARG_FLASHROW_SIZE[] = L"--flash_row_size";
    static const wchar_t ARG_FLASHARR_SIZE[] = L"--flash_array_size";
    static const wchar_t ARG_IGNORE_BYTES[] = L"--ignore";
    static const wchar_t ARG_BLSIZENAME[] = L"--size_var_name";
    static const wchar_t ARG_BLCHECKSUMNAME[] = L"--checksum_var_name";
    static const wchar_t ARG_EE_ROW_SIZE[] = L"--ee_row_size";
    static const wchar_t ARG_EE_ARRAY[] = L"--ee_array";
    static const wchar_t ARG_EE_ARRAY_SIZE[] = L"--ee_array_size";
    static const wchar_t ARG_OFFSET_BOOTLOADABLE[] = L"--offset";
    static const wchar_t ARG_EXCLUDE_FROM_SIZE[] = L"--exclude_from_size";
    static const wchar_t ARG_TOOLCHAIN[] = L"--toolchain";
    static const wchar_t ARG_PADDING[] = L"--padding";

    CyErr::CyErrStatus ret = CyErr::CyErrStatus::OK;
    bool gotFlashSize = false,
         gotFlashOffset = false,
         gotFlashRowSize = false,
         gotFlashArrSize = false,
         gotBlSizeName = false,
         gotBlChecksumName = false,
         gotEeRowSize = false,
         gotEeArray = false,
         gotEeArraySize = false,
         gotToolchain = false;

    // Start with first argument after command
    for (unsigned int i = 2; ret == CyErr::CyErrStatus::OK && i < argv.size(); i++)
    {
        if (argv[i].size() != 0 && argv[i][0] == L'-')
        {
            if (argv[i] == ARG_FLASHARR_SIZE)
            {
                m_flashArraySize = ReadUintArg(argv, i);
                i++;
                gotFlashArrSize = true;
            }
            else if (argv[i] == ARG_FLASH_OFFSET)
            {
                m_flashOffset = ReadUintArg(argv, i);
                i++;
                gotFlashOffset = true;
            }
            else if (argv[i] == ARG_FLASHROW_SIZE)
            {
                m_flashRowSize = ReadUintArg(argv, i);
                i++;
                gotFlashRowSize = true;
            }
            else if (argv[i] == ARG_FLASH_SIZE)
            {
                m_flashSize = ReadUintArg(argv, i);
                i++;
                gotFlashSize = true;
            }
            else if (argv[i] == ARG_IGNORE_BYTES)
            {
                m_ignoreBytes = ReadUintArg(argv, i);
                i++;
            }
            else if (argv[i] == ARG_BLSIZENAME)
            {
                m_varSizeName = wstr_to_str(ReadStringArg(argv, i));
                i++;
                gotBlSizeName = true;
            }
            else if (argv[i] == ARG_BLCHECKSUMNAME)
            {
                m_varChecksumName = wstr_to_str(ReadStringArg(argv, i));
                i++;
                gotBlChecksumName = true;
            }
            else if (argv[i] == ARG_EE_ROW_SIZE)
            {
                m_eeRowSize = ReadUintArg(argv, i);
                i++;
                gotEeRowSize = true;
            }
            else if (argv[i] == ARG_EE_ARRAY_SIZE)
            {
                m_eeArraySize = ReadUintArg(argv, i);
                i++;
                gotEeArraySize = true;
            }
            else if (argv[i] == ARG_EE_ARRAY)
            {
                m_eeArrayId = (uint8_t)ReadUintArg(argv, i);
                i++;
                gotEeArray = true;
            }
            else if (argv[i] == ARG_OFFSET_BOOTLOADABLE)
            {
                m_offset = true;
                i++;
            }
            else if (argv[i] == ARG_EXCLUDE_FROM_SIZE)
            {
                m_excludeFromSize.insert(wstr_to_str(ReadStringArg(argv, i)));
                i++;
            }
            else if (argv[i] == ARG_TOOLCHAIN)
            {
                m_toolchain = wstr_to_str(ReadStringArg(argv, i));
                i++;
                gotToolchain = true;
            }
            else if (argv[i] == ARG_PADDING)
            {
                m_fillValue = ReadUint8Arg(argv, i);
                i++;
            }
            else
            {
                wcerr << L"Unrecognized parameter: " << argv[i] << endl;
                ret = CyErr::CyErrStatus::FAIL;
            }
        }
        else
            m_inputs.push_back(argv[i]);
    }

    if ((required & ARGF_FLASHSIZE) && !gotFlashSize)
    {
        wcerr << ARG_FLASH_SIZE << " is required." << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    if ((required & ARGF_FLASHOFFSET) && !gotFlashOffset)
    {
        wcerr << ARG_FLASH_OFFSET << " is required." << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    if ((required & ARGF_ROWSIZE) && !gotFlashRowSize)
    {
        wcerr << ARG_FLASHROW_SIZE << " is required." << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    if ((required & ARGF_ARRSIZE) && !gotFlashArrSize)
    {
        wcerr << ARG_FLASHARR_SIZE << " is required." << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    if ((required & ARGF_BLSYMNAMES) && !gotBlSizeName)
    {
        wcerr << ARG_BLSIZENAME << " is required." << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    if ((required & ARGF_BLSYMNAMES) && !gotBlChecksumName)
    {
        wcerr << ARG_BLCHECKSUMNAME << " is required." << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    if ((required & ARGF_EEPROM) && !gotEeRowSize)
    {
        wcerr << ARG_EE_ROW_SIZE << " is required." << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    if ((required & ARGF_EEPROM) && !gotEeArraySize)
    {
        wcerr << ARG_EE_ARRAY_SIZE << " is required." << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    if ((required & ARGF_EEPROM) && !gotEeArray)
    {
        wcerr << ARG_EE_ARRAY << " is required." << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    if ((required & ARGF_TOOLCHAIN) && !gotToolchain)
    {
        wcerr << ARG_TOOLCHAIN << " is required." << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }

    return ret;
}

} // namespace cyelftool
