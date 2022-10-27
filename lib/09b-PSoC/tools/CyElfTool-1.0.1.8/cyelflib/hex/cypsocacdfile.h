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

#ifndef CYPSOCACDFILE_H
#define CYPSOCACDFILE_H

#include <vector>
#include <fstream>
#include <string>
#include "cyerr.h"
#include "utils.h"

namespace cyelflib {
namespace hex {

/// <summary>
/// Represents a line from an Application Code and Data file. This class is immutable.
/// </summary>
class CyACDFileLine
{
private:
    static const uint16_t CHECKSUM_INVERTER = 0x100;
    static const char LINE_IDENT = ':';
   
    std::string m_lineString;
    uint16_t m_length;
    std::vector<uint8_t> m_data;
    uint8_t m_checksum;
    uint16_t m_lineNum;
    uint8_t m_arrayID;

public:
    /// <summary>
    /// Full hex record
    /// </summary>
    const std::string &LineString() const { return m_lineString; }
    /// <summary>
    /// The length parameter in the hex record
    /// </summary>
    const uint16_t Length() const { return m_length; }
    /// <summary>
    /// The data from the hex record
    /// </summary>
    const std::vector<uint8_t> &Data() const { return m_data; }
    /// <summary>
    /// The record checksum
    /// </summary>
    const uint8_t Checksum() const { return m_checksum; }
    /// <summary>
    /// The flash row number this file represents.
    /// </summary>
    const uint16_t LineNum() const { return m_lineNum; }
    /// <summary>
    /// The flash array id this line belongs to.
    /// </summary>
    const uint8_t ArrayID() const { return m_arrayID; }
    /// <summary>
    /// Creates a new instance of CyADCFileLine from the base components that make up a hex record
    /// </summary>
    /// <param name="arrayId">The id of the flash array this line belongs to.</param>
    /// <param name="lineNum">The line number in the array.</param>
    /// <param name="data">The data contained in the hex record</param>
    CyACDFileLine(uint8_t arrayId, uint16_t lineNum, const std::vector<uint8_t> &data);

    /// <summary>
    /// Parses the provided line of data from a *.cyacd file and returns the parsed line a passed by
    /// reference pointer to an allocated object
    /// </summary>
    /// <param name="txtLine">The line of data to parse</param>
    /// <param name="acdLine">The object representing the parsed line of data, or null if failed to parse</param>
    /// <returns>CyErr indicating whether the input string was parsed successfully</returns>
    static CyErr Parse(const std::string &txtLine, CyACDFileLine* &acdLine);

    /// <summary>
    /// Print out the line
    /// </summary>
    friend std::ostream& operator<< (std::ostream& stream, const CyACDFileLine& matrix);
};

/// <summary>
/// Represents the Application Code and Data file from a boot loadable file.
/// The file is stored MSB first, and has the following structure:
/// 
/// HEADER  [4-byte Silicon ID][1-byte Silicon Rev][1-byte Checksum Type]
/// DATA    [:][1-byte Array ID][2-byte Row Number][2-byte Length][n-byte Data][1-byte Checksum]
/// DATA    [:][1-byte Array ID][2-byte Row Number][2-byte Length][n-byte Data][1-byte Checksum]
/// ...
/// </summary>
class CyPSoCACDFile : noncopyable
{
private:
    std::vector<CyACDFileLine *> m_data;
    uint32_t m_deviceID;
    uint8_t m_deviceRev;
    uint8_t m_checkSumType;

public:
    /// <summary>
    /// This value is used to signify that we do not know what type of checksum to use for
    /// packet transmission.
    /// </summary>
    static const uint8_t UNKNOWN_CHECKSUM_TYPE = 0xFF;

    /// <summary>
    /// This is the silicon id for the target device
    /// </summary>
    const uint32_t DeviceID() const { return m_deviceID; }
    /// <summary>
    /// This is the revision of silicon for the target device
    /// </summary>
    const uint8_t DeviceRev() const { return m_deviceRev; }
    /// <summary>
    /// This is the type of checksum algorithm to use for packet transmission
    /// </summary>
    const uint8_t CheckSumType() const { return m_checkSumType; }

    /// <summary>
    /// Creates a new instance of the CyPSoCADCFile class based on the given lines.
    /// </summary>
    /// <param name="devId">The device ID this ACD is matched with.</param>
    /// <param name="devRev">The device rev this ACD is matched with.</param>
    /// <param name="checksumType">The type of checksum to use with the corresponding bootloader.</param>
    /// <param name="data">That lines that will be used to populate this file.</param>
    CyPSoCACDFile(unsigned int devId, uint8_t devRev, uint8_t checksumType, const std::vector<CyACDFileLine *> &data);

    /// <summary>
    /// Gets the collection of lines that make up the file
    /// </summary>
    const std::vector<CyACDFileLine *> &Lines() const { return m_data; }

    /// <summary>
    /// Write the cyacd file.
    /// </summary>
    /// <param name="writer">Stream to write hex file to.</param>
    /// <returns>CyErr.OK if successful.</returns>
    CyErr Write(std::ofstream &writer);

    /// <summary>
    /// Read a cyacd file.
    /// </summary>
    /// <param name="path">Path of the file to read.</param>
    /// <param name="acdFile">The acdFile that was read in</param>
    /// <returns>CyErr.OK if successful.</returns>
    static CyErr Read(const char *path, CyPSoCACDFile* &acdFile);
};

} // namespace hex
} // namespace cyelflib

#endif
