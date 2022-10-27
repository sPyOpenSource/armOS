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

#ifndef CYHEXFILE_H
#define CYHEXFILE_H

#include <string>
#include <vector>
#include "cyerr.h"
#include "utils.h"

namespace cyelflib {
namespace hex {

/// This enumeration represents the different types of records that can be
/// found in an Intel Hex file.
typedef enum
{
    /// represent the ASCII code for data bytes that make up a portion of a memory image.
    CYHEXRECORDTYPE_DATA = 0,
    /// specifies the end of the hexadecimal object file
    CYHEXRECORDTYPE_END_OF_FILE = 1,
    /// used to specify bits 4-19 of the Segment Base Address
    CYHEXRECORDTYPE_EXTENDED_SEGMENT_ADDRESS = 2,
    /// used to specify the execution start address for the object file
    CYHEXRECORDTYPE_START_SEGMENT_ADDRESS = 3,
    /// used to specify bits 16-31 of the Linear Base Address
    CYHEXRECORDTYPE_EXTENDED_LINEAR_ADDRESS = 4,
    /// used to specify the execution start address for the object file
    CYHEXRECORDTYPE_START_LINEAR_ADDRESS = 5
} CyHexRecordType;

class CyHexFile;

/// Represents a line from an Intel Hex file. This class is immutable.
class CyHexFileLine
{
private:
    static const uint16_t CHECKSUM_INVERTER = 0x100;

    static const char LINE_IDENT = ':';
    static const int LENGTH_LOC = 1;
    static const int OFFSET_LOC = LENGTH_LOC + 2;
    static const int TYPE_LOC = OFFSET_LOC + 4;
    static const int DATA_LOC = TYPE_LOC + 2;

    static const int EOF_LEN = 0;
    static const int START_SEG_LEN = 4;
    static const int START_LIN_LEN = 4;
    static const int EXTEND_SEG_LEN = 2;
    static const int EXTEND_LIN_LEN = 2;
    static const int RECORD_OVERHEAD = 11;

    std::string m_line;
    uint8_t m_length;
    uint32_t m_address;
    std::vector<uint8_t> m_data;
    CyHexRecordType m_type;
    uint8_t m_checksum;
    bool m_valid;
    uint16_t m_offset;

public:
    /// Full hex record
    const std::string &Line() const { return m_line; }

    /// The length parameter in the hex record
    uint8_t Length() const { return m_length; }

    /// Full address of beginning of record
    uint32_t Address() const { return m_address; }

    /// The data from the hex record
    const std::vector<uint8_t> &Data() const { return m_data; }

    /// The data from the hex record
    std::vector<uint8_t> &Data() { return m_data; }

    /// The type of record
    CyHexRecordType Type() const { return m_type; }

    /// The record checksum
    uint8_t Checksum() const { return m_checksum; }

    /// Indication whether the row of hex data is actually valid
    bool IsValid () const { return m_valid; }

    /// Offset of the data in the record
    uint16_t Offset() const { return m_offset; }

    /// Creates a new instance of the CyHexFileLine by parsing an actual record from a hex file
    ///
    /// \param line Line corresponding to an actual hex file record
    /// \param baseAddr High part of address. Should contain bits 19:4 for SBA or bits 31:16 for LBA.
    CyHexFileLine(const std::string &line, uint32_t baseAddr);

    /// Creates a new instance of CyHexFileLine from the base components that make up a hex record
    ///
    /// \param baseAddr High part of address. Should contain bits 19:4 for SBA or bits 31:16 for LBA.
    /// \param offset The address offset
    /// \param type The type of hex record
    /// \param data The data conatined in the hex record
    /// \param size The number of data bytes.
    CyHexFileLine(uint32_t baseAddr, uint16_t offset, CyHexRecordType type, const uint8_t *data, size_t size);

private:
    friend class CyHexFile;

    /// If the line's data has been changed then we need to recalculate the Line and Checksum values.
    void RecalculateLineInfo();

    static bool ValidTypeData(CyHexRecordType type, const std::vector<uint8_t> &data);
};

/// This class is used to parse or generate a full intel hex file.
class CyHexFile : noncopyable
{
private:
    /// This line indicates the end of an Intel hex file
    static const std::string EOF_LINE;

    /// List of lines in the hex file
    std::vector<CyHexFileLine *> m_data;

    bool SetValueAtAddress(uint32_t address, const uint8_t *valArr, size_t length);

protected:
    static CyErr ReadLines(const std::wstring &filename,
        std::istream &reader, std::vector<CyHexFileLine *> &data);

public:
    /// Creates a new instance of a CyHexFile from a list of hex records. Does not copy the list.
    explicit CyHexFile(std::vector<CyHexFileLine *> data);

    CyHexFile();

    virtual ~CyHexFile();

    /// returns a list of the hex file data
    const std::vector<CyHexFileLine*> &Data() const { return m_data; }

    /// Reads an actual Hex file and generates an instance of CyHexFile
    /// \param sourceFile The actual hex file on disk
    /// \param fileData The Parsed CyHexFile containing all of the data from the on disk hex file
    /// \return 0 if successful, otherwise an error
    static CyErr Read(const std::wstring &sourceFile, CyHexFile* &fileData);

    CyErr Append(CyHexFileLine *line);

    /// This method adds lines to the end of the file. It may be necessary to remove EOF lines first.
    CyErr Append(const std::vector<CyHexFileLine *> &lines);

    /// Appends raw data to the file. It may be necessary to remove EOF lines first.
    void AppendData(uint32_t baseAddress, const void *buf, size_t size);

    /// Removes EOF lines from the end of the file.
    void StripEof();

    /// This method calculates the checksum of the data lines.
    /// \return checksum of all data lines.
    uint16_t Checksum() const;

    /// <summary>
    /// This method calculates the checksum of the specified data lines.
    /// </summary>
    /// <param name="data">input lines</param>
    /// <returns>checksum of data lines.</returns>
    static uint16_t Checksum(const std::vector<CyHexFileLine *> &data);

    /// <summary>
    /// Find the minimum address that contains data.
    /// </summary>
    /// <returns>minimum address, uint32_t.MaxValue if no data.</returns>
    uint32_t FindMinAddress() const;

    /// <summary>
    /// Find the maximum address that contains data.
    /// </summary>
    /// <returns>maximum address, 0 if no data.</returns>
    uint32_t FindMaxAddress() const;

    /// <summary>
    /// Sets the given value at the given address in the hex file.
    /// </summary>
    /// <param name="address">The address of the value.</param>
    /// <param name="value">The value to set.</param>
    /// <param name="size">The size of the value in bytes.</param>
    /// <param name="isLittleEndian">true if the data should be encoded in little endian byte layout.</param>
    /// <returns>true if the value was successfully set. If false is returned then the address + size was
    /// greater than the maximum address in the file.</returns>
    bool SetValueAtAddress(uint32_t address, uint32_t value, uint32_t size, bool isLittleEndian);

    /// <summary>
    /// Get data from a section of the hex file
    /// </summary>
    /// <param name="startAddr">Beginning address of the section.</param>
    /// <param name="length">Length of the section.</param>
    /// <param name="pad">Filler byte for missing data.</param>
    /// <returns>Byte array containing data from hex file.</returns>
    uint8_t* ToByteArray(uint32_t startAddr, uint32_t length, uint8_t pad) const;

    /// <summary>
    /// Add an offset to all addresses. To ensure that extended address records are correct, SortRealign should be
    /// called after this method.
    /// </summary>
    /// <param name="offset"></param>
    void Rebase(uint32_t offset);

    /// <summary>
    /// Remove data from the hex file that isn't in the specified range. SortRealign should be called after this
    /// method to normalize data lines and remove unnecessary non-data lines.
    /// </summary>
    /// <param name="startAddr">Starting address</param>
    /// <param name="length">Length of data</param>
    void Truncate(uint32_t startAddr, uint32_t length);

    /// <summary>
    /// Sort the lines. Lines must have correct high address words. The lines will be rearranged
    /// such that each line begins on a 2^align boundary. If necessary, pad will be inserted as padding.
    /// </summary>
    /// <param name="align">Align beginning of lines to 2^align.</param>
    /// <param name="pad">Padding byte to add for alignment.</param>
    /// <param name="prevBase">Base address of previous line, 0 if none.</param>
    /// <returns>CyErr.OK on success.</returns>
    CyErr SortRealign(int align, uint8_t pad, uint32_t prevBase);

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

    CyErr SortRealign(int align, uint8_t pad, uint32_t prevBase, bool padAll, uint32_t startAddr, uint32_t length);

    /// <summary>
    /// This method writes the in memory representation of the hex file to disk
    /// </summary>
    /// <param name="writer">The destination stream</param>
    /// <returns> 0 if successful, otherwise an error</returns>
    CyErr Write(std::ofstream &writer) const;
};

} // namespace hex
} // namespace cyelflib

#endif
