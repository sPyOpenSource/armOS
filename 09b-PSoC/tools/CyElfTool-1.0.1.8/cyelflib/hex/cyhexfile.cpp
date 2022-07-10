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
#include <algorithm>
#include <cassert>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <exception>
#include <stdio.h>
#include <limits.h>
#include "cyhexfile.h"

namespace cyelflib {
namespace hex {

// Static class member initalization

const std::string CyHexFile::EOF_LINE = ":00000001FF";

CyHexFileLine::CyHexFileLine(const std::string &line, uint32_t baseAddr) :
    m_line(line),
    m_length(0),
    m_address(0),
    m_type(CYHEXRECORDTYPE_DATA),
    m_checksum(0),
    m_valid(false),
    m_offset(0)
{
    const char *linechars = line.c_str();
    m_valid = line.length() > 6 && line[0] == LINE_IDENT;

    for (size_t i = 1; i < line.length(); i++)
        m_valid = m_valid && isxdigit(line[i]);

    if (m_valid)
    {
        m_length = ReadHex8(linechars, LENGTH_LOC);
        m_offset = ReadHex16(linechars, OFFSET_LOC);
        m_type = (CyHexRecordType)ReadHex8(linechars, TYPE_LOC);
        m_checksum = ReadHex8(linechars, line.length() - 2);

        m_address = baseAddr + m_offset;

        uint8_t checksum = m_length;
        checksum += (uint8_t)m_type;
        checksum += (uint8_t)(m_offset & 0xFF);
        checksum += (uint8_t)(m_offset >> 8);

        m_data.resize(m_length);
        for (int i = 0, j = DATA_LOC; i < m_length; i++, j += 2)
        {
            uint8_t tmp = ReadHex8(linechars, j);
            m_data[i] = (uint8_t)tmp;
            checksum += (uint8_t)tmp;
        }

        checksum = (uint8_t)(CHECKSUM_INVERTER - checksum);

        m_valid = ValidTypeData(m_type, m_data);
        m_valid &= line.length() == m_data.size() * 2 + RECORD_OVERHEAD;
        m_valid &= (checksum == m_checksum);
    }
}

CyHexFileLine::CyHexFileLine(uint32_t baseAddr, uint16_t offset, CyHexRecordType type, const uint8_t *data, size_t size)
{
    assert(size <= 255);
    m_length = (uint8_t)size;
    m_offset = offset;
    m_address = baseAddr + offset;
    m_type = type;
    m_data.assign(data, data + size);

    m_valid = ValidTypeData(type, m_data);

    RecalculateLineInfo();
}

void CyHexFileLine::RecalculateLineInfo()
{
    char *str;
    int dataLoc;

    m_checksum = m_length;
    m_checksum += (uint8_t)m_type;
    m_checksum += (uint8_t)(m_offset & 0xFF);
    m_checksum += (uint8_t)(m_offset >> 8);

    int numChars = m_length * 2 + RECORD_OVERHEAD + 1;
    str = new char[numChars];
#ifdef _MSC_VER
    sprintf_s(str, numChars, "%c%02X%04X%02X", LINE_IDENT, m_length, m_offset,  (uint8_t)m_type);
#else
    snprintf(str, numChars, "%c%02X%04X%02X", LINE_IDENT, m_length, m_offset,  (uint8_t)m_type);
#endif

    dataLoc = DATA_LOC;
    for (int i = 0; i < m_length; i++)
    {
        WriteHex8(str, dataLoc, m_data[i]);
        m_checksum += m_data[i];
        dataLoc += 2;
    }
    m_checksum = (uint8_t)(CHECKSUM_INVERTER - m_checksum);
    WriteHex8(str, dataLoc, m_checksum);
    assert(dataLoc + 2 == numChars - 1);

    m_line.assign(str, numChars - 1);
}

bool CyHexFileLine::ValidTypeData(CyHexRecordType type, const std::vector<uint8_t> &data)
{
    bool isValid = true;
    switch (type)
    {
    case CYHEXRECORDTYPE_DATA:
        isValid = (data.size() > 0);
        break;
    case CYHEXRECORDTYPE_END_OF_FILE:
        isValid = (data.size() == (size_t)EOF_LEN);
        break;
    case CYHEXRECORDTYPE_EXTENDED_SEGMENT_ADDRESS:
        isValid = (data.size() == (size_t)EXTEND_SEG_LEN);
        break;
    case CYHEXRECORDTYPE_START_SEGMENT_ADDRESS:
        isValid = (data.size() == (size_t)START_SEG_LEN);
        break;
    case CYHEXRECORDTYPE_EXTENDED_LINEAR_ADDRESS:
        isValid = (data.size() == (size_t)EXTEND_LIN_LEN);
        break;
    case CYHEXRECORDTYPE_START_LINEAR_ADDRESS:
        isValid = (data.size() == (size_t)START_LIN_LEN);
        break;
    default:
        isValid = false;
        break;
    }
    return isValid;
}

/// <summary>
/// Used to sort CyHexFileLine objects
/// </summary>
static bool LineSort(CyHexFileLine *a, CyHexFileLine *b)
{
    return a->Address() < b->Address();
}

bool CyHexFile::SetValueAtAddress(uint32_t address, const uint8_t *valArr, size_t length)
{
    std::vector<CyHexFileLine *>::iterator it;

    for (it = m_data.begin(); it != m_data.end(); ++it)
    {
        if ((*it)->Type() == CYHEXRECORDTYPE_DATA)
        {
            uint32_t lastByte = (*it)->Address() + (*it)->Length() - 1; // the last byte of available data in this line
            if ((*it)->Address() <= address && address <= lastByte)
            {
                // found the right line, now plugin our values
                uint32_t lineIdx = address - (*it)->Address();
                uint32_t i, j;
                for (i = lineIdx, j = 0; i < (*it)->Length() && j < length; ++i, ++j)
                    (*it)->Data()[i] = valArr[j];

                (*it)->RecalculateLineInfo();

                if (lastByte < address + length - 1)
                {
                    // oops, our data is split over two lines, construct shortented data with the correct
                    // address and recursively call this method to set the remaining bytes.
                    uint32_t nextAddr = lastByte + 1; // very next address beyond us
                    size_t remainingBytesSize = length - j; // j is the byte after the last one used
                    std::vector<uint8_t> remainingBytes(remainingBytesSize);
                    return SetValueAtAddress(nextAddr, valArr + j, remainingBytesSize);
                }
                else
                    return true; // base case
            }
        }
    }

    return true;
}


/// Creates a new instance of a CyHexFile from a list of hex records. Does not copy the list.
/// </summary>
/// <param name="data"></param>
CyHexFile::CyHexFile(std::vector<CyHexFileLine *> data)
{
    m_data.clear();
    m_data = data;
}

CyHexFile::CyHexFile()
{
}

CyHexFile::~CyHexFile()
{
    for (size_t i = 0; i < m_data.size(); i++)
        delete m_data[i];
    m_data.clear();
}

/// <summary>
/// Reads an actual Hex file and generates an instance of CyHexFile
/// </summary>
/// <param name="sourceFile">The actual hex file on disk</param>
/// <param name="fileData">The Parsed CyHexFile containing all of the data from the on disk hex file</param>
/// <returns>0 if successful, otherwise an error</returns>
CyErr CyHexFile::Read(const std::wstring &sourceFile, CyHexFile* &fileData)
{
    CyErr err;
    std::vector<CyHexFileLine *> data;
    std::string lineBuf;
#ifdef _MSC_VER
    std::ifstream reader(sourceFile.c_str());
#else
    std::ifstream reader(wstr_to_str(sourceFile).c_str());
#endif
    if (reader.fail())
        err = CyErr(CyErr::FAIL, L"Unable to read hex file %ls", sourceFile.c_str());

    if (err.IsOK())
        err = ReadLines(sourceFile, reader, data);

    if (err.IsOK())
        fileData = new CyHexFile(data);
    else
    {
        for (size_t i = 0; i < data.size(); i++)
            delete data[i];
    }

    reader.close();

    return err;
}

CyErr CyHexFile::ReadLines(const std::wstring &filename, std::istream &reader, std::vector<CyHexFileLine *> &data)
{
    CyErr err;
    std::string lineBuf;

    int lineNum = 0;
    uint32_t addrHigh = 0;
    while (err.IsOK() && !reader.eof())
    {
        lineNum++;
        getline(reader, lineBuf);
        CyHexFileLine *line = new CyHexFileLine(lineBuf.data(), addrHigh);
        if (line->IsValid())
        {
            if (line->Type() == CYHEXRECORDTYPE_DATA && (line->Address() & 0xFFFF0000) != ((line->Address() + line->Length() - 1) & 0xFFFF0000))
            {
                err = CyErr(CyErr::FAIL, L"Segment wrapping not supported: start=0x%08X end=0x%08X",
                    line->Address(), line->Address()+line->Length()-1);
            }

            data.push_back(line);

            if (line->Type() == CYHEXRECORDTYPE_EXTENDED_LINEAR_ADDRESS)
                // LBA defines bits 31:16 of address
                addrHigh = (uint32_t)(line->Data()[0] << 24) | (uint32_t)(line->Data()[1] << 16);
            else if (line->Type() == CYHEXRECORDTYPE_EXTENDED_SEGMENT_ADDRESS)
                // SBA defines bits 19:4 of base address
                addrHigh = (uint32_t)(line->Data()[0] << 12) | (uint32_t)(line->Data()[1] << 4);
            else if (line->Type() == CYHEXRECORDTYPE_END_OF_FILE)
                break;
        }
        else
        {
            delete line;
            err = CyErr(CyErr::FAIL, L"Invalid line at %ls:%d", filename.c_str(), lineNum);
        }
    }

    return err;
}

/// <summary>
/// This method adds lines to the end of the file. It may be necessary to remove EOF lines first.
/// </summary>
/// <param name="lines"></param>
/// <returns></returns>
CyErr CyHexFile::Append(const std::vector<CyHexFileLine *> &lines)
{
    CyErr err;

    std::vector<CyHexFileLine *>::const_iterator it;

    for (it = lines.begin(); it != lines.end(); ++it)
    {
        if (!(*it)->IsValid())
            return CyErr(L"Appending an invalid line");
    }

    m_data.insert(m_data.end(), lines.begin(), lines.end());

    return err;
}

CyErr CyHexFile::Append(CyHexFileLine *line)
{
    if (!line->IsValid())
        return CyErr(L"Appending an invalid line");
    m_data.push_back(line);
    return CyErr();
}

static uint32_t AppendAddressRecord(CyHexFile *hexFile, uint32_t address)
{
    uint8_t bytes[] = { (uint8_t)(address >> 24), (uint8_t)(address >> 16) };
    CyErr err = hexFile->Append(new CyHexFileLine(
        address & 0xFFFF0000u,
        0,
        CYHEXRECORDTYPE_EXTENDED_LINEAR_ADDRESS,
        bytes,
        2));
    assert(err.IsOK());
    return address & 0xFFFF0000u;
}

void CyHexFile::AppendData(uint32_t address, const void *buf, size_t size)
{
    static const size_t ROW_MAX = 64;

    const uint8_t *buf8 = (const uint8_t *)buf;

    uint32_t base = 0;
    if (m_data.size() != 0)
    {
        size_t i = m_data.size() - 1;
        if (m_data[i]->Type() == CYHEXRECORDTYPE_EXTENDED_LINEAR_ADDRESS)
        {
            const std::vector<uint8_t> &data = m_data[i]->Data();
            assert(data.size() == 2);
            base = (data[0] << 24) + (data[1] << 16);
        }
        else if (m_data[i]->Type() == CYHEXRECORDTYPE_DATA)
            base = m_data[i]->Address() & 0xFFFF0000u;
        else
            base = AppendAddressRecord(this, address);
    }
    else
        base = 0u;

    for (size_t off = 0; off < size;)
    {
        uint32_t addr = address + off;
        size_t rowLen = std::min(size - off, ROW_MAX);

        // Insert extended address record if needed
        if ((base & 0xFFFF0000u) != (addr & 0xFFFF0000u))
            base = AppendAddressRecord(this, addr);

        // Align end of row to 64 bytes
        // This makes the file look nicer and ensures that the address will not wrap around
        uint32_t rowStart = addr % ROW_MAX;
        if (rowStart != 0 && rowStart + rowLen > ROW_MAX)
            rowLen = ROW_MAX - rowStart;

        m_data.push_back(new CyHexFileLine(
            base,
            (uint16_t)addr,
            CYHEXRECORDTYPE_DATA,
            &buf8[off],
            rowLen));

        off += rowLen;
    }
}

/// <summary>
/// Removes EOF lines from the end of the file.
/// </summary>
void CyHexFile::StripEof()
{
    while (m_data.size() >= 1 && m_data.back()->Type() == CYHEXRECORDTYPE_END_OF_FILE)
    {
        CyHexFileLine *last = m_data.back();
        m_data.pop_back();
        delete last;
    }
}

/// <summary>
/// This method calculates the checksum of the data lines.
/// </summary>
/// <returns>checksum of all data lines.</returns>
uint16_t CyHexFile::Checksum() const
{
    return Checksum(m_data);
}

/// <summary>
/// This method calculates the checksum of the specified data lines.
/// </summary>
/// <param name="data">input lines</param>
/// <returns>checksum of data lines.</returns>
uint16_t CyHexFile::Checksum(const std::vector<CyHexFileLine *> &data)
{
    uint16_t checksum = 0;
    std::vector<CyHexFileLine *>::const_iterator it;

    for (it = data.begin(); it != data.end(); ++it)
    {
        if ((*it)->Type() == CYHEXRECORDTYPE_DATA)
        {
            for (int i = 0; i < (*it)->Length(); i++)
                checksum += (*it)->Data()[i];
        }
    }
    return checksum;
}

/// <summary>
/// Find the minimum address that contains data.
/// </summary>
/// <returns>minimum address, uint32_t.MaxValue if no data.</returns>
uint32_t CyHexFile::FindMinAddress() const
{
    uint32_t min = UINT_MAX;
    std::vector<CyHexFileLine *>::const_iterator it;

    for (it = m_data.begin(); it != m_data.end(); ++it)
    {
        if ((*it)->Type() == CYHEXRECORDTYPE_DATA)
        {
            if ((*it)->Address() < min)
                min = (*it)->Address();
        }
    }
    return min;
}

/// <summary>
/// Find the maximum address that contains data.
/// </summary>
/// <returns>maximum address, 0 if no data.</returns>
uint32_t CyHexFile::FindMaxAddress() const
{
    uint32_t max = 0;
    std::vector<CyHexFileLine *>::const_iterator it;

    for (it = m_data.begin(); it != m_data.end(); ++it)
    {
        if ((*it)->Type() == CYHEXRECORDTYPE_DATA)
        {
            uint32_t end = (*it)->Address() + (*it)->Length() - 1;
            if (end > max)
                max = end;
        }
    }
    return max;
}

/// <summary>
/// Sets the given value at the given address in the hex file.
/// </summary>
/// <param name="address">The address of the value.</param>
/// <param name="value">The value to set.</param>
/// <param name="size">The size of the value in bytes.</param>
/// <param name="isLittleEndian">true if the data should be encoded in little endian byte layout.</param>
/// <returns>true if the value was successfully set. If false is returned then the address + size was
/// greater than the maximum address in the file.</returns>
bool CyHexFile::SetValueAtAddress(uint32_t address, uint32_t value, uint32_t size, bool isLittleEndian)
{
    uint32_t maxAddr = FindMaxAddress();
    if ((address + size - 1) > maxAddr)
        return false;

    std::vector<uint8_t> valArr(size);
    if (size > 1)
    {
        if (isLittleEndian)
        {
            for (size_t i = 0; i < size; ++i)
            {
                valArr[i] = (uint8_t)(value >> (i * 8)); // shift value down by 8 bits every iteration
            }
        }
        else
        {
            for (size_t i = 0; i < size; ++i)
            {
                valArr[i] = (uint8_t)(value >> (int)((size - 1 - i) * 8)); // shift value down by 8 bits every iteration
            }
        }
    }
    else
        valArr[0] = (uint8_t)value;

    return SetValueAtAddress(address, &valArr[0], size);
}


/// <summary>
/// Get data from a section of the hex file
/// </summary>
/// <param name="startAddr">Beginning address of the section.</param>
/// <param name="length">Length of the section.</param>
/// <param name="pad">Filler byte for missing data.</param>
/// <returns>Byte array containing data from hex file.</returns>
uint8_t *CyHexFile::ToByteArray(uint32_t startAddr, uint32_t length, uint8_t pad) const
{
    uint8_t* output = new uint8_t[length];
    uint32_t endAddr = startAddr + length - 1;
    std::vector<CyHexFileLine *>::const_iterator it;

    for (uint32_t i = 0; i < length; i++)
        output[i] = pad;

    for (it = m_data.begin(); it != m_data.end(); ++it)
    {
        if ((*it)->Type() == CYHEXRECORDTYPE_DATA)
        {
            uint32_t lineStart = (*it)->Address();
            uint32_t lineEnd = (*it)->Address() + (*it)->Length() - 1;

            if (lineStart <= endAddr && lineEnd >= startAddr)
            {
                uint32_t begin = std::max(startAddr, lineStart);
                uint32_t end = std::min(lineEnd, endAddr);
                for (uint32_t a = begin, i = a - lineStart, j = a - startAddr; a <= end; a++, i++, j++)
                {
                    output[j] = (*it)->Data()[i];
                }
            }
        }
    }
    return output;
}

/// <summary>
/// Add an offset to all addresses. To ensure that extended address records are correct, SortRealign should be
/// called after this method.
/// </summary>
/// <param name="offset"></param>
void CyHexFile::Rebase(uint32_t offset)
{
    if (offset == 0)
        return;

    for (size_t i = 0; i < m_data.size(); i++)
    {
        CyHexFileLine *orig = m_data[i];
        uint32_t addr = orig->Address() + offset;
        m_data[i] = new CyHexFileLine(addr & 0xFFFF0000,
            (uint16_t)addr,
            orig->Type(),
            &(orig->Data()[0]),
            orig->Data().size());
        delete orig;
    }
}

/// <summary>
/// Remove data from the hex file that isn't in the specified range. SortRealign should be called after this
/// method to normalize data lines and remove unnecessary non-data lines.
/// </summary>
/// <param name="startAddr">Starting address</param>
/// <param name="length">Length of data</param>
void CyHexFile::Truncate(uint32_t startAddr, uint32_t length)
{
    uint32_t endAddr = startAddr + length - 1;
    std::vector<CyHexFileLine *> truncatedData;

    for (size_t i = 0; i < m_data.size(); i++)
    {
        CyHexFileLine *orig = m_data[i];
        if (orig->Type() == CYHEXRECORDTYPE_DATA)
        {
            uint32_t lineStart = orig->Address();
            uint32_t lineEnd = lineStart + orig->Length() - 1;

            if (lineEnd >= startAddr && lineStart <= endAddr)
            {
                // At least part of the line is in range
                if (lineStart >= startAddr && lineEnd <= endAddr)
                {
                    // Completely in range; copy as-is
                    truncatedData.push_back(orig);
                }
                else
                {
                    uint32_t copyStart = 0;
                    uint32_t copyLen = orig->Length();

                    // Truncate beginning
                    if (lineStart < startAddr)
                    {
                        uint32_t diff = startAddr - lineStart;
                        copyStart += diff;
                        copyLen -= diff;
                    }
                    // Truncate end
                    if (lineEnd > endAddr)
                    {
                        copyLen -= (lineEnd - endAddr);
                    }
                    std::vector<uint8_t> copyData(copyLen);

                    std::vector<uint8_t>::iterator start = orig->Data().begin();
                    std::vector<uint8_t>::iterator finish = orig->Data().begin();

                    std::advance(start, copyStart);
                    std::advance(finish, copyStart + copyLen);

                    std::copy(start, finish, copyData.begin());

                    uint32_t copyAddr = orig->Address() + copyStart;
                    truncatedData.push_back(new CyHexFileLine(copyAddr & 0xFFFF0000, 
                        (uint16_t)copyAddr, CYHEXRECORDTYPE_DATA, &copyData[0], copyData.size()));
                }
            }
        }
        else
            truncatedData.push_back(orig);
    }
    m_data = truncatedData;
}

/// <summary>
/// Sort the lines. Lines must have correct high address words. The lines will be rearranged
/// such that each line begins on a 2^align boundary. If necessary, pad will be inserted as padding.
/// </summary>
/// <param name="align">Align beginning of lines to 2^align.</param>
/// <param name="pad">Padding byte to add for alignment.</param>
/// <param name="prevBase">Base address of previous line, 0 if none.</param>
/// <returns>CyErr.OK on success.</returns>
CyErr CyHexFile::SortRealign(int align, uint8_t pad, uint32_t prevBase)
{
    return SortRealign(align, pad, prevBase, false, 0, 0);
}

/// <summary>
/// Sort the lines. Replaces all non-data lines. Lines must have correct addresses. The lines will be rearranged
/// such that each line begins on a 2^align boundary. If necessary, pad will be inserted as padding.
/// </summary>
/// <param name="align">Align beginning of lines to 2^align.</param>
/// <param name="pad">Padding byte to add for alignment.</param>
/// /// <param name="prevBase">Base address of previous line, 0 if none.</param>
/// <param name="padAll">If true, fills the entire startAddr...length range, using padding as necessary.</param>
/// <param name="startAddr">Starting address of padding. Must be less than the first data address. Ignored
/// if padAll is false. Must be a multiple of 2^align.</param>
/// <param name="length">Length of address range to pad. Ignored if padAll is false. If the end of the data
/// exceeds startAddr+length, no padding lines will be added to the end. Must be a multiple of 2^align. If
/// length is 0, padding will only be added to complete the last data line.</param>
/// <returns>CyErr.OK on success.</returns>


CyErr CyHexFile::SortRealign(int align, uint8_t pad, uint32_t prevBase, bool padAll, uint32_t startAddr, uint32_t length)
{
    CyErr err;

    uint32_t alignMask = ~((uint32_t)(1 << align) - 1);
    unsigned int line_len = 1 << align;
    std::vector<CyHexFileLine *>::iterator it;

    // Get just the data lines
    std::vector<CyHexFileLine*> dataLines;

    for (it = m_data.begin(); it != m_data.end(); ++it)
    {
        if ((*it)->Type() == CYHEXRECORDTYPE_DATA)
            dataLines.push_back((*it));
    }

    // Sort lines by address
    std::stable_sort(dataLines.begin(), dataLines.end(), LineSort);

    std::vector<CyHexFileLine*> alignedLines;

    std::vector<uint8_t> padline(line_len);
    for (size_t i = 0; i < line_len; i++)
        padline[i] = pad;

    if (padAll)
    {
        if ((startAddr & ~alignMask) != 0 || (length & ~alignMask) != 0)
            return CyErr(L"start address and length must be multiples of 2^align");
    }

    uint32_t prevLineAddr = prevBase;
    uint32_t lineAddr = padAll ? startAddr : 0;
    std::vector<uint8_t> bytes;
    for (it = dataLines.begin(); it != dataLines.end(); ++it)
    {
        if ((*it)->Address() >= lineAddr + line_len)
        {
            // Need to start a new line
            if (bytes.size() != 0)
            {
                // Finish the previous line
                // Check if new LBA line is needed
                if ((lineAddr & 0xFFFF0000) != (prevLineAddr & 0xFFFF0000))
                {
                    uint8_t extAddr[2];
                    extAddr[0] = (uint8_t)(lineAddr >> 24);
                    extAddr[1] = (uint8_t)(lineAddr >> 16);
                    alignedLines.push_back(new CyHexFileLine(0, 0, CYHEXRECORDTYPE_EXTENDED_LINEAR_ADDRESS,
                        extAddr, 2));
                }
                // Pad until end
                while (bytes.size() < line_len)
                    bytes.push_back(pad);

                alignedLines.push_back(new CyHexFileLine(lineAddr & 0xFFFF0000, (uint16_t)lineAddr,
                    CYHEXRECORDTYPE_DATA, &bytes[0], bytes.size()));
                prevLineAddr = lineAddr;
                lineAddr += (uint32_t)line_len;
                bytes.clear();
            }

            if (padAll)
            {
                if ((*it)->Address() < startAddr)
                    return CyErr(CyErr::FAIL, L"data address 0x%0X is less than start address 0x%0X",
                        (*it)->Address(), startAddr);

                // Pad until next data line
                while ((*it)->Address() >= lineAddr + line_len)
                {
                    if ((lineAddr & 0xFFFF0000) != (prevLineAddr & 0xFFFF0000))
                    {
                        uint8_t extAddr[2];
                        extAddr[0] = (uint8_t)(lineAddr >> 24);
                        extAddr[1] = (uint8_t)(lineAddr >> 16);
                        alignedLines.push_back(new CyHexFileLine(0, 0, CYHEXRECORDTYPE_EXTENDED_LINEAR_ADDRESS,
                            extAddr, 2));
                    }

                    alignedLines.push_back(new CyHexFileLine(lineAddr & 0xFFFF0000, (uint16_t)lineAddr,
                        CYHEXRECORDTYPE_DATA, &padline[0], padline.size()));
                    prevLineAddr = lineAddr;
                    lineAddr += (uint32_t)line_len;
                }
            }

            // Start new line
            lineAddr = (*it)->Address() & alignMask;
        }

        // Pad until beginning of data, if necessary
        if ((*it)->Address() < lineAddr + bytes.size())
#ifdef _MSC_VER
            return CyErr(CyErr::FAIL, L"Hex file line overlaps previous data: %hs\n", (*it)->Line().c_str());
#else
            return CyErr(CyErr::FAIL, L"Hex file line overlaps previous data: %s\n", (*it)->Line().c_str());
#endif

        if ((*it)->Address() > lineAddr + bytes.size())
        {
            int paddingSize = (int)((*it)->Address() - (lineAddr + bytes.size()));
            for (int i = 0; i < paddingSize; i++)
                bytes.push_back(pad);
        }

        for (size_t i = 0; i < (*it)->Data().size(); i++)
        {
            // Add data to the line
            bytes.push_back((*it)->Data()[i]);
            // Output line when full
            if (bytes.size() >= line_len)
            {
                // Check if new LBA line is needed
                if ((lineAddr & 0xFFFF0000) != (prevLineAddr & 0xFFFF0000))
                {
                    uint8_t extAddr[2];
                    extAddr[0] = (uint8_t)(lineAddr >> 24);
                    extAddr[1] = (uint8_t)(lineAddr >> 16);
                    alignedLines.push_back(new CyHexFileLine(0, 0, CYHEXRECORDTYPE_EXTENDED_LINEAR_ADDRESS,
                        extAddr, 2));
                }

                // Output line
                alignedLines.push_back(new CyHexFileLine(lineAddr & 0xFFFF0000, (uint16_t)lineAddr,
                    CYHEXRECORDTYPE_DATA, &bytes[0], bytes.size()));
                prevLineAddr = lineAddr;
                lineAddr += (uint32_t)line_len;
                bytes.clear();
            }
        }
    }

    if (bytes.size() != 0)
    {
        // Finish the previous line
        // Check if new LBA line is needed
        if ((lineAddr & 0xFFFF0000) != (prevLineAddr & 0xFFFF0000))
        {
            uint8_t extAddr[2];
            extAddr[0] = (uint8_t)(lineAddr >> 24);
            extAddr[1] = (uint8_t)(lineAddr >> 16);
            alignedLines.push_back(new CyHexFileLine(0, 0, CYHEXRECORDTYPE_EXTENDED_LINEAR_ADDRESS, extAddr, 2));
        }

        // Pad until end
        while (bytes.size() < line_len)
            bytes.push_back(pad);
   
        alignedLines.push_back(new CyHexFileLine(lineAddr & 0xFFFF0000, (uint16_t)lineAddr,
            CYHEXRECORDTYPE_DATA, &bytes[0], bytes.size()));
        prevLineAddr = lineAddr;
        lineAddr += (uint32_t)line_len;
        bytes.clear();
    }

    if (padAll)
    {
        uint32_t endAddr = startAddr + length;

        // Pad to end of address range
        while (lineAddr < endAddr)
        {
            if ((lineAddr & 0xFFFF0000) != (prevLineAddr & 0xFFFF0000))
            {
                uint8_t extAddr[2];
                extAddr[0] = (uint8_t)(lineAddr >> 24);
                extAddr[1] = (uint8_t)(lineAddr >> 16);
                alignedLines.push_back(new CyHexFileLine(0, 0, CYHEXRECORDTYPE_EXTENDED_LINEAR_ADDRESS,
                    extAddr, 2));
            }
            alignedLines.push_back(new CyHexFileLine(lineAddr & 0xFFFF0000, (uint16_t)lineAddr,
                CYHEXRECORDTYPE_DATA, &padline[0], padline.size()));
            prevLineAddr = lineAddr;
            lineAddr += (uint32_t)line_len;
        }
    }

    m_data = alignedLines;

    return err;
}

/// <summary>
/// This method writes the in memory representation of the hex file to disk
/// </summary>
/// <param name="writer">The destination stream</param>
/// <returns> 0 if successful, otherwise an error</returns>
CyErr CyHexFile::Write(std::ofstream &writer) const
{
    CyErr err;

    std::vector<CyHexFileLine *>::const_iterator it;
    bool hasEOF = false;
    for (it = m_data.begin(); it != m_data.end(); ++it)
    {
        writer << (*it)->Line() << '\n';
        if ((*it)->Type() == CYHEXRECORDTYPE_END_OF_FILE)
            hasEOF = true;
    }

    if (!hasEOF)
        writer << EOF_LINE;

    return err;
}

} // namespace hex
} // namespace cyelflib
