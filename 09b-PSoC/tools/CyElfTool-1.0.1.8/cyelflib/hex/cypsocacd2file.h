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
/// Represents a line from an Application Code and Data 2 file. This class is immutable.
/// </summary>
class CyACD2FileLine
{
private:
    static const char LINE_IDENT_DATA = ':';
    static const char LINE_IDENT_COMMENT = '#';
    static const char LINE_IDENT_META = '@';

    std::string m_lineString;
    std::vector<uint8_t> m_data;
    uint32_t m_address;
    bool m_isMeta;

public:
    /// <summary>
    /// Full hex record
    /// </summary>
    const std::string &LineString() const { return m_lineString; }
    /// <summary>
    /// The data from the hex record
    /// </summary>
    const std::vector<uint8_t> &Data() const { return m_data; }
    /// <summary>
    /// The memory address this line belongs to.
    /// </summary>
    const uint32_t Address() const { return m_address; }
    /// <summary>
    /// Creates a new instance of CyADC2FileLine from the base components that make up a hex record
    /// </summary>
    /// <param name="address">The memory address this line belongs to.</param>
    /// <param name="data">The data contained in the hex record</param>
    /// <param name="isMeta">Does the line of data correspond to meta data (true) or real data (false)</param>
    CyACD2FileLine(uint32_t address, const std::vector<uint8_t> &data, bool isMeta = false);
    /// <summary>
    /// Creates a new instance of metadata CyADC2FileLine
    /// </summary>
    /// <param name="marker">The unique string identifier for metadata name</param>
    /// <param name="data">The hex data contained in the metadata</param>
    /// <param name="isMeta">Does the line of data correspond to meta data (true) or real data (false)</param>
    CyACD2FileLine(const std::wstring &marker, const std::wstring &data, bool isMeta = true);

    /// <summary>
    /// Parses the provided line of data from a *.cyacd2 file and returns the parsed line a passed by
    /// reference pointer to an allocated object
    /// </summary>
    /// <param name="txtLine">The line of data to parse</param>
    /// <param name="acdLine">The object representing the parsed line of data, or null if failed to parse</param>
    /// <returns>CyErr indicating whether the input string was parsed successfully</returns>
    static CyErr Parse(const std::string &txtLine, std::unique_ptr<CyACD2FileLine> &acdLine);

    /// <summary>
    /// Print out the line
    /// </summary>
    friend std::ostream& operator<< (std::ostream& stream, const CyACD2FileLine& matrix);
};

/// <summary>
/// Represents the Application Code and Data 2 file from a boot loadable file.
/// The file is stored MSB first, and has the following structure:
/// 
/// HEADER  [1-byte Version][4-byte Silicon ID][1-byte Silicon Rev][1-byte Checksum Type][1-byte App ID][1-byte Product ID]
/// IV      [@][n-byte Data]
/// DATA    [:][4-byte Address][n-byte Data]
/// DATA    [:][4-byte Address][n-byte Data]
/// ...
/// </summary>
class CyPSoCACD2File : noncopyable
{
private:
    uint8_t m_version;
    uint32_t m_deviceID;
    uint8_t m_deviceRev;
    uint8_t m_checkSumType;
    uint8_t m_appId;
    uint32_t m_productId;
    std::vector<std::unique_ptr<CyACD2FileLine>> m_data;
    std::wstring m_initVec;
    std::wstring m_appInfo;

public:
    /// The version number for this file
    static const uint8_t VERSION = 1;

    /// <summary>
    /// This is the version of the file
    /// </summary>
    const uint8_t Version() const { return m_version; }
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
    /// This is the type of encryption initialization vector used for encryption
    /// </summary>
    const std::wstring InitVec() const { return m_initVec; }
    /// <summary>
    /// This is the application id the file corresponds to
    /// </summary>
    const uint8_t AppId() const { return m_appId; }
    /// <summary>
    /// This is the product id the file corresponds to
    /// </summary>
    const uint32_t ProductId() const { return m_productId; }
    /// <summary>
    /// This is the hex values of __cy_app_verify_start and __cy_app_verify_length, if present in elf
    /// </summary>
    const std::wstring AppInfo() const { return m_appInfo; }

    /// <summary>
    /// Creates a new instance of the CyPSoCAD2CFile class based on the given lines.
    /// </summary>
    /// <param name="version">The version of the file.</param>
    /// <param name="devId">The device ID this ACD is matched with.</param>
    /// <param name="devRev">The device rev this ACD is matched with.</param>
    /// <param name="checksumType">The type of checksum to use with the corresponding bootloader.</param>
    /// <param name="appId">The application id the file corresponds to.</param>
    /// <param name="productId">The product id the file corresponds to.</param>
    /// <param name="data">That lines that will be used to populate this file.</param>
    /// <param name="initVec">Optional initialization vector used for encryption</param>
    CyPSoCACD2File(uint8_t version, uint32_t devId, uint8_t devRev, uint8_t checksumType, uint8_t appId, uint32_t productId,
        std::vector<std::unique_ptr<CyACD2FileLine>> &&data, const std::wstring &initVec, const std::wstring &appInfo);

    /// <summary>
    /// Gets the collection of lines that make up the file
    /// </summary>
    const std::vector<std::unique_ptr<CyACD2FileLine>> &Lines() const { return m_data; }

    /// <summary>
    /// Write the cyacd2 file.
    /// </summary>
    /// <param name="writer">Stream to write hex file to.</param>
    /// <returns>CyErr.OK if successful.</returns>
    CyErr Write(std::ofstream &writer);
};

} // namespace hex
} // namespace cyelflib

#endif
