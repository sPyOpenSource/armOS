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
#include "utils.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using std::vector;
using std::string;
using std::wstring;
using std::ifstream;
using std::wcout;
using std::wcerr;
using std::endl;
using cyelflib::CyErr;
using namespace cyelflib;

namespace cymcuelftool
{

CommandLine::CommandLine() :
    m_action(ActionType::UNSPECIFIED),
    m_hash(cmd::HashType::UNSPECIFIED),
    m_cipher(cmd::CipherType::UNSPECIFIED),
    m_cipherKey(L""),
    m_initVector(L""),
    m_hexFile(L""),
    m_macType(cmd::SecureMacType::UNSPECIFIED),
    m_requiredArgs(ArgFlags::ARGF_NONE),
    m_supportedArgs(ArgFlags::ARGF_NONE)
{
}

CommandLine::~CommandLine()
{
}

void CommandLine::DisplayHelp() const
{
    wcout <<
        L"Usage:\n\n"
        L"Display help:\n"
        L"   cymcuelftool -h/--help\n"
        L"Display version information:\n"
        L"   cymcuelftool -v/--version\n"
        L"Display memory allocation by type:\n"
        L"   cymcuelftool -A/--allocation file.elf\n"
        L"Merge ELF files:\n"
        L"   cymcuelftool -M/--merge complete_app1.elf complete_app2.elf ... [--output merged.elf] [--hex merged.hex]\n"
        L"Sign ELF file, with option for secure (encrypted) signature:\n"
        L"   cymcuelftool -S/--sign unsigned.elf [<SignScheme>] [--output signed.elf] [--hex signed.hex]\n"
        L"Generate Patch file:\n"
        L"   cymcuelftool -P/--patch file.elf [--encrypt <Cipher*> --key key.txt [--iv iv.txt]] [--output patch.cyacd2]\n"
        L"   *NOTE: RSAES-PKCS and RSASSA-PKCS not allowed for this command\n"
        L"Create Code sharing file:\n"
        L"   cymcuelftool -R/--codeshare file.elf symbols.txt <GCC/ARMCC/IAR> [--output shared.s]\n"
        L"\n\n"
        L"<SignScheme> is only used for signing the user application. It must be ONE of:\n"
        L"   1) HMAC <Hash*>  --key key.txt              (*CRC not supported)\n"
        L"   2) CMAC-AES-XXX* --key key.txt              (*XXX can be 128, 192, or 256)\n"
        L"   3) <Hash> [--encrypt <Cipher> --key key.txt [--iv iv.txt]]\n"
        L"\n"
        L"<Hash>: CRC, SHA1, SHA224, SHA256, SHA384, SHA512\n"
        L"\n"
        L"<Cipher> (requires key): \n"
        L"   Public-key: RSAES-PKCS, RSASSA-PKCS\n"
        L"   Symmetric:  DES-ECB, TDES-ECB, AES-{128|192|256}-{ECB|CBC|CFB}\n"
        L"\n"
        L"key.txt: ASCII text file containing key appropriate for chosen Cipher. May be symmetric hex key or PEM format for RSA cipher variants\n"
        L"iv.txt:  ASCII text file containing initialization vector for certain encryption algorithms"

        << endl
        << endl;
}

inline CommandLine::ArgFlags& operator|=(CommandLine::ArgFlags & myFlags, const CommandLine::ArgFlags & newFlag){
    myFlags = (CommandLine::ArgFlags)(myFlags | newFlag);
    return myFlags;
}

CyErr::CyErrStatus CommandLine::_ReadArguments(const vector<wstring> &argv)
{
    assert(m_action == ActionType::UNSPECIFIED);

    if (argv.size() < 2)    // No arguments
        return CyErr::CyErrStatus::OK;

    const wstring &cmd = argv[1];
    CyErr::CyErrStatus ret = CyErr::CyErrStatus::OK;
    uint32_t nextArgIdx = FindNextArgIdx(argv, 2);

    if (cmd == L"-h" || cmd == L"-H" || cmd == L"--help")
    {
        m_action = ActionType::HELP;
        ret = CheckArgs(cmd, argv.size() - 2, 0);
    }
    else if (cmd == L"-v" || cmd == L"-V" || cmd == L"--version")
    {
        m_action = ActionType::VERSION;
        ret = CheckArgs(cmd, argv.size() - 2, 0);
    }
    else if (cmd == L"-A" || cmd == L"--allocated")
    {
        m_action = ActionType::SIZE;
        ret = CheckArgs(cmd, argv.size() - 2, 1);
    }
   else if (cmd == L"-M" || cmd == L"--merge")
    {
        if (nextArgIdx < 4)
        {
            wcerr << L"Expected at least 2 elf file arguments for -M/--merge" << endl;
            m_action = ActionType::HELP;
            ret = CyErr::CyErrStatus::FAIL;
        }
        else
        {
            m_output = argv[2];
            CopyToInputs(argv, 2, nextArgIdx - 2, m_inputs);
            m_supportedArgs = (ArgFlags)(ArgFlags::ARGF_OUTPUT | ArgFlags::ARGF_HEXFILE);
            m_action = ActionType::MERGE;
        }
    }
    else if (cmd == L"-S" || cmd == L"--sign")
    {
        if (nextArgIdx < 3)
        {
            wcerr << L"Expected an elf file as input for -S/--sign" << endl;
            m_action = ActionType::HELP;
            ret = CyErr::CyErrStatus::FAIL;
        }
        else
        {
            m_supportedArgs = (ArgFlags)(ArgFlags::ARGF_OUTPUT  | ArgFlags::ARGF_HEXFILE | ArgFlags::ARGF_INITVEC
                                       | ArgFlags::ARGF_ENCRYPT | ArgFlags::ARGF_KEYDATA);
            m_output = argv[2];
            if (argv.size() > 3){
                ret = ReadOptionalHashAlgorithm(argv, 3);
            }
            CopyToInputs(argv, 2, 1, m_inputs);
            m_action = ActionType::SIGN;
        }
    }
    else if (cmd == L"-P" || cmd == L"--patch")
    {
        if (nextArgIdx < 3)
        {
            wcerr << L"Expected at least 1 elf file argument for -P/--patch" << endl;
            m_action = ActionType::HELP;
            ret = CyErr::CyErrStatus::FAIL;
        }
        else
        {
            m_supportedArgs = (ArgFlags)(ArgFlags::ARGF_OUTPUT  | ArgFlags::ARGF_ENCRYPT 
                                       | ArgFlags::ARGF_KEYDATA | ArgFlags::ARGF_INITVEC);
            m_output = ReplaceExtension(argv[2], L".cyacd2");
            CopyToInputs(argv, 2, nextArgIdx - 2, m_inputs);
            m_action = ActionType::PATCH;
        }
    }
    else if (cmd == L"-R" || cmd == L"--codeshare")
    {
        if (nextArgIdx < 5)
        {
            wcerr << L"Expected at least 3 arguments (elf file, symbol file, toolchain) for -R/--codeshare" << endl;
            m_action = ActionType::HELP;
            ret = CyErr::CyErrStatus::FAIL;
        }
        else
        {
            m_supportedArgs = (ArgFlags)(ArgFlags::ARGF_OUTPUT);
            m_output = ReplaceExtension(argv[2], L".s");
            CopyToInputs(argv, 2, nextArgIdx - 2, m_inputs);
            m_action = ActionType::SHARE;
        }
    }
    else
    {
        wcerr << L"Unrecognized command: " << cmd << endl;
        m_action = ActionType::HELP;
        ret = CyErr::CyErrStatus::FAIL;
    }

    if (CyErr::CyErrStatus::OK == ret)
        ret = ReadOptionalArgs(argv);

    return ret;
}

CyErr::CyErrStatus CommandLine::ReadOptionalArgs(const vector<wstring> &argv)
{
    static const wchar_t ARG_OUTPUT[] = L"--output";
    static const wchar_t ARG_HEXFILE[] = L"--hex";
    static const wchar_t ARG_KEYDATA[] = L"--key";
    static const wchar_t ARG_ENCRYPT[] = L"--encrypt";
    static const wchar_t ARG_INITVEC[] = L"--iv";

    CyErr::CyErrStatus ret = CyErr::CyErrStatus::OK;
    ArgFlags gotArgs = ARGF_NONE;

    // Start with first argument after command
    for (uint32_t i = 2 + m_inputs.size(); ret == CyErr::CyErrStatus::OK && i < argv.size(); i++)
    {
        if (argv[i].size() != 0 && argv[i][0] == L'-')
        {
            if (argv[i] == ARG_OUTPUT)
            {
                gotArgs |= ARGF_OUTPUT;
                m_output = ReadStringArg(argv, i);
                i++;
            }
            else if (argv[i] == ARG_HEXFILE)
            {
                gotArgs |= ARGF_HEXFILE;
                m_hexFile = ReadStringArg(argv, i);
                i++;
            }
            else if (argv[i] == ARG_KEYDATA)
            {
                gotArgs |= ARGF_KEYDATA;
                uint32_t nextArg = FindNextArgIdx(argv, i+1);
                if (nextArg <= i + 1 || nextArg > i + 2){
                    errx(EXIT_FAILURE, "ERROR: %s option only accepts one argument", ARG_KEYDATA);
                }
                i++;
                if (m_cipher == cmd::CipherType::RSAES_PKCS || m_cipher == cmd::CipherType::RSASSA_PKCS){
                    if (fileExists(wstr_to_str(argv[i]))){
                        m_cipherKey = argv[i]; // OpenSSL expects pem file name instead of explicit string
                    }
                    else{
                        errx(EXIT_FAILURE, "ERROR: could not find key file \"%ls\"", argv[i].c_str());
                    }
                }
                else{
                    ret = ReadKeyFromFile(argv[i], m_cipherKey).ErrorId();
                }
            }
            else if (argv[i] == ARG_ENCRYPT)
            {
                gotArgs |= ARGF_ENCRYPT;
                m_requiredArgs |= ARGF_KEYDATA;
                wstring enc_string = ReadStringArg(argv, i);
                ret = ReadCipherAlgorithm(enc_string);
                i++;
            }
            else if (argv[i] == ARG_INITVEC)
            {
                gotArgs |= ARGF_INITVEC;
                if (cmd::SecureMacType::UNSPECIFIED != m_macType){
                    errx(EXIT_FAILURE, "ERROR: HMAC/CMAC do not support initialization vectors");
                }
                m_requiredArgs |= ARGF_ENCRYPT;
                m_requiredArgs |= ARGF_KEYDATA;
                ret = ReadKeyFromFile(argv[i + 1], m_initVector).ErrorId();
                i++;
            }
            else {
                errx(EXIT_FAILURE, "Unrecognized parameter: %ls", argv[i].c_str());
            }
        }
        else
            m_inputs.push_back(argv[i]);
    }

    if (CyErr::FAIL == ret)
        return ret;

    // Check for valid key length
    if ((gotArgs & ARGF_KEYDATA) && (m_macType != cmd::SecureMacType::HMAC))
    {
        ret = ConfirmKeyLength(m_cipher, m_cipherKey.length());
        if (CyErr::FAIL == ret)
            return ret;
    }
    // check for unsupported flags
    ArgFlags illegalArgs = (ArgFlags)(~m_supportedArgs & gotArgs);
    if (illegalArgs)
    {
        ret = CyErr::CyErrStatus::FAIL;
        if (illegalArgs & ARGF_OUTPUT)
            wcerr << L"ERROR: " << ARG_OUTPUT << L" is not supported for this command." << endl;
        if (illegalArgs & ARGF_HEXFILE)
            wcerr << L"ERROR: " << ARG_HEXFILE << L" is not supported for this command." << endl;
        if (illegalArgs & ARGF_KEYDATA)
            wcerr << L"ERROR: " << ARG_KEYDATA << L" is not supported for this command." << endl;
        if (illegalArgs & ARGF_ENCRYPT)
            wcerr << L"ERROR: " << ARG_ENCRYPT << L" is not supported for this command." << endl;
        if (illegalArgs & ARGF_INITVEC)
            wcerr << L"ERROR: " << ARG_INITVEC << L" is not supported for this command." << endl;
        return ret;
    }
    //Check that all required flags were provided
    ArgFlags missingFlags = (ArgFlags)(m_requiredArgs & ~gotArgs);
    if (missingFlags)
    {
        ret = CyErr::CyErrStatus::FAIL;
        if (missingFlags & ARGF_KEYDATA)
            wcerr << L"ERROR: " << ARG_KEYDATA << L" is required when using --encrypt or HMAC" << endl;
        if (missingFlags & ARGF_INITVEC)
            wcerr << L"ERROR: " << ARG_INITVEC << L" is required when using AES encryption" << endl;
        if (missingFlags & ARGF_ENCRYPT)
            wcerr << L"ERROR: " << ARG_ENCRYPT << L" is required when using CMAC or when an initialization vector is provided" << endl;
        return ret;
    }
    // check special illegal cases
    if ((gotArgs & ARGF_KEYDATA) && m_cipher == cmd::CipherType::UNSPECIFIED && m_macType == cmd::SecureMacType::UNSPECIFIED){
        wcerr << L"ERROR: No cipher or CMAC/HMAC secified, but key was provided. Did you mean to use encryption?" << endl;
        ret = CyErr::CyErrStatus::FAIL;
        return ret;
    }
    // If init vector given, make sure the cipher supports it
    if ((gotArgs & ARGF_ENCRYPT) && (gotArgs & ARGF_INITVEC)) {
        uint32_t validLen = 0;
        switch (m_cipher)
        {
        case cmd::CipherType::AES_128_CBC:
        case cmd::CipherType::AES_128_CFB:
        case cmd::CipherType::AES_192_CBC:
        case cmd::CipherType::AES_192_CFB:
        case cmd::CipherType::AES_256_CBC:
        case cmd::CipherType::AES_256_CFB:
            validLen = 128;
            break;
        default:
            wcerr << L"ERROR: chosen cipher does not support initialization vectors" << endl;
            ret = CyErr::CyErrStatus::FAIL;
            break;
        }
        if (validLen && m_initVector.length() != (validLen / 4)){
            wcerr << L"ERROR: wrong length of initialization vector for chosen cipher. Expected " << (validLen / 4) << L" hex characters, but got " << m_initVector.length() << endl;
            ret = CyErr::CyErrStatus::FAIL;
        }
    }
    // Check that --output and --hex file names are different
    if ((gotArgs & ARGF_OUTPUT) && (gotArgs & ARGF_HEXFILE)){
        if (m_output == m_hexFile){
            wcerr << L"ERROR: --output and --hex file names are the same" << endl;
            ret = CyErr::FAIL;
        }
    }
    // If OpenSSL options requested, confirm OpenSSL is installed
    if (m_cipher != cmd::CipherType::UNSPECIFIED || (m_hash != cmd::HashType::CRC && m_hash != cmd::HashType::UNSPECIFIED)){
        uint32_t notOk = system("openssl version");
        if (notOk){
            wcerr << L"ERROR: OpenSSL not found on system. Must have OpenSSL v1.0.2 or higher installed\nto use cymcuelftool's hash or encryption features (CRC is only hash that does not require it)" << endl;
            ret = CyErr::FAIL;
        }
    }
    return ret;
}

CyErr CommandLine::ReadKeyFromFile(const wstring &keyFile, wstring &dest)
{
    ifstream inFile(wstr_to_str(keyFile).c_str(), std::ios_base::in);
    if (!inFile.good()){
        wcerr << L"Failed to open input file " << keyFile.c_str() << endl;
        return CyErr(CyErr::FAIL, L"");
    }

    //Get file size
    uint32_t fileSize;
    inFile.seekg(0, std::ios::end);
    fileSize = (uint32_t)inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    // Read file contents into string buffer
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    dest = str_to_wstr(buffer.str());
    inFile.close();
    return CyErr();
}

void CommandLine::ReadKey(const vector<wstring> &argv, uint32_t &idx)
{
    uint32_t nextArgIdx = FindNextArgIdx(argv, idx);
    if (nextArgIdx <= idx + 1 || nextArgIdx > idx + 1){
        errx(EXIT_FAILURE, "%s option only accepts one key", wstr_to_str(argv[idx - 1]).c_str());
    }
    m_cipherKey = argv[idx];
}

uint32_t CommandLine::FindNextArgIdx(const vector<wstring> &argv, uint32_t startIdx)
{
    for (uint32_t i = startIdx; i < argv.size(); i++)
    {
        if (argv[i].size() >= 3 && argv[i][0] == L'-' && argv[i][1] == L'-')
            return i;
    }
    return argv.size();
}

CyErr::CyErrStatus CommandLine::ReadCipherAlgorithm(const wstring &arg)
{
    CyErr::CyErrStatus ret = CyErr::CyErrStatus::OK;
    if (L"RSAES-PKCS" == arg){
        m_cipher = cmd::CipherType::RSAES_PKCS;
        if (ActionType::PATCH == m_action 
            || cmd::SecureMacType::CMAC_AES_128 == m_macType
            || cmd::SecureMacType::CMAC_AES_192 == m_macType
            || cmd::SecureMacType::CMAC_AES_256 == m_macType){
            wcerr << L"RSAES-PKCS cipher not supported with CMAC or for --patch command" << endl;
            return CyErr::FAIL;
        }
    }
    else if (L"RSASSA-PKCS" == arg){
        m_cipher = cmd::CipherType::RSASSA_PKCS;
        if (ActionType::PATCH == m_action
            || cmd::SecureMacType::CMAC_AES_128 == m_macType
            || cmd::SecureMacType::CMAC_AES_192 == m_macType
            || cmd::SecureMacType::CMAC_AES_256 == m_macType){
            wcerr << L"RSASSA-PKCS cipher not supported with CMAC or for --patch command" << endl;
            return CyErr::FAIL;
        }
    }
    else if (L"ECC" == arg){
        // TODO: implement support for ECC
        wcerr << L"ERROR: ECC cipher not supported at this time" << endl;
        ret = CyErr::FAIL;
    }
    else if (L"DES-ECB" == arg)
        m_cipher = cmd::CipherType::DES_ECB;
    else if (L"TDES-ECB" == arg)
        m_cipher = cmd::CipherType::TDES_ECB;
    else if (L"AES-128-ECB" == arg)
        m_cipher = cmd::CipherType::AES_128_ECB;
    else if (L"AES-192-ECB" == arg)
        m_cipher = cmd::CipherType::AES_192_ECB;
    else if (L"AES-256-ECB" == arg)
        m_cipher = cmd::CipherType::AES_256_ECB;
    else if (L"AES-128-CBC" == arg){
        m_cipher = cmd::CipherType::AES_128_CBC;
        m_requiredArgs |= ARGF_INITVEC;
    }
    else if (L"AES-192-CBC" == arg){
        m_cipher = cmd::CipherType::AES_192_CBC;
        m_requiredArgs |= ARGF_INITVEC;
    }
    else if (L"AES-256-CBC" == arg){
        m_cipher = cmd::CipherType::AES_256_CBC;
        m_requiredArgs |= ARGF_INITVEC;
    }
    else if (L"AES-128-CFB" == arg){
        m_cipher = cmd::CipherType::AES_128_CFB;
        m_requiredArgs |= ARGF_INITVEC;
    }
    else if (L"AES-192-CFB" == arg){
        m_cipher = cmd::CipherType::AES_192_CFB;
        m_requiredArgs |= ARGF_INITVEC;
    }
    else if (L"AES-256-CFB" == arg){
        m_cipher = cmd::CipherType::AES_256_CFB;
        m_requiredArgs |= ARGF_INITVEC;
    }
    else
    {
        wcerr << L"Unrecognized encryption algorithm: " << arg << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    return ret;
}

CyErr::CyErrStatus CommandLine::ConfirmKeyLength(cmd::CipherType cipher, unsigned keylen)
{
    unsigned valid_length = 0;
    CyErr::CyErrStatus ret = CyErr::CyErrStatus::OK;

    switch (cipher)
    {
        case cmd::CipherType::RSAES_PKCS:
        case cmd::CipherType::RSASSA_PKCS:
            valid_length = 0; // let OpenSSL handle
            return CyErr::CyErrStatus::OK;
        case cmd::CipherType::DES_ECB:
            valid_length = 64;
            break;
        case cmd::CipherType::AES_128_ECB:
        case cmd::CipherType::AES_128_CBC:
        case cmd::CipherType::AES_128_CFB:
        case cmd::CipherType::AES_128_CTR:
            valid_length = 128;
            break; 
        case cmd::CipherType::AES_192_ECB:
        case cmd::CipherType::AES_192_CBC:
        case cmd::CipherType::AES_192_CFB:
        case cmd::CipherType::AES_192_CTR:
        case cmd::CipherType::TDES_ECB:
            valid_length = 192;
            break; 
        case cmd::CipherType::AES_256_ECB:
        case cmd::CipherType::AES_256_CBC:
        case cmd::CipherType::AES_256_CFB:
        case cmd::CipherType::AES_256_CTR:
            valid_length = 256;
            break; 
        default:
            errx(EXIT_FAILURE, L"No encryption type specified, but a key was given");
    }
    if (keylen != (valid_length / 4))
    {
        wcerr << L"ERROR: Invalid key length for cipher: expected " << (valid_length / 4) << L" hex digits, but got " << keylen << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }
    
    return ret;
}

CyErr::CyErrStatus CommandLine::ReadOptionalHashAlgorithm(const vector<wstring> &argv, uint32_t idx)
{
    CyErr::CyErrStatus err = CyErr::OK;
    if (CyErr::OK == ReadMacAlgorithm(argv[idx]))
    {
        m_requiredArgs |= ArgFlags::ARGF_KEYDATA;
        m_supportedArgs = (ArgFlags)(m_supportedArgs & ~ArgFlags::ARGF_ENCRYPT & ~ArgFlags::ARGF_INITVEC); // --encrypt not supported with MAC
        if (getMacType() == cmd::SecureMacType::HMAC)
        {
            /* HMAC */
            if (FindNextArgIdx(argv, idx) <= (idx + 1))
                errx(CyErr::FAIL, L"Error: must specify a hash algorithm after HMAC");
            err = ReadHashAlgorithm(argv[idx + 1]);
            if (m_hash == cmd::HashType::CRC){
                errx(EXIT_FAILURE, "HMAC does not support the CRC hash algorithm");
            }
        }
    }
    else
    {
        err = CyErr::OK;
        if (FindNextArgIdx(argv, idx) >= (idx + 1)){
            /* Hash arg given */
            err = ReadHashAlgorithm(argv[idx]);
        }
    }
    return err;
}

CyErr::CyErrStatus CommandLine::ReadHashAlgorithm(const wstring &arg)
{
    assert(m_hash == cmd::HashType::UNSPECIFIED);

    CyErr::CyErrStatus ret = CyErr::CyErrStatus::OK;
    if (L"CRC" == arg)
        m_hash = cmd::HashType::CRC;
    else if (L"SHA1" == arg)
        m_hash = cmd::HashType::SHA1;
    else if (L"SHA224" == arg)
        m_hash = cmd::HashType::SHA224;
    else if (L"SHA256" == arg)
        m_hash = cmd::HashType::SHA256;
    else if (L"SHA384" == arg)
        m_hash = cmd::HashType::SHA384;
    else if (L"SHA512" == arg)
        m_hash = cmd::HashType::SHA512;
    else if (L"SHA512_224" == arg)
        m_hash = cmd::HashType::SHA512_224;
    else if (L"SHA512_256" == arg)
        m_hash = cmd::HashType::SHA512_256;
    else
    {
        wcerr << L"Unrecognized hash algorithm: " << arg << endl;
        ret = CyErr::CyErrStatus::FAIL;
    }

    return ret;
}

CyErr::CyErrStatus CommandLine::ReadMacAlgorithm(const wstring &arg)
{
    assert(m_macType == cmd::SecureMacType::UNSPECIFIED);

    CyErr::CyErrStatus ret = CyErr::CyErrStatus::OK;
    if (L"HMAC" == arg)
        m_macType = cmd::SecureMacType::HMAC;
    else if (L"CMAC-AES-128" == arg){
        m_macType = cmd::SecureMacType::CMAC_AES_128;
        m_cipher = cmd::CipherType::AES_128_CBC;
    }
    else if (L"CMAC-AES-192" == arg){
        m_macType = cmd::SecureMacType::CMAC_AES_192;
        m_cipher = cmd::CipherType::AES_192_CBC;
    }
    else if (L"CMAC-AES-256" == arg){
        m_macType = cmd::SecureMacType::CMAC_AES_256;
        m_cipher = cmd::CipherType::AES_256_CBC;
    }
    else
    {
        ret = CyErr::CyErrStatus::FAIL;
    }

    return ret;
}

} // namespace cymcuelftool
