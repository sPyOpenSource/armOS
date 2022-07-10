/*
 *  CyElfTool: a command line tool for ELF file post-processing
 *  Copyright (C) 2013-2016 - Cypress Semiconductor Corp.
 * 
 *  CyElfTool is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CyElfTool is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public License
 *  along with CyElfTool; if not, write to the Free Software Foundation, 
 *  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *  Contact
 *  Email: customercare@cypress.com
 *  USPS:  Cypress Semiconductor
 *         198 Champion Court,
 *         San Jose CA 95134 USA
 */

#include "stdafx.h"
#include "cyelfcmd.h"
#include "elf/cyelfutil.h"
#include "elf/elfxx.h"
#include "elf2hex.h"
#include "cybootloaderutil.h"
#include "utils.h"
#include "commandline.h"
#include "hex/cypsochexfile.h"
#include "hex/cypsocacdfile.h"
#include <string>
#include <vector>
#include <utility>
#include <fstream>

using std::wstring;
using std::pair;
using std::vector;
using std::ofstream;
using cyelflib::hex::CyHexFile;
using cyelflib::hex::CyPsocHexFile;
using cyelflib::hex::CyPSoCACDFile;
using cyelflib::hex::CyACDFileLine;
using cyelflib::elf::CyElfFile;

namespace cyelftool {
namespace cmd {

static bool Overlaps(pair<uint32_t, uint32_t> p1, pair<uint32_t, uint32_t> p2)
{
    if (p1.second < p2.first)
        return false;
    if (p1.first > p2.second)
        return false;
    return true;
}

static CyErr ReadFlashData(CyElfFile &elf, uint32_t minAddr, uint32_t maxAddr, uint32_t rowSize,
    vector<vector<uint8_t> > &flashRows)
{
    CyErr err;
    // minAddr is inclusive and maxAddr is exclusive
    for (int i = 0; i < elf.PhdrCount(); i++)
    {
        const GElf_Phdr &phdr = elf.GetPhdr(i);
        uint32_t startAddr = (uint32_t)phdr.p_paddr;
        uint32_t endAddr = (uint32_t)(startAddr + phdr.p_filesz);
        if (phdr.p_type == PT_LOAD && phdr.p_filesz &&
            endAddr > minAddr && startAddr < maxAddr)
        {
            startAddr = std::max(startAddr, minAddr);
            endAddr = std::min(endAddr, maxAddr);
            uint32_t firstRow = startAddr / rowSize;
            uint32_t lastRow = (endAddr - 1) / rowSize;
            uint32_t addr = startAddr;
            for (uint32_t row = firstRow; row <= lastRow; row++)
            {
                uint32_t rowAddr = row * rowSize;
                uint32_t rowEnd = rowAddr + rowSize;
                if (flashRows.at(row).size() == 0)
                    flashRows.at(row).resize(rowSize);
                if ((err = elf.ReadPhdrData(&phdr,
                    addr - phdr.p_paddr,   // Offset
                    std::min(rowEnd - addr, endAddr - addr),    // Length
                    &(flashRows[row][addr % rowSize]))).IsNotOK())  // Destination
                    return err;
                addr = rowEnd;
            }
        }
    }
    return err;
}

static CyErr ReadSection(CyElfFile &elf, const char *name, vector<uint8_t> &data, uint32_t *addr = 0)
{
    CyErr err;
    if (addr)
        *addr = 0;
    Elf_Scn *scn = GetSectionEx(&elf, name);
    if (!scn)
        return err;
    GElf_Shdr shdr;
    if ((err = elf.GetShdr(scn, &shdr)).IsNotOK())
        return err;
    if (addr)
        *addr = (uint32_t)shdr.sh_addr;
    if (shdr.sh_type != SHT_NOBITS)
    {
        data.resize((uint32_t)shdr.sh_size);
        err = elf.ReadData(scn, 0, (uint32_t)shdr.sh_size, &data[0]);
    }
    else
        data.resize(0);
    return err;
}

void MultiAppMergeCommand(const CommandLine &cmd)
{
    CyElfFile app1(cmd.getInput(0));
    CyErr err(app1.Read());
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"%ls: %ls", app1.Path().c_str(), err.Message().c_str());
    CyElfFile app2(cmd.getInput(1));
    if ((err = app2.Read()).IsNotOK())
        errx(EXIT_FAILURE, "L%ls: %ls", app2.Path().c_str(), err.Message().c_str());

    const uint32_t flashRowSize = cmd.FlashRowSize();
    const uint32_t flashSize = cmd.FlashSize();
    static const uint32_t FLASH_PER_ECC = 8;
    //const uint32_t eccRowSize = flashRowSize / FLASH_PER_ECC;
    const uint32_t eccSize = flashSize / FLASH_PER_ECC;
    const uint32_t meta_addr = flashSize - CyPsocHexFile::ACD_META_DATA_LEN;

    pair<uint32_t, uint32_t> bounds1 = GetLoadableBounds(app1, flashRowSize, meta_addr);
    pair<uint32_t, uint32_t> bounds2 = GetLoadableBounds(app2, flashRowSize, meta_addr);

    if (Overlaps(bounds1, bounds2))
        errx(EXIT_FAILURE, L"%ls overlaps with %ls", app1.Path().c_str(), app2.Path().c_str());

    // Read data from ELF file
    vector<vector<uint8_t> > flashRows(flashSize / flashRowSize);
    if ((err = ReadFlashData(app1, 0, flashSize, flashRowSize, flashRows)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());
    if ((err = ReadFlashData(app2, 0, flashSize, flashRowSize, flashRows)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());

    uint32_t lastFlashRow = flashSize / flashRowSize - 1;
    uint8_t metaBase = (uint8_t)(flashRowSize - CyPsocHexFile::ACD_META_DATA_LEN);
    uint8_t activeAppBit = metaBase + 0x10u;
    vector<uint8_t> metaRow = flashRows[lastFlashRow];

    if (metaRow.size() == 0)
        errx(EXIT_FAILURE, L"%ls", "Not meta row found for app 1");

    metaRow[activeAppBit] = 1;

    flashRows[lastFlashRow] = metaRow;

    uint32_t eccAddr1 = 0, eccAddr2 = 0, eeAddr1 = 0; /*, eeAddr2 = 0;*/
    vector<uint8_t> eccData1;
    if ((err = ReadSection(app1, cyelflib::elf::CONFIGECC_SECTION, eccData1, &eccAddr1)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());
    vector<uint8_t> eccData2;
    if ((err = ReadSection(app2, cyelflib::elf::CONFIGECC_SECTION, eccData2, &eccAddr2)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());
    vector<uint8_t> cunvlData;
    if ((err = ReadSection(app1, cyelflib::elf::CUSTNVL_SECTION, cunvlData)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());
    vector<uint8_t> wonvlData;
    if ((err = ReadSection(app1, cyelflib::elf::WOLATCH_SECTION, wonvlData)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());
    vector<uint8_t> eeData1;
    if ((err = ReadSection(app1, cyelflib::elf::EEPROM_SECTION, eeData1, &eeAddr1)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());
	/*CDT 235269: This is duplicating the EEPROM section hence commented out */
	/*vector<uint8_t> eeData2;
    if ((err = ReadSection(app2, cyelflib::elf::EEPROM_SECTION, eeData2, &eeAddr2)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());*/
    vector<uint8_t> flashProtData;
    if ((err = ReadSection(app1, cyelflib::elf::FLASHPROT_SECTION, flashProtData)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());
    vector<uint8_t> chipProtData;
    if ((err = ReadSection(app1, cyelflib::elf::CHIPPROT_SECTION, chipProtData)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());
    vector<uint8_t> cyMetaData;
    if ((err = ReadSection(app1, cyelflib::elf::META_SECTION, cyMetaData)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());

    // Generate hex file
    CyHexFile hex;
    uint32_t checksum = 0;
    vector<uint8_t> emptyRow(flashRowSize);
    for (uint32_t row = 0; row < flashRows.size(); row++)
    {
        if (flashRows[row].size())
        {
            hex.AppendData(row * flashRowSize, &(flashRows[row][0]), flashRowSize);
            for (uint32_t i = 0; i < flashRows[row].size(); i++)
                checksum += flashRows[row][i];
        }
        else
        {
            hex.AppendData(row * flashRowSize, &emptyRow[0], flashRowSize);
        }
    }
    
    // ECC
    vector<uint8_t> eccData(eccSize);
    if (eccData1.size() &&
        eccAddr1 >= CyPsocHexFile::ADDRESS_CONFIG &&
        eccAddr1 < CyPsocHexFile::ADDRESS_CONFIG + eccSize)
    {
        memcpy(&eccData[eccAddr1 - CyPsocHexFile::ADDRESS_CONFIG],
            &eccData1[0],
            std::min(eccData1.size(), (size_t)(eccSize - (eccAddr1 - CyPsocHexFile::ADDRESS_CONFIG))));
    }
    if (eccData2.size() &&
        eccAddr2 >= CyPsocHexFile::ADDRESS_CONFIG &&
        eccAddr2 < CyPsocHexFile::ADDRESS_CONFIG + eccSize)
    {
        memcpy(&eccData[eccAddr2 - CyPsocHexFile::ADDRESS_CONFIG],
            &eccData2[0],
            std::min(eccData2.size(), (size_t)(eccSize - (eccAddr2 - CyPsocHexFile::ADDRESS_CONFIG))));
    }
    if (eccData1.size() || eccData2.size())
    {
        hex.AppendData(CyPsocHexFile::ADDRESS_CONFIG, &eccData[0], eccSize);
        for (uint32_t i = 0; i < eccData.size(); i++)
            checksum += eccData[i];
    }

    if (cunvlData.size())
        hex.AppendData(CyPsocHexFile::ADDRESS_CUSTNVLAT, &cunvlData[0], cunvlData.size());
    if (wonvlData.size())
        hex.AppendData(CyPsocHexFile::ADDRESS_WONVLAT, &wonvlData[0], wonvlData.size());
    if (eeData1.size())
        hex.AppendData(eeAddr1, &eeData1[0], eeData1.size());
	/*CDT 235269: This is duplicating the EEPROM section hence commented out */
	//if (eeData2.size() && eeAddr2 != eeAddr1)
    //    hex.AppendData(eeAddr2, &eeData2[0], eeData2.size());
    // Checksum
    uint8_t checksumSect[2];
    WriteBE16(checksumSect, (uint16_t)checksum);
    hex.AppendData(CyPsocHexFile::ADDRESS_CHECKSUM, checksumSect, sizeof(checksumSect));
    if (flashProtData.size())
        hex.AppendData(CyPsocHexFile::ADDRESS_FLASH_PROTECT, &flashProtData[0], flashProtData.size());
    if (cyMetaData.size() >= (size_t)CyPsocHexFile::CyPSoCHexMetaData::META_SIZE)
    {
        // Update checksum
        uint32_t siliconId = ReadBE32(&cyMetaData[CyPsocHexFile::CyPSoCHexMetaData::SILICONID_OFFSET]);
        uint32_t metaChecksum = checksum + siliconId;
        WriteBE32(&cyMetaData[CyPsocHexFile::CyPSoCHexMetaData::CHECKSUM_OFFSET], metaChecksum);
        hex.AppendData(CyPsocHexFile::ADDRESS_META, &cyMetaData[0], cyMetaData.size());
    }
    if (chipProtData.size())
    {
        // Always truncate to one byte--may be too big in ELF file
        chipProtData.resize(1);
        hex.AppendData(CyPsocHexFile::ADDRESS_CHIP_PROTECT, &chipProtData[0], chipProtData.size());
    }

    // Write hex file
    const std::wstring &hexFileName = cmd.PrimaryOutput();
#ifdef _MSC_VER
    _wunlink(hexFileName.c_str());
#else
    unlink(wstr_to_str(hexFileName).c_str());
#endif
#ifdef _MSC_VER
    ofstream hexFile(hexFileName.c_str());
#else
    ofstream hexFile(wstr_to_str(hexFileName).c_str());
#endif
    if (hexFile.fail())
        errx(EXIT_FAILURE, L"Unable to open %ls for writing.", hexFileName.c_str());
    if ((err = hex.Write(hexFile)).IsNotOK())
    {
        hexFile.close();
#ifdef _MSC_VER
        _wunlink(hexFileName.c_str());
#else
        unlink(wstr_to_str(hexFileName).c_str());
#endif
        errx(EXIT_FAILURE, L"Error writing %ls: %ls", hexFileName.c_str(), err.Message().c_str());
    }
    hexFile.close();
}

} // namespace cmd
} // namespace cyelftool
