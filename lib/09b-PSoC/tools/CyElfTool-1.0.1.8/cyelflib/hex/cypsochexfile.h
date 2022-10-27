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

#ifndef CYPSOCHEXFILE_H
#define CYPSOCHEXFILE_H

#include <vector>
#include <string>
#include "cyhexfile.h"

namespace cyelflib {
namespace hex {

class CyPsocHexFile : public CyHexFile
{
private:
    std::vector<uint8_t>* GetSectionData(unsigned int startAddr, unsigned int maxSize) const;

public:
    /// <summary>
    /// This class represents the meta data section of the PSoC Hex file.  This meta data block
    /// contains information that is used to make sure the hex file is built for the intended device
    /// and has not been tampered with.
    /// </summary>
    class CyPSoCHexMetaData
    {
    private:
        std::vector<uint8_t> m_data;
        uint16_t m_version;
        uint32_t m_siliconID;
        uint8_t m_siliconRev;
        uint8_t m_debugEnabled;
        uint32_t m_checksum;

    public:
        static const int META_SIZE = 12;
        static const int VERSION_OFFSET = 0;
        static const int SILICONID_OFFSET = 2;
        static const int SILICONREV_OFFSET = 6;
        static const int DEBUGEN_OFFSET = 7;
        static const int CHECKSUM_OFFSET = 8;

        /// Gets the raw data that makes up the meta data section
        const std::vector<uint8_t> &Data() const { return m_data; }

        /// Gets the hex file version information
        const uint16_t Version() const { return m_version; }

        /// Gets the silicon ID of the target device
        const uint32_t SiliconID() const { return m_siliconID; }

        /// Gets the silicon revision of the target device
        const uint8_t SiliconRev() const { return m_siliconRev; }

        /// Gets whether or not the image in the hex file has debugging enabled
        const uint8_t DebugEnabled() const { return m_debugEnabled; }

        /// Gets the custom checksum used to prevent tampering
        const uint32_t Checksum() const { return m_checksum; }

        /// <summary>
        /// Creates a new instance of the CyPSoCHexMetaData class from the raw meta data
        /// </summary>
        /// <param name="data">The raw meta data to use to build the object</param>
        explicit CyPSoCHexMetaData(const std::vector<uint8_t> &data);

        /// <summary>
        /// Creates a new instance of the CyPSoCHexMetaData class from the individual settings
        /// </summary>
        /// <param name="version">The hex file version information</param>
        /// <param name="siliconId">The silicon ID of the target device</param>
        /// <param name="siliconRev">The silicon revision of the target device</param>
        /// <param name="debugEnabled">Whether or not the image in the hex file has debugging enabled</param>
        /// <param name="checksum">The custom checksum used to prevent tampering</param>
        CyPSoCHexMetaData(uint16_t version, unsigned int siliconId, uint8_t siliconRev, uint8_t debugEnabled, unsigned int checksum);
    };

    /// <summary>
    /// This class represents the bootloader meta data that is shared between bootloader and application.
    /// This block is used to ensure both images know critical details about the other.
    /// </summary>
    class BootloaderMetaData
    {
    private:
        std::vector<uint8_t> m_data;
        uint8_t m_checksum;
        unsigned int m_appAddress;
        unsigned int m_lastBootloaderRow;
        unsigned int m_appLength;
        uint16_t m_bootloaderBuiltVersion;
        uint16_t m_appID;
        uint16_t m_appVersion;
        unsigned int m_customID;

    public:
        /// This is the offset within the metadata where version information starts
        static const int VERSION_INFO_OFFSET = 18;
        static const int COPIER_OFFSET = 28;

        /// Gets the raw data that makes up the meta data section
        const std::vector<uint8_t> &Data() const { return m_data; }
        /// Gets the checksum of the bootloadable application
        const uint8_t Checksum() const { return m_checksum; }
        /// Gets the address within the application to start executing
        const unsigned int AppAddress() const { return m_appAddress; }
        /// Gets the last row of data consumed by the bootloader
        const unsigned int LastBootloaderRow() const { return m_lastBootloaderRow; }
        /// Gets the length of the application
        const unsigned int AppLength() const { return m_appLength; }
        /// Gets the version of the bootloader that the application was built against
        const uint16_t BootloaderBuiltVersion() const { return m_bootloaderBuiltVersion; }
        /// Gets the unique identifier for the application
        const uint16_t AppID() const { return m_appID; }
        /// Gets the version of the application
        const uint16_t AppVersion() const { return m_appVersion; }
        /// Gets the custom ID specified by the user
        const unsigned int CustomID() const { return m_customID; }

        /// Creates a new BootloaderMetaData object from the provided raw data
        ///
        /// \param data The bootloader meta data to use in consructing the object
        /// \param isLittleEndian Whether the data is in little endian or big endian
        BootloaderMetaData(const std::vector<uint8_t> &data, bool isLittleEndian);

        /// Creates a new BootloaderMetaData object from the provided distinct values
        ///
        /// \param isLittleEndian Whether the data is in little endian or big endian
        /// \param checkSum The checksum for the application
        /// \param numBLLines The number of rows occupied by the bootloader
        /// \param entryAddr The entry address of the application
        /// \param validACDLenuint8_ts The number of bytes used by the application
        /// \param active Indicates whether this application is active
        /// \param verData The version information
        BootloaderMetaData(bool isLittleEndian, uint8_t checkSum, uint32_t numBLLines, 
            uint32_t entryAddr, uint32_t validACDLenBytes, bool active, const std::vector<uint8_t> &verData);

        /// This method extracts the bootloader meta data out of the provided hex file and returns the values back.
        ///
        /// \param file The hex file to extract data from
        /// \param endian Specifies whether this is little or big endian
        /// \param version The user specified bootloader version
        /// \param apps The number of application images supported
        /// \param checksum The checksum type to use for the bootloader
        /// \param checksumEcc Whether the bootloadable checksums should be run over ecc too
        static void ExtractBtldrData(const CyPsocHexFile &file, CyEndian endian, uint16_t &version, uint8_t &apps, 
            uint8_t &checksum, bool &checksumEcc);

        /// This method extracts the bootloader meta data out of the provided flash row and returns the values.
        ///
        /// \param data The flash row containing the bootloader metadata.
        /// \param endian Specifies whether this is little or big endian
        /// \param version The user specified bootloader version
        /// \param apps The number of application images supported
        /// \param checksum The checksum type to use for the bootloader
        /// \param checksumEcc Whether the bootloadable checksums should be run over ecc too
        static void ExtractBtldrData(const uint8_t *data, CyEndian endian, uint16_t &version, uint8_t &apps, 
            uint8_t &checksum, bool &checksumEcc);

        /// Builds a uint8_t array out of the provided bootloader information
        ///
        /// \param endian Specifies whether this is little or big endian
        /// \param version The version of the bootloader
        /// \param checksum The communications checksum type for communicating
        /// \param appImages The number of application images to generate
        /// \param checksumEcc Whether the bootloadable checksums should be run over ecc too
        /// \return uint8_t array from the provided data
        static std::vector<uint8_t> *LayoutBtldrData(CyEndian endian, uint16_t version, uint8_t checksum,
            uint8_t appImages, bool checksumEcc);
    };

    /// from the cyhextool.cs
    static const unsigned int ACD_META_DATA_LEN = 64;
    /// The hex file revision used for PSoC3/5 parts
    static const uint16_t PSOC3_FILE_VERSION = 0x0001;
    /// The hex file revision used for MOS8 parts
    static const uint16_t LATEST_FILE_VERSION = 0x0002;

    // Default addresses from CAX-341/OPM-90
    /// The starting address of program data
    static const uint32_t ADDRESS_PROGRAM = 0x00000000;
    /// The starting address of ECC Configuration data
    static const uint32_t ADDRESS_CONFIG = 0x80000000;
    /// The starting address of Customer NVL data
    static const uint32_t ADDRESS_CUSTNVLAT = 0x90000000;
    /// The starting address of Write Once NVL data
    static const uint32_t ADDRESS_WONVLAT = 0x90100000;
    /// The starting address of EEPROM data
    static const uint32_t ADDRESS_EEPROM = 0x90200000;
    /// The starting address of the file Checksum
    static const uint32_t ADDRESS_CHECKSUM = 0x90300000;
    /// The starting address of the Flash Protection data
    static const uint32_t ADDRESS_FLASH_PROTECT = 0x90400000;
    /// The starting address of the Meta data
    static const uint32_t ADDRESS_META = 0x90500000;
    /// The starting address of the Chip Protection data
    static const uint32_t ADDRESS_CHIP_PROTECT = 0x90600000;

    // Absolute maximum sizes (hex file limits)
    /// The maximum size of the program data block
    static const unsigned int MAX_SIZE_PROGRAM = 0x80000000;
    /// The maximum size of the ECC Configuration data block
    static const unsigned int MAX_SIZE_CONFIG = 0x10000000;
    /// The maximum size of the Customer NVL data block
    static const unsigned int MAX_SIZE_CUSTNVLAT = 0x100000;
    /// The maximum size of the Write Once NVL data block
    static const unsigned int MAX_SIZE_WONVLAT = 0x100000;
    /// The maximum size of the EEPROM data block
    static const unsigned int MAX_SIZE_EEPROM = 0x100000;
    /// The maximum size of the file Checksum block
    static const unsigned int MAX_SIZE_CHECKSUM = 0x100000;
    /// The maximum size of the Flash Protection data block
    static const unsigned int MAX_SIZE_FLASH_PROTECT = 0x100000;
    /// The maximum size of the Meta data block
    static const unsigned int MAX_SIZE_META = 0x100000;
    /// The maximum size of the Chip Protection data block
    static const unsigned int MAX_SIZE_CHIP_PROTECT = 0x100000;

    /// <summary>
    /// Default value to pad flash rows.
    /// </summary>
    static const uint8_t PADDING_BYTE = 0x00;

    /// <summary>
    /// Gets the Program portion of the hex file
    /// </summary>
    std::vector<uint8_t> *ProgramData() const;

    /// <summary>
    /// Gets the ECC Configuration portion of the hex file
    /// </summary>
    std::vector<uint8_t> *ConfigData() const;

    /// <summary>
    /// Gets the Customer NVL portion of the hex file
    /// </summary>
    std::vector<uint8_t> *CustNvlData() const;

    /// <summary>
    /// Gets the Write Once NVL portion of the hex file
    /// </summary>
    std::vector<uint8_t> *WoNvlData() const;

    /// <summary>
    /// Gets the EEPROM portion of the hex file
    /// </summary>
    std::vector<uint8_t> *EepromData() const;

    /// <summary>
    /// Gets the file Checksum portion of the hex file
    /// </summary>
    std::vector<uint8_t> *ChecksumData() const;

    /// <summary>
    /// Gets the Flash Protection portion of the hex file
    /// </summary>
    std::vector<uint8_t> *FlashProtectData() const;

    /// <summary>
    /// Gets the Meta data portion of the hex file
    /// </summary>
    std::vector<uint8_t> *MetaData() const;

    /// <summary>
    /// Gets the Chip Protection portion of the hex file
    /// </summary>
    std::vector<uint8_t> *ChipProtectData() const;

    /// <summary>
    /// Creates a new PSoC Hex file
    /// </summary>
    /// <param name="data">The lines data to use to create the hex file</param>
    explicit CyPsocHexFile(std::vector<CyHexFileLine *> data);

    /// <summary>
    /// Read a hex file.
    /// </summary>
    /// <param name="sourceFile">File name.</param>
    /// <param name="hexData">Output object.</param>
    /// <returns>CyErr.OK on success.</returns>
    static CyErr Read(const std::wstring &sourceFile, CyPsocHexFile* &hexData);

};

} // namespace hex
} // namespace cyelflib

#endif
