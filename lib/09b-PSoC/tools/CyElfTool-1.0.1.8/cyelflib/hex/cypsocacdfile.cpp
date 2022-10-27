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
#include <vector>
#include <sstream>
#include "cypsocacdfile.h"

namespace cyelflib {
namespace hex {

std::ostream& operator<<(std::ostream& out, const CyACDFileLine& line)
{
    return out << line.LineString();
}

/// <summary>
/// Creates a new instance of the CyPSoCACDFile class based on the given lines.
/// </summary>
/// <param name="devId">The device ID this ACD is matched with.</param>
/// <param name="devRev">The device rev this ACD is matched with.</param>
/// <param name="checksumType">The type of checksum to use with the corresponding bootloader.</param>
/// <param name="data">That lines that will be used to populate this file.</param>
CyPSoCACDFile::CyPSoCACDFile(unsigned int devId, uint8_t devRev, 
    uint8_t checksumType, const std::vector<CyACDFileLine *> &data) :
    m_data(data),
    m_deviceID(devId),
    m_deviceRev(devRev),
    m_checkSumType(checksumType)
{
}

/// <summary>
/// Write the cyacd file.
/// </summary>
/// <param name="writer">Writer to write cyacd file to.</param>
/// <returns>CyErr.OK if successful.</returns>
CyErr CyPSoCACDFile::Write(std::ofstream &writer)
{
    CyErr err;

    char msg[13];
    if (CheckSumType() != UNKNOWN_CHECKSUM_TYPE)
    {
        sprintf(msg, "%08X%02X%02X", DeviceID(), DeviceRev(), CheckSumType());
    }
    else
    {
        sprintf(msg, "%08X%02X", DeviceID(), DeviceRev());
    }
    writer << msg << std::endl;

    for (auto const line : m_data)
        writer << (*line) << '\n';

    return err;
}

/// <summary>
/// Read a cyacd file.
/// </summary>
/// <param name="path">Path of the file to read.</param>
/// <param name="acdFile">The acdFile that was read in</param>
/// <returns>CyErr.OK if successful.</returns>
CyErr CyPSoCACDFile::Read(const char *path, CyPSoCACDFile* &acdFile)
{
    CyErr err;
    std::vector<CyACDFileLine *> lines;
    unsigned int deviceId = 0;
    uint8_t deviceRev = 0, checksumType = 0;
    std::string lineBuf;
    std::ifstream reader(path);
    acdFile = NULL;

    CyACDFileLine *acdLine;

    getline(reader, lineBuf);

    if (lineBuf.size() > 0)
    {
        const char *lineChars = lineBuf.c_str();

        deviceId = ReadHex32(lineChars, 0);
        deviceRev = ReadHex8(lineChars, 8);

        if (lineBuf.size() > 10)
        {
            checksumType = ReadHex8(lineChars, 10);
        }
        else
        {
            checksumType = UNKNOWN_CHECKSUM_TYPE;
        }
    }
    do
    {
        getline(reader, lineBuf);

        if (lineBuf.size() > 0)
        {
            err = CyACDFileLine::Parse(lineBuf, acdLine);
            if (err.IsOK())
                lines.push_back(acdLine);
            else
                err = CyErr(L"Unable to parse ACD file.");
        }
    } while (err.IsOK() && lineBuf.size() != 0);
	reader.close();

    if (err.IsOK())
        acdFile = new CyPSoCACDFile(deviceId, deviceRev, checksumType, lines);
    else
    {
        for (size_t i = 0; i < lines.size(); i++)
            delete lines[i];
    }

    return err;
}

/// <summary>
/// Creates a new instance of CyACDFileLine from the base components that make up a record
/// </summary>
/// <param name="arrayId">The id of the flash array this line belongs to.</param>
/// <param name="lineNum">The line number in the array.</param>
/// <param name="data">The data conatined in the record</param>
CyACDFileLine::CyACDFileLine(uint8_t arrayId, uint16_t lineNum, const std::vector<uint8_t> &data)
{
    std::stringstream sb;
    char hexBuffer[5]; // longest value is %04X

    m_arrayID = arrayId;
    m_lineNum = lineNum;
    m_length = (uint16_t)data.size();
    m_data = data;

    m_checksum = (uint8_t)(m_length & 0xFF);
    m_checksum += (uint8_t)(m_length >> 8);
    m_checksum += (uint8_t)(lineNum & 0xFF);
    m_checksum += (uint8_t)(lineNum >> 8);
    m_checksum += (uint8_t)(arrayId & 0xFF);
    // m_checksum += (uint8_t)(arrayId >> 8);

    sb << LINE_IDENT;

    sprintf(hexBuffer, "%02X", arrayId);
    sb << hexBuffer;

    sprintf(hexBuffer, "%04X", lineNum);
    sb << hexBuffer;

    sprintf(hexBuffer, "%04X", m_length);
    sb << hexBuffer;

    for (unsigned int i = 0; i < data.size(); i++)
    {
        sprintf(hexBuffer, "%02X", data[i]);
        sb << hexBuffer;

        m_checksum += data[i];
    }

    m_checksum = (uint8_t)(CHECKSUM_INVERTER - m_checksum);

    sprintf(hexBuffer, "%02X", m_checksum);
    sb << hexBuffer;

    m_lineString = sb.str();
}

/// <summary>
/// Parses the provided line of data from a *.cyacd file and returns the parsed line in the form 
/// of an out parameter
/// </summary>
/// <param name="txtLine">The line of data to parse</param>
/// <param name="acdLine">The object representing the parsed line of data, or null if failed to parse</param>
/// <returns>CyErr indicating whether the input string was parsed successfully</returns>
CyErr CyACDFileLine::Parse(const std::string &txtLine, CyACDFileLine* &acdLine)
{
    CyErr err;
    acdLine = NULL;

    const char *txtChars = txtLine.c_str();

    uint8_t arrayID = ReadHex8(txtChars, 1);
    uint16_t lineNum = ReadHex16(txtChars, 3);
    uint16_t length = ReadHex16(txtChars, 7);
    uint16_t checksum = ReadHex8(txtChars, txtLine.length() - 2);

    size_t expectLength = length * 2 + 13;
    if (expectLength != txtLine.size())
    {
        err = CyErr(L"The length of the input line does not match the expected length.");
    }

    if (err.IsOK())
    {
        std::vector<uint8_t> data(length);
        std::string dataStr;
        for (int i = 0; i < length; i++)
        {
            data[i] = ReadHex8(txtChars, 11 + i * 2);
        }

        acdLine = new CyACDFileLine(arrayID, lineNum, data);

        if (acdLine->Checksum() != checksum)
        {
            err = CyErr(L"Checksum from line data does not match the computed checksum.");
            delete acdLine;
            acdLine = NULL;
        }
    }

    return err;
}

} // namespace hex
} // namespace cyelflib
