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

#ifndef INCLUDED_UTILS_H
#define INCLUDED_UTILS_H

#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <stdint.h>
#include <sys/stat.h>

// Different platforms have different ways to perform 64bit seeks. We use
// cylseek as our neutral lseek name.

#ifdef _MSC_VER
// Windows uses _lseeki64 for 64bit file seeking
#include <io.h>
#define cylseek(fd, offset, whence) _lseeki64((fd), (offset), (whence))
#define O_RDWR _O_RDWR
#define O_BINARY _O_BINARY
typedef int32_t ssize_t;


// Mac uses plain lseek (off_t is 64 bits natively)
#elif __APPLE__
    #include <unistd.h>
    #define cylseek(fd, offset, whence) lseek((fd), (offset), (whence))


// Linux uses lseek64
#else
    #include <unistd.h>
    #define cylseek(fd, offset, whence) lseek64((fd), (offset), (whence))
#endif

namespace cyelflib
{

enum CyEndian
{
    CYENDIAN_LITTLE,
    CYENDIAN_BIG
};

class noncopyable
{
protected:
    noncopyable() {}
public:
    // These methods are unimplemented.
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;
};

std::wstring str_to_wstr(const std::string &src);
std::string wstr_to_str(const std::wstring &src);
uint8_t str_to_uint8(const wchar_t *str);
uint32_t str_to_uint32(const wchar_t *str);

uint8_t ReadHex8(const char *str, unsigned int offset);
uint16_t ReadHex16(const char *str, unsigned int offset);
uint32_t ReadHex32(const char *str, unsigned int offset);
void WriteHex8(char *str, unsigned int offset, uint8_t value);

void HexDump(std::ostream &os, const uint8_t buf[], unsigned int length);

static inline bool fileExists(const std::string &name)
{
    struct stat buf;
    return (stat(name.c_str(), &buf) == 0);
}

static inline uint16_t ReadLE16(const uint8_t *src)
{
    return (uint16_t)(src[0] | (src[1] << 8));
}

static inline void WriteLE16(uint8_t *dst, uint16_t src)
{
    dst[0] = (uint8_t)src;
    dst[1] = (uint8_t)(src >> 8);
}

static inline uint16_t ReadBE16(const uint8_t *src)
{
    return (uint16_t)((src[0] << 8) | src[1]);
}

static inline void WriteBE16(uint8_t *dst, uint16_t src)
{
    dst[0] = (uint8_t)(src >> 8);
    dst[1] = (uint8_t)src;
}

static inline uint32_t ReadLE32(const uint8_t *src)
{
    return src[0] |
          (src[1] << 8) |
          (src[2] << 16) |
          (src[3] << 24);
}

static inline void WriteLE32(uint8_t *dst, uint32_t src)
{
    dst[0] = (uint8_t)src;
    dst[1] = (uint8_t)(src >> 8);
    dst[2] = (uint8_t)(src >> 16);
    dst[3] = (uint8_t)(src >> 24);
}

static inline uint32_t ReadBE32(const uint8_t *src)
{
    return (src[0] << 24) |
           (src[1] << 16) |
           (src[2] << 8) |
            src[3];
}

static inline void WriteBE32(uint8_t *dst, uint32_t src)
{
    dst[0] = (uint8_t)(src >> 24);
    dst[1] = (uint8_t)(src >> 16);
    dst[2] = (uint8_t)(src >> 8);
    dst[3] = (uint8_t)src;
}

static inline uint16_t ReadWithEndian16(CyEndian endian, const uint8_t *src)
{
    return endian == CYENDIAN_BIG ? ReadBE16(src) : ReadLE16(src);
}

static inline void WriteWithEndian16(CyEndian endian, uint8_t *dst, uint16_t src)
{
    if (endian == CYENDIAN_BIG)
        WriteBE16(dst, src);
    else
        WriteLE16(dst, src);
}

static inline uint32_t ReadWithEndian32(CyEndian endian, const uint8_t *src)
{
    return endian == CYENDIAN_BIG ? ReadBE32(src) : ReadLE32(src);
}

static inline void WriteWithEndian32(CyEndian endian, uint8_t *dst, uint32_t src)
{
    if (endian == CYENDIAN_BIG)
        WriteBE32(dst, src);
    else
        WriteLE32(dst, src);
}

static inline uint32_t SwapEndian32(uint32_t val)
{
    return (val << 24) | ((val << 8) & 0x00ff0000) | ((val >> 8) & 0x0000ff00) | ((val >> 24) & 0x000000ff);
}

static inline std::vector<uint8_t> ConvertToBytes(uint32_t value)
{
    std::vector<uint8_t> data;
    data.push_back((uint8_t)(value));
    data.push_back((uint8_t)(value >> 8));
    data.push_back((uint8_t)(value >> 16));
    data.push_back((uint8_t)(value >> 24));
    return data;
}

ssize_t readfd(int fd, void *buf, size_t count);
ssize_t writefd(int fd, const void *buf, size_t count);
bool file_copy(const wchar_t *src, const wchar_t *dst, bool replace = true);
bool file_rename(const wchar_t *src, const wchar_t *dst);
bool file_delete(const wchar_t *path);

std::wstring ReplaceExtension(const std::wstring filename, const std::wstring &newExt);

//This function generates a CRC-32C which is required to match the calculation done by the bootloader SDK
template<typename iterator_type>
uint32_t Compute32bitCRC(iterator_type begin, iterator_type end)
{
    const static uint32_t table[16] =
    {
        0x00000000, 0x105ec76f, 0x20bd8ede, 0x30e349b1,
        0x417b1dbc, 0x5125dad3, 0x61c69362, 0x7198540d,
        0x82f63b78, 0x92a8fc17, 0xa24bb5a6, 0xb21572c9,
        0xc38d26c4, 0xd3d3e1ab, 0xe330a81a, 0xf36e6f75,
    };

    uint32_t crc = 0xFFFFFFFF;
    for (iterator_type it = begin; it != end; ++it)
    {
        crc = crc ^ (*it);
        crc = (crc >> 4) ^ table[crc & 0xF];
        crc = (crc >> 4) ^ table[crc & 0xF];
    }

    return ~crc;
}

uint16_t ComputeCRC16CCITT(const std::vector<uint8_t> &data);
uint32_t RoundToMultiple(uint32_t value, uint32_t multiple, bool upNdown);

void errx(int exitCode, const char *format, ...);
void errx(int exitCode, const wchar_t *format, ...);
void GenerateInTempFile(const std::wstring &outputFile, std::function<void(const std::wstring &)> func);

} // namespace cyelflib

#endif
