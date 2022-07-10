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
#include <vector>
#include <fstream>
#include <string>
#include "cyhexfile.h"
#include "cypsochexfile.h"

namespace cyelflib {
namespace hex {

CyPsocHexFile::CyPSoCHexMetaData::CyPSoCHexMetaData(const std::vector<uint8_t> &data)
    : m_data(data)
{
    m_version = (uint16_t)((data[VERSION_OFFSET] << 8) | (data[VERSION_OFFSET+1]));
    m_siliconID = (uint32_t)(
        (data[SILICONID_OFFSET] << 24) | 
        (data[SILICONID_OFFSET+1] << 16) | 
        (data[SILICONID_OFFSET+2] << 8) | 
        (data[SILICONID_OFFSET+3]));

    m_siliconRev = data[SILICONREV_OFFSET];
    m_debugEnabled = data[DEBUGEN_OFFSET];
    m_checksum = (uint32_t)(
        (data[CHECKSUM_OFFSET] << 24) | 
        (data[CHECKSUM_OFFSET+1] << 16) | 
        (data[CHECKSUM_OFFSET+2] << 8) | 
        (data[CHECKSUM_OFFSET+3]));
}

CyPsocHexFile::CyPSoCHexMetaData::CyPSoCHexMetaData(uint16_t version, uint32_t siliconId, 
    uint8_t siliconRev, uint8_t debugEnabled, uint32_t checksum)
{
    m_version = version;
    m_siliconID = siliconId;
    m_siliconRev = siliconRev;
    m_debugEnabled = debugEnabled;
    m_checksum = checksum;

    m_data.resize(META_SIZE);

    m_data[VERSION_OFFSET] = (uint8_t)(m_version >> 8);
    m_data[VERSION_OFFSET + 1] = (uint8_t)m_version;

    m_data[SILICONID_OFFSET] = (uint8_t)(m_siliconID >> 24);
    m_data[SILICONID_OFFSET+1] = (uint8_t)(m_siliconID >> 16);
    m_data[SILICONID_OFFSET+2] = (uint8_t)(m_siliconID >> 8);
    m_data[SILICONID_OFFSET+3] = (uint8_t)m_siliconID;

    m_data[SILICONREV_OFFSET] = m_siliconRev;

    m_data[DEBUGEN_OFFSET] = m_debugEnabled;

    m_data[CHECKSUM_OFFSET] = (uint8_t)(m_checksum >> 24); 
    m_data[CHECKSUM_OFFSET+1] = (uint8_t)(m_checksum >> 16);
    m_data[CHECKSUM_OFFSET+2] = (uint8_t)(m_checksum >> 8);
    m_data[CHECKSUM_OFFSET+3] = (uint8_t)m_checksum;
}

CyPsocHexFile::BootloaderMetaData::BootloaderMetaData(const std::vector<uint8_t> &data, bool isLittleEndian)
    : m_data(data)
{
    m_checksum = data[0];

    if (isLittleEndian)
    {
        m_appAddress = (uint32_t)((data[1]) | (data[2] << 8) | (data[3] << 16) | (data[4] << 24));
        m_lastBootloaderRow = (uint32_t)((data[5]) | (data[6] << 8) | (data[7] << 16) | (data[8] << 24));
        m_appLength = (uint32_t)((data[9]) | (data[10] << 8) | (data[11] << 16) | (data[12] << 24));

        m_bootloaderBuiltVersion = (uint16_t)((data[18]) | (data[19] << 8));
        m_appID = (uint16_t)((data[20]) | (data[21] << 8));
        m_appVersion = (uint16_t)((data[22]) | (data[23] << 8));
        m_customID = (uint32_t)((data[24]) | (data[25] << 8) | (data[26] << 16) | (data[27] << 24));
    }
    else
    {
        m_appAddress = (uint32_t)((data[4]) | (data[3] << 8) | (data[2] << 16) | (data[1] << 24));
        m_lastBootloaderRow = (uint32_t)((data[8]) | (data[7] << 8) | (data[6] << 16) | (data[5] << 24));
        m_appLength = (uint32_t)((data[12]) | (data[11] << 8) | (data[10] << 16) | (data[9] << 24));

        m_bootloaderBuiltVersion = (uint16_t)((data[19]) | (data[18] << 8));
        m_appID = (uint16_t)((data[21]) | (data[20] << 8));
        m_appVersion = (uint16_t)((data[23]) | (data[22] << 8));
        m_customID = (uint32_t)((data[27]) | (data[26] << 8) | (data[25] << 16) | (data[24] << 24));
    }
}

static void memrev(std::vector<uint8_t> &data, int first, int len)
{
    std::vector<uint8_t>::iterator start;
    std::vector<uint8_t>::iterator finish;

    start = data.begin();
    std::advance(start, first);
    finish = start;
    std::advance(finish, len);
    std::reverse(start, finish);
}

CyPsocHexFile::BootloaderMetaData::BootloaderMetaData(bool isLittleEndian,
    uint8_t checkSum, uint32_t numBLLines, uint32_t entryAddr, 
    uint32_t validACDLenBytes, bool active, const std::vector<uint8_t> &verData)
{
    m_data.resize(ACD_META_DATA_LEN);

    // insert checksum
    m_data[0] = checkSum;

    // insert ACD entry address
    m_data[1] = (uint8_t)(entryAddr & 0xFF);
    m_data[2] = (uint8_t)(entryAddr >> 8);
    m_data[3] = (uint8_t)(entryAddr >> 16);
    m_data[4] = (uint8_t)(entryAddr >> 24);

    // insert bootloader size in lines
    m_data[5] = (uint8_t)(numBLLines & 0xFF);
    m_data[6] = (uint8_t)(numBLLines >> 8);
    m_data[7] = (uint8_t)(numBLLines >> 16);
    m_data[8] = (uint8_t)(numBLLines >> 24);

    // insert the count of the number of acd bytes
    m_data[9] = (uint8_t)(validACDLenBytes & 0xFF);
    m_data[10] = (uint8_t)(validACDLenBytes >> 8);
    m_data[11] = (uint8_t)(validACDLenBytes >> 16);
    m_data[12] = (uint8_t)(validACDLenBytes >> 24);

    m_data[16] = (uint8_t)(active ? 1 : 0);

    std::vector<uint8_t>::iterator val = m_data.begin();
    std::advance(val, VERSION_INFO_OFFSET);
    std::copy(verData.begin(), verData.end(), val);

    if (!isLittleEndian)
    {
        memrev(m_data, 1, 4);
        memrev(m_data, 5, 4);
        memrev(m_data, 9, 4);

        //Do not swizzle to verData that is passed in to us
        //memrev(m_data, 18, 2);
        //memrev(m_data, 20, 2);
        //memrev(m_data, 22, 2);
        //memrev(m_data, 24, 4);
    }
}

void CyPsocHexFile::BootloaderMetaData::ExtractBtldrData(const CyPsocHexFile &file, CyEndian endian,
    uint16_t &version, uint8_t &apps, uint8_t &checksum, bool &checksumEcc)
{
    std::vector<uint8_t> *flash = file.ProgramData();
    ExtractBtldrData(&(*flash)[flash->size() - ACD_META_DATA_LEN], endian, version, apps, checksum, checksumEcc);
    delete flash;
}

void CyPsocHexFile::BootloaderMetaData::ExtractBtldrData(const uint8_t *data, CyEndian endian,
    uint16_t &version, uint8_t &apps, uint8_t &checksum, bool &checksumEcc)
{
    const uint8_t *verData = &data[VERSION_INFO_OFFSET];

    version = (endian == CYENDIAN_LITTLE) ?
        (uint16_t)(verData[0] | (verData[1] << 8)) :
        (uint16_t)(verData[1] | (verData[0] << 8));
    apps = (0 == verData[2]) ? (uint8_t)1 : verData[2];
    checksum = verData[3];
    checksumEcc = (verData[4] == 1);
}

std::vector<uint8_t> *CyPsocHexFile::BootloaderMetaData::LayoutBtldrData(CyEndian endian, uint16_t version, 
    uint8_t checksum, uint8_t appImages, bool checksumEcc)
{
    std::vector<uint8_t> *data = new std::vector<uint8_t>(5);
    if (endian == CYENDIAN_LITTLE)
    {
        (*data)[0] = (uint8_t)(version);
        (*data)[1] = (uint8_t)(version >> 8);
    }
    else
    {
        (*data)[0] = (uint8_t)(version >> 8);
        (*data)[1] = (uint8_t)(version);
    }
    (*data)[2] = appImages;
    (*data)[3] = checksum;
    (*data)[4] = (uint8_t)(checksumEcc ? 1 : 0);

    return data;
}


/// <summary>
/// Gets the Program portion of the hex file
/// </summary>
std::vector<uint8_t> *CyPsocHexFile::ProgramData() const
{
    return GetSectionData(CyPsocHexFile::ADDRESS_PROGRAM, CyPsocHexFile::MAX_SIZE_PROGRAM);
}

/// <summary>
/// Gets the ECC Configuration portion of the hex file
/// </summary>
std::vector<uint8_t> *CyPsocHexFile::ConfigData() const
{
    return GetSectionData(CyPsocHexFile::ADDRESS_CONFIG, CyPsocHexFile::MAX_SIZE_CONFIG);
}

/// <summary>
/// Gets the Customer NVL portion of the hex file
/// </summary>
std::vector<uint8_t> *CyPsocHexFile::CustNvlData() const
{
    return GetSectionData(CyPsocHexFile::ADDRESS_CUSTNVLAT, CyPsocHexFile::MAX_SIZE_CUSTNVLAT);
}

/// <summary>
/// Gets the Write Once NVL portion of the hex file
/// </summary>
std::vector<uint8_t> *CyPsocHexFile::WoNvlData() const
{
    return GetSectionData(CyPsocHexFile::ADDRESS_WONVLAT, CyPsocHexFile::MAX_SIZE_WONVLAT);
} 
/// <summary>
/// Gets the EEPROM portion of the hex file
/// </summary>
std::vector<uint8_t> *CyPsocHexFile::EepromData() const
{
    return GetSectionData(CyPsocHexFile::ADDRESS_EEPROM, CyPsocHexFile::MAX_SIZE_EEPROM);
}

/// <summary>
/// Gets the file Checksum portion of the hex file
/// </summary>
std::vector<uint8_t> *CyPsocHexFile::ChecksumData() const
{
    return GetSectionData(CyPsocHexFile::ADDRESS_CHECKSUM, CyPsocHexFile::MAX_SIZE_CHECKSUM);
}

/// <summary>
/// Gets the Flash Protection portion of the hex file
/// </summary>
std::vector<uint8_t> *CyPsocHexFile::FlashProtectData() const
{
    return GetSectionData(CyPsocHexFile::ADDRESS_FLASH_PROTECT, CyPsocHexFile::MAX_SIZE_FLASH_PROTECT);
}

/// <summary>
/// Gets the Meta data portion of the hex file
/// </summary>
std::vector<uint8_t> *CyPsocHexFile::MetaData() const
{
    return GetSectionData(CyPsocHexFile::ADDRESS_META, CyPsocHexFile::MAX_SIZE_META);
}

/// <summary>
/// Gets the Chip Protection portion of the hex file
/// </summary>
std::vector<uint8_t> *CyPsocHexFile::ChipProtectData() const
{
    return GetSectionData(CyPsocHexFile::ADDRESS_CHIP_PROTECT, CyPsocHexFile::MAX_SIZE_CHIP_PROTECT);
}

/// <summary>
/// Creates a new PSoC Hex file
/// </summary>
/// <param name="data">The lines data to use to create the hex file</param>
CyPsocHexFile::CyPsocHexFile(std::vector<CyHexFileLine *> data)
    : CyHexFile(data)
{
}

std::vector<uint8_t> *CyPsocHexFile::GetSectionData(unsigned int startAddr, unsigned int maxSize) const
{
    std::vector<uint8_t> output(0x20);

    unsigned int endAddr = startAddr + maxSize - 1;
    unsigned int maxAddrFound = startAddr;

    std::vector<CyHexFileLine *>::const_iterator line;

    for (line = Data().begin(); line != Data().end(); ++line)
    {
        if ((*line)->Type() == CYHEXRECORDTYPE_DATA)
        {
            unsigned int lineStart = (*line)->Address();
            unsigned int lineEnd = (*line)->Address() + (*line)->Length() - 1;

            if (lineStart <= endAddr && lineEnd >= startAddr)
            {
                unsigned int begin = std::max(startAddr, lineStart);
                unsigned int end = std::min(lineEnd, endAddr);

                if (end >= maxAddrFound)
                    maxAddrFound = end + 1;

                if ((end - startAddr) > output.size())
                    output.resize(std::max(end - startAddr + 1, (unsigned int)(output.size() * 2)));

                for (unsigned int a = begin, i = a - lineStart, j = a - startAddr; a <= end; a++, i++, j++)
                    output[j] = (*line)->Data()[i];
            }
        }
    }

    output.resize((int)(maxAddrFound - startAddr));

    std::vector<uint8_t> *result = new std::vector<uint8_t>(output);
    return result;
}

/// <summary>
/// Read a hex file.
/// </summary>
/// <param name="sourceFile">File name.</param>
/// <param name="hexData">Output object.</param>
/// <returns>CyErr.OK on success.</returns>
CyErr CyPsocHexFile::Read(const std::wstring &sourceFile, CyPsocHexFile* &hexData)
{
    CyErr err;
    std::string lineBuf;
#ifdef _MSC_VER
    std::ifstream reader(sourceFile.c_str());
#else
    std::ifstream reader(wstr_to_str(sourceFile).c_str());
#endif

    std::vector<CyHexFileLine *> list;
    if (reader.fail())
        err = CyErr(CyErr::FAIL, L"Unable to read hex file %ls", sourceFile.c_str());

    if (err.IsOK())
        err = ReadLines(sourceFile, reader, list);
	reader.close();

    if (err.IsOK())
        hexData = new CyPsocHexFile(list);
    else
    {
        for (size_t i = 0; i < list.size(); i++)
            delete list[i];
    }

    return err;
}

} // namespace hex
} // namespace cyelflib
