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
#include "utils.h"
#include "hex/cypsochexfile.h"
#include "commandline.h"
#include "cybootloaderutil.h"
#include <iostream>
#include <fstream>
#include <vector>

using std::ofstream;
using std::wstring;
using std::endl;
using std::cout;
using std::vector;
using cyelflib::hex::CyPsocHexFile;
using cyelflib::elf::CyElfFile;

namespace cyelftool {
namespace cmd {

static bool DumpSection(CyElfFile &elf, const char *name)
{
    Elf_Scn *scn = GetSectionEx(&elf, name);
    if (scn)
    {
        Elf_Data *data = elf.GetFirstData(scn);
        if (data)
        {
            cout << name << ":";
            HexDump(cout, (const uint8_t *)data->d_buf, (uint32_t)data->d_size);
            cout << endl;
            return true;
        }
    }
    return false;
}

static void DumpCArray(std::ostream &os, const char *section, const char *name, const uint8_t data[], uint32_t size)
{
    os << "\n" <<
        "#if defined(__GNUC__) || defined(__ARMCC_VERSION)\n" <<
        "__attribute__ ((__section__(\"" << section << "\"), used))\n" <<
        "#elif defined(__ICCARM__)\n" <<
        "#pragma  location=\"" << section << "\"\n" <<
        "#else\n" <<
        "#error \"Unsupported toolchain\"\n" <<
        "#endif\n" <<
        "const uint8 " << name << "[] = {";
    for (uint32_t i = 0; i < size; i++)
    {
        if (i != 0)
            os << ',';
        if ((i % 8) == 0)
            os << "\n    0x";
        else if (i != 0)
            os << " 0x";
        char buf[4];
        sprintf(buf, "%02Xu", data[i]);
        os << buf;
    }
    os << "};\n";
}

void ExtractBootloaderCommand(const CommandLine &cmd)
{
    CyElfFile elf(cmd.PrimaryInput());
    CyErr err(elf.Read());
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"%ls: %ls", elf.Path().c_str(), err.Message().c_str());

    uint32_t bootloaderSize = GetBootloaderSize(&elf, cmd.FlashSize(), cmd.FlashRowSize());
    // Round up to nearest flash row
    uint32_t bootloaderRows = (bootloaderSize + cmd.FlashRowSize() - 1) / cmd.FlashRowSize();
    bootloaderSize = bootloaderRows * cmd.FlashRowSize();

    // Read bootloader flash
    vector<uint8_t> bootloaderFlash(bootloaderSize);
    wstring hexFileName = ReplaceExtension(elf.Path(), L".hex");
    CyPsocHexFile *hex;
    if ((err = CyPsocHexFile::Read(hexFileName, hex)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls: %ls", elf.Path().c_str(), err.Message().c_str());

    /* Due to the need to support backwards compatibility with bootloaders from prior to 3.0,
       We need to favor getting bootloader program data from the hex file, instead of the elf
       file. This is because prior to 3.0, the bootloader elf file did not contain valid values
       for the "bootloader size" and "bootloader checksum" variables that exist in the bootloader.
       However, the hex file did.
    */
    vector<uint8_t>* programVec = hex->ProgramData();
    assert(programVec->size() >= bootloaderSize);
    uint8_t* programData = &(*programVec)[0];
    for (uint32_t i = 0; i < bootloaderSize; i++)
    {
        bootloaderFlash[i] = programData[i];
    }

    // Dump bootloader metadata
    vector<uint8_t> blMetaData(CyPsocHexFile::ACD_META_DATA_LEN);
    if ((err = GetBootloaderMeta(elf, cmd.FlashSize(), cmd.FlashRowSize(), &blMetaData[0])).IsNotOK())
        errx(EXIT_FAILURE, "%ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Dump device meta data
    vector<uint8_t> metaData(CyPsocHexFile::MAX_SIZE_META);
    size_t devMetaBytesRead;
    if ((err = GetBootloaderData(elf, cyelflib::elf::META_SECTION, CyPsocHexFile::MAX_SIZE_META,
        &metaData[0], &devMetaBytesRead)).IsNotOK())
        errx(EXIT_FAILURE, "%ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Dump customer NVL values
    vector<uint8_t> custNvl(CyPsocHexFile::MAX_SIZE_CUSTNVLAT);
    size_t custNVLBytesRead;
    if ((err = GetBootloaderData(elf, cyelflib::elf::CUSTNVL_SECTION, CyPsocHexFile::MAX_SIZE_CUSTNVLAT,
        &custNvl[0], &custNVLBytesRead)).IsNotOK())
        errx(EXIT_FAILURE, "%ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Dump Write Once Latch value
    vector<uint8_t> woNvl(CyPsocHexFile::MAX_SIZE_WONVLAT);
    size_t woNVLBytesRead;
    if ((err = GetBootloaderData(elf, cyelflib::elf::WOLATCH_SECTION, CyPsocHexFile::MAX_SIZE_WONVLAT,
        &woNvl[0], &woNVLBytesRead)).IsNotOK())
        errx(EXIT_FAILURE, "%ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Dump EEPROM data
    vector<uint8_t> eeprom(CyPsocHexFile::MAX_SIZE_EEPROM);
    size_t eepromBytesRead;
    if ((err = GetBootloaderData(elf, cyelflib::elf::EEPROM_SECTION, CyPsocHexFile::MAX_SIZE_EEPROM,
        &eeprom[0], &eepromBytesRead)).IsNotOK())
        errx(EXIT_FAILURE, "%ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Dump Flash Protection data
    vector<uint8_t> flsProt(CyPsocHexFile::MAX_SIZE_FLASH_PROTECT);
    size_t flsProtBytesRead;
    if ((err = GetBootloaderData(elf, cyelflib::elf::FLASHPROT_SECTION, CyPsocHexFile::MAX_SIZE_FLASH_PROTECT,
        &flsProt[0], &flsProtBytesRead)).IsNotOK())
        errx(EXIT_FAILURE, "%ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Dump Chip Protection data
    vector<uint8_t> chipProt(CyPsocHexFile::MAX_SIZE_CHIP_PROTECT);
    size_t chipProtBytesRead;
    if ((err = GetBootloaderData(elf, cyelflib::elf::CHIPPROT_SECTION, CyPsocHexFile::MAX_SIZE_CHIP_PROTECT,
        &chipProt[0], &chipProtBytesRead)).IsNotOK())
        errx(EXIT_FAILURE, "%ls: %ls", elf.Path().c_str(), err.Message().c_str());

    ofstream outFlash("cybootloader.c");
    if (outFlash.fail())
        errx(EXIT_FAILURE, L"Error writing cybootloader.c");
    outFlash << "/* GENERATED CODE -- CHANGES WILL BE OVERWRITTEN */\n";
    outFlash << "\n";
    outFlash << "#include \"cytypes.h\"\n";
    outFlash << "\n";

    outFlash << "#if (!CYDEV_BOOTLOADER_ENABLE)\n";
    DumpCArray(outFlash, cyelflib::elf::LOADERMETA_SECTION, "cy_meta_loader", &blMetaData[0], blMetaData.size());
    outFlash << "#endif /* (!CYDEV_BOOTLOADER_ENABLE) */\n\n";

    DumpCArray(outFlash, cyelflib::elf::CYBOOTLOADER_SECTION, "cy_bootloader", &bootloaderFlash[0], bootloaderSize);
    if (devMetaBytesRead != 0)
        DumpCArray(outFlash, cyelflib::elf::META_SECTION, "cy_metadata", &metaData[0], devMetaBytesRead);
    if (custNVLBytesRead != 0)
        DumpCArray(outFlash, cyelflib::elf::CUSTNVL_SECTION, "cy_meta_custnvl", &custNvl[0], custNVLBytesRead);
    if (woNVLBytesRead != 0)
        DumpCArray(outFlash, cyelflib::elf::WOLATCH_SECTION, "cy_meta_wonvl", &woNvl[0], woNVLBytesRead);
    if (eepromBytesRead != 0)
        DumpCArray(outFlash, cyelflib::elf::EEPROM_SECTION, "cy_eeprom", &eeprom[0], eepromBytesRead);
    if (flsProtBytesRead != 0)
        DumpCArray(outFlash, cyelflib::elf::FLASHPROT_SECTION, "cy_meta_flashprotect", &flsProt[0], flsProtBytesRead);
    if (chipProtBytesRead != 0)
        DumpCArray(outFlash, cyelflib::elf::CHIPPROT_SECTION, "cy_meta_chipprotect", &chipProt[0], chipProtBytesRead);
    outFlash.close();

    ofstream outHeader("cybootloader.icf");
    if (outFlash.fail())
        errx(EXIT_FAILURE, L"Error writing cybootloader.icf");
    outHeader << "/* GENERATED CODE -- CHANGES WILL BE OVERWRITTEN */\n\n";
    char buf[10];
    sprintf(buf, "%08X", bootloaderSize);
    outHeader << "define symbol CYDEV_BTLDR_SIZE = 0x" << buf << ";\n";
    outHeader.close();

    cout << cyelflib::elf::LOADERMETA_SECTION << ":";
    HexDump(cout, &blMetaData[0], blMetaData.size());
    cout << endl;
    DumpSection(elf, cyelflib::elf::CUSTNVL_SECTION);
    DumpSection(elf, cyelflib::elf::WOLATCH_SECTION);
    DumpSection(elf, cyelflib::elf::EEPROM_SECTION);
    DumpSection(elf, cyelflib::elf::FLASHPROT_SECTION);
    DumpSection(elf, cyelflib::elf::CHIPPROT_SECTION);
    DumpSection(elf, cyelflib::elf::META_SECTION);
}

} // namespace cmd
} // namespace cyelftool
