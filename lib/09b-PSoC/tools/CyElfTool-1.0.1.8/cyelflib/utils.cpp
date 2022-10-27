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
#include "utils.h"
#include <cstdarg>
#include <cerrno>
#include <locale>
#include <codecvt>

using std::string;
using std::wstring;

namespace cyelflib
{

// See http://en.cppreference.com/w/cpp/locale/codecvt
// utility wrapper to adapt locale-bound facets for wstring/wbuffer convert
template<typename Facet>
struct deletable_facet : Facet
{
    template<typename ...Args>
    deletable_facet(Args&&... args) : Facet(std::forward<Args>(args)...) {}
    ~deletable_facet() {}
};

wstring str_to_wstr(const string &src)
{
    std::wstring_convert<deletable_facet<std::codecvt<wchar_t, char, std::mbstate_t>>> cvt;
    return cvt.from_bytes(src);
}

string wstr_to_str(const wstring &src)
{
    std::wstring_convert<deletable_facet<std::codecvt<wchar_t, char, std::mbstate_t>>> cvt;
    return cvt.to_bytes(src);
}

uint8_t str_to_uint8(const wchar_t *str)
{
    wchar_t *endptr = 0;
    errno = 0;
    uint8_t val = (uint8_t)wcstoul(str, &endptr, 0);
    if (errno || (endptr && *endptr))
        errx(EXIT_FAILURE, L"%ls is not a valid byte", str);
    return val;
}

uint32_t str_to_uint32(const wchar_t *str)
{
    wchar_t *endptr = 0;
    errno = 0;
    uint32_t val = wcstoul(str, &endptr, 0);
    if (errno || (endptr && *endptr))
        errx(EXIT_FAILURE, L"%ls is not a valid non-negative integer", str);
    return val;
}

static uint8_t HexValue(char digit)
{
    if (digit >= '0' && digit <= '9')
        return digit - '0';
    if (digit >= 'a' && digit <= 'f')
        return 10 + digit - 'a';
    if (digit >= 'A' && digit <= 'F')
        return 10 + digit - 'A';
    return 0;
}

uint8_t ReadHex8(const char *str, unsigned int offset)
{
    return (HexValue(str[offset]) << 4) | HexValue(str[offset + 1]);
}

uint16_t ReadHex16(const char *str, unsigned int offset)
{
    return (HexValue(str[offset]) << 12) |
        (HexValue(str[offset + 1]) << 8) |
        (HexValue(str[offset + 2]) << 4) |
        HexValue(str[offset + 3]);
}

uint32_t ReadHex32(const char *str, unsigned int offset)
{
    return (HexValue(str[offset]) << 28) |
        (HexValue(str[offset + 1]) << 24) |
        (HexValue(str[offset + 2]) << 20) |
        (HexValue(str[offset + 3]) << 16) |
        (HexValue(str[offset + 4]) << 12) |
        (HexValue(str[offset + 5]) << 8) |
        (HexValue(str[offset + 6]) << 4) |
        HexValue(str[offset + 7]);
}

void WriteHex8(char *str, unsigned int offset, uint8_t value)
{
    static const char hexDigits[] = "0123456789ABCDEF";
    str[offset] = hexDigits[(value >> 4) & 0x0F];
    str[offset + 1] = hexDigits[value & 0x0F];
}

void HexDump(std::ostream &os, const uint8_t buf[], unsigned int length)
{
    char str[3];
    str[2] = '\0';
    for (unsigned int i = 0; i < length; i++)
    {
        WriteHex8(str, 0, buf[i]);
        os << str;
    }
}

// readfd is a wrapper that restarts on a partial read or EINTR
ssize_t readfd(int fd, void *buf, size_t count)
{
    size_t total = 0;
    ssize_t r = 1;
    uint8_t *ptr = (uint8_t *)buf;

    while (total < count && r != 0)
    {
        r = read(fd, ptr, count - total);
        if (r < 0 && errno != EINTR)
            return r;

        if (r > 0)
        {
            total += r;
            ptr += r;
        }
    }

    return total;
}

// writefd is a wrapper that restarts on a partial read or EINTR
ssize_t writefd(int fd, const void *buf, size_t count)
{
    size_t total = 0;
    const uint8_t *ptr = (const uint8_t *)buf;
    ssize_t r = 1;
    while (total < count)
    {
        r = write(fd, ptr, count - total);
        if (r < 0 && errno != EINTR)
            return r;
        total += r;
        ptr += r;
    }

    return total;
}

bool file_copy(const wchar_t *src, const wchar_t *dst, bool replace/* = true*/)
{
    if (!fileExists(wstr_to_str(src))){
        std::wcerr << L"File \"" << src << L"\" does not exist." << std::endl;
        return false;
    }
    ssize_t size = -1;
    if (replace && !file_delete(dst))
        return false;
#ifdef _MSC_VER
    int srcFd = _wopen(src, _O_BINARY | _O_RDONLY);
    int dstFd = _wopen(dst, _O_BINARY | _O_WRONLY | _O_CREAT | _O_EXCL, _S_IREAD | _S_IWRITE);
#else
    int srcFd = open(wstr_to_str(src).c_str(), O_RDONLY);
    int dstFd = open(wstr_to_str(dst).c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
#endif
    if (srcFd != -1 && dstFd != -1)
    {
        uint8_t buf[4096];
        while ((size = readfd(srcFd, buf, sizeof(buf))) > 0)
        {
            if (writefd(dstFd, buf, size) != size)
            {
                size = -1;
                break;
            }
        }
    }
    if (srcFd != -1)
        close(srcFd);
    if (dstFd != -1)
        close(dstFd);
    return size >= 0;
}

bool file_rename(const wchar_t *src, const wchar_t *dst)
{
    if (!fileExists(wstr_to_str(src))){
        std::wcerr << L"File \"" << src << L"\" does not exist." << std::endl;
        return false;
    }
#ifdef _MSC_VER
    return 0 == _wrename(src, dst);
#else
    return 0 == rename(wstr_to_str(src).c_str(), wstr_to_str(dst).c_str());
#endif
}

bool file_delete(const wchar_t *path)
{
#ifdef _MSC_VER
    return _wunlink(path) == 0 || errno == ENOENT;
#else
    return unlink(wstr_to_str(path).c_str()) == 0 || errno == ENOENT;
#endif
}

std::wstring ReplaceExtension(std::wstring filename, const std::wstring &newExt)
{
    typedef std::wstring::size_type size_type;
    for (int i = (int)filename.length() - 1; i >= 0; i--)
    {
        if (filename[i] == '.')
            return filename.replace((size_type)i, (size_type)(filename.length() - i), newExt);
        else if (filename[i] == '/'
#ifdef WIN32
                || filename[i] == '\\'
#endif
            )
            return filename + newExt;
    }
    return filename + newExt;
}

//This function generates a CRC-32C (based on Numerical Recipes in C, page 900, but an initial value of 0xFFFF instead of 0x0000)
uint16_t ComputeCRC16CCITT(const std::vector<uint8_t> &data)
{
    uint16_t crc = 0xFFFF;
    for (std::vector<uint8_t>::const_iterator it = data.begin(); it < data.end(); it++)
    {
        crc = (crc ^ (*it) << 8);
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x8000)
                crc = (crc <<= 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

uint32_t RoundToMultiple(uint32_t value, uint32_t multiple, bool upNdown)
{
    if (0 == multiple)
        return value;

    uint32_t remainder = value % multiple;
    if (0 == remainder)
        return value;

    return upNdown
        ? value + multiple - remainder
        : value - remainder;
}

// define to make it easier to reuse some of the libelf example code... 
void errx(int exitCode, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    puts("");
    exit(exitCode);
}

void errx(int exitCode, const wchar_t *format, ...)
{
    va_list args;
    va_start(args, format);
    vwprintf(format, args);
    va_end(args);
    fputws(L"\n", stdout);
    exit(exitCode);
}

void GenerateInTempFile(const std::wstring &outputFile, std::function<void(const std::wstring &)> func)
{
    std::wstring tmpOutFile = outputFile + L".tmp";

    func(tmpOutFile);

    file_delete(outputFile.c_str());
    if (!file_rename(tmpOutFile.c_str(), outputFile.c_str()) != 0){
        file_delete(tmpOutFile.c_str());
        errx(EXIT_FAILURE, L"Failed to move %ls to %ls", tmpOutFile.c_str(), outputFile.c_str());
    }
}

} // namespace cyelflib
