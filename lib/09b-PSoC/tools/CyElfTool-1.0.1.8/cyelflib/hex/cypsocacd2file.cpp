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
#include "cypsocacd2file.h"

namespace cyelflib {
namespace hex {

std::ostream& operator<<(std::ostream& out, const CyACD2FileLine& line)
{
    return out << line.LineString();
}

CyPSoCACD2File::CyPSoCACD2File(uint8_t version, uint32_t devId, uint8_t devRev, uint8_t checksumType, uint8_t appId, 
    uint32_t productId, std::vector<std::unique_ptr<CyACD2FileLine>> &&data, const std::wstring &iv, const std::wstring &appInfo) :
    m_version(version),
    m_deviceID(devId),
    m_deviceRev(devRev),
    m_checkSumType(checksumType),
    m_appId(appId),
    m_productId(productId),
    m_data(std::move(data)),
    m_initVec(iv),
    m_appInfo(appInfo)
{
}

CyErr CyPSoCACD2File::Write(std::ofstream &writer)
{
    CyErr err;

    char msg[33];
    sprintf(msg, "%02X%08X%02X%02X%02X%08X", 
        Version(), m_deviceID, DeviceRev(), CheckSumType(), AppId(), SwapEndian32(ProductId()));
    writer << msg << std::endl;
    
    // Write metadata lines
    if (m_appInfo.length() > 0){
        writer << "@" << wstr_to_str(m_appInfo) << std::endl;
    }
    if (m_initVec.length() > 0){
        writer << "@EIV:" << wstr_to_str(m_initVec) << std::endl;
    }

    for (auto const &line : m_data)
        writer << (*line) << '\n';

    return err;
}

CyACD2FileLine::CyACD2FileLine(uint32_t address, const std::vector<uint8_t> &data, bool isMeta)
{
    std::stringstream sb;
    char hexBuffer[9]; // longest value is %08X

    m_address = address;
    m_data = data;
    m_isMeta = isMeta;

    sb << (isMeta ? LINE_IDENT_META : LINE_IDENT_DATA);

    sprintf(hexBuffer, "%08X", SwapEndian32(address));
    sb << hexBuffer;

    for (unsigned int i = 0; i < data.size(); i++)
    {
        sprintf(hexBuffer, "%02X", data[i]);
        sb << hexBuffer;
    }

    m_lineString = sb.str();
}

CyErr CyACD2FileLine::Parse(const std::string &txtLine, std::unique_ptr<CyACD2FileLine> &acdLine)
{
    CyErr err;
    acdLine = NULL;

    const char *txtChars = txtLine.c_str();

    bool isMeta = (LINE_IDENT_META == txtChars[0]);
    uint32_t address = SwapEndian32(ReadHex32(txtChars, 1));

    uint32_t length = (txtLine.size() - 9) / 2;
    std::vector<uint8_t> data(length);
    std::string dataStr;
    for (uint32_t i = 0; i < length; i++)
        data[i] = ReadHex8(txtChars, 9 + i * 2);

    acdLine = std::make_unique<CyACD2FileLine>(address, data, isMeta);

    return err;
}

} // namespace hex
} // namespace cyelflib
