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
#include "commandline.h"
#include "cyelfcmd.h"
#include "elf/cyelfutil.h"
#include "elf/elfxx.h"
#include "elf2hex.h"
#include "utils.h"
#include "hex/cypsochexfile.h"
#include "hex/cypsocacdfile.h"
#include "cybootloaderutil.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>

using std::wstring;
using std::vector;
using cyelflib::hex::CyPsocHexFile;
using cyelflib::hex::CyPSoCACDFile;
using cyelflib::hex::CyACDFileLine;
using std::ofstream;
using namespace cyelflib::elf;

namespace cyelftool {
namespace cmd {

/// \brief Post-build step for bootloadable applications.
///
/// 1. Adds checksum and hex metadata (same as --checksum option).
/// 2. Updates checksum, entry point, and size in .cyloadable1meta or .cyloadable2meta.
/// 3. Generates hex and cyacd files.
void BuiltBootloadableCommand(const CommandLine &cmd)
{
    CyElfFile elf(cmd.PrimaryInput());
    CyErr err(elf.Read(true));
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"Error reading %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    CyEndian endian = elf.Endian();

    uint32_t meta_addr;
    uint16_t blVersion;
    uint8_t blApps;
    uint8_t checksumType;
    bool checksumEcc;
    // Find .cyloadermeta section
    {
        Elf_Scn *loaderMeta = GetSectionEx(&elf, LOADERMETA_SECTION);
        if (loaderMeta == NULL)
            errx(EXIT_FAILURE, L"%ls not found in %ls", str_to_wstr(LOADERMETA_SECTION).c_str(), elf.Path().c_str());
        Elf_Data *blData = elf.GetFirstData(loaderMeta);
        if (!blData || blData->d_size < CyPsocHexFile::ACD_META_DATA_LEN)
            errx(EXIT_FAILURE, L"Bootloader metadata not found in %ls", elf.Path().c_str());
        uint8_t *blMeta = (uint8_t *)blData->d_buf;
        CyPsocHexFile::BootloaderMetaData::ExtractBtldrData(blMeta, elf.Endian(),
            blVersion, blApps, checksumType, checksumEcc);
    }

    uint8_t *metaRow;
    Elf_Data *data;
    {
        // Find .cyloadable[12]?meta section
        Elf_Scn *loadableMeta = GetSectionEx(&elf, LOADABLE2META_SECTION);
        if (!loadableMeta)
            loadableMeta = GetSectionEx(&elf, LOADABLE1META_SECTION);
        if (!loadableMeta)
            loadableMeta = GetSectionEx(&elf, LOADABLEMETA_SECTION);
        if (!loadableMeta)
            errx(EXIT_FAILURE, L"%ls not found in %ls", str_to_wstr(LOADABLEMETA_SECTION).c_str(), elf.Path().c_str());
        data = elf.GetFirstData(loadableMeta);
        if (!data || data->d_size < CyPsocHexFile::ACD_META_DATA_LEN)
            errx(EXIT_FAILURE, L"Bootloadable metadata not found in %ls", elf.Path().c_str());
        metaRow = (uint8_t *)data->d_buf;
        GElf_Shdr metaDataShdrInfo;
        err = elf.GetShdr(loadableMeta, &metaDataShdrInfo);
        if (err.IsNotOK())
            errx(EXIT_FAILURE, L"Failed to look up meta data section info in %ls. Error: %ls.", elf.Path().c_str(), err.Message().c_str());
        meta_addr = (uint32_t)metaDataShdrInfo.sh_addr;
    }

    // if we have a checksum exclude section, then we use that as our end of the range to checksum.
    // Also, store the size of this section, we will use that later to calculate the application size
    std::pair<uint32_t, uint32_t> checksumExcludeBounds = GetSectionStartEndAddr(&elf, CHECKSUM_EXCLUDE_SECTION);
    uint32_t checksumExcludeSize = checksumExcludeBounds.second - checksumExcludeBounds.first;
    uint32_t checksum_end_addr = checksumExcludeSize ? checksumExcludeBounds.first : meta_addr;


    //check to make sure the loadable section is not overlapping with meta-data
    if (LoadableMetaDataOverlap(elf, cmd.FlashRowSize(), meta_addr))
    {
        errx(EXIT_FAILURE, L"Metadata section overlap with bootloadable section");
    }

    std::pair<uint32_t, uint32_t> bounds = GetLoadableBounds(elf, cmd.FlashRowSize(), meta_addr);

    // Update bootloadable metadata (OPM-112)
    uint8_t appChecksum = ComputeBootloadableChecksum(elf,
        bounds.first * cmd.FlashRowSize(),
        std::min((bounds.second + 1) * cmd.FlashRowSize(), checksum_end_addr),  // End of last row
        0, checksumEcc);

    metaRow[0] = appChecksum;
    uint32_t entryPoint = (uint32_t)elf.GetEhdr()->e_entry;
    WriteWithEndian32(endian, &metaRow[1], entryPoint);
    uint32_t firstLoadableRow = bounds.first;
    WriteWithEndian32(endian, &metaRow[5], firstLoadableRow - 1); // subtract one from the row to get row before

    // don't include the last N bytes of non-checksummed data in the application size
    uint32_t applicationSize = (bounds.second - bounds.first + 1u) * cmd.FlashRowSize();

    if (checksumEcc)
    {
        Elf_Scn *eccScn = GetSectionEx(&elf, CONFIGECC_SECTION);
        if (eccScn)
        {
            GElf_Shdr eccShdr;
            if ((err = elf.GetShdr(eccScn, &eccShdr)).IsNotOK())
                return;
            uint32_t eccStartAddr = (uint32_t)((eccShdr.sh_addr - CyPsocHexFile::ADDRESS_CONFIG) * 8);
            uint32_t eccEndAddr = eccStartAddr + ((uint32_t)(eccShdr.sh_size * 8));
            uint32_t flashStartAddr = bounds.first * cmd.FlashRowSize();
            uint32_t flashEndAddr = (bounds.second + 1) * cmd.FlashRowSize();
            applicationSize = (uint32_t)(std::max (flashEndAddr, eccEndAddr) - std::min(flashStartAddr, eccStartAddr));

            applicationSize = (uint32_t)(applicationSize + cmd.FlashRowSize() - 1) / cmd.FlashRowSize() * cmd.FlashRowSize();
        }        
    }

    WriteWithEndian32(endian, &metaRow[9], applicationSize);
    WriteWithEndian16(endian, &metaRow[0x12], blVersion);
    elf.DirtyData(data);

    if ((err = AdjustHeaders(&elf)).IsNotOK())
        errx(EXIT_FAILURE, L"Failed to remove unused phdrs from %ls", elf.Path().c_str());

    if ((err = elf.Write()).IsNotOK())
        errx(EXIT_FAILURE, L"Error writing %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Compute and insert the regular checksums *after* the .cyloadable1/2meta has been updated
    uint32_t checksum = ComputeChecksumFromElf(elf, true);
    if ((err = StoreMetaChecksums(&elf, checksum)).IsNotOK())
        errx(EXIT_FAILURE, L"Failed to store checksums in %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    if ((err = AdjustHeaders(&elf)).IsNotOK())
        errx(EXIT_FAILURE, L"Failed to update program headers: %ls", err.Message().c_str());

    if ((err = elf.Write()).IsNotOK())
        errx(EXIT_FAILURE, L"Error writing %ls: %ls", elf.Path().c_str(), err.Message().c_str());

    // Generate ACD data
    const wstring acdFileName = ReplaceExtension(elf.Path(), L".cyacd");
    vector<CyACDFileLine *> acdLines;
    if (GetSectionEx(&elf, EEPROM_SECTION))
    {
        if (cmd.EepromRowSize() == 0)
            errx(EXIT_FAILURE, L"--ee_row_size is required");
        if (cmd.EepromArraySize() == 0)
            errx(EXIT_FAILURE, L"--ee_array_size is required");
        if (cmd.EepromArray() == 0)
            errx(EXIT_FAILURE, L"--ee_array is required");
    }
    // TODO: don't include EEPROM if it was provided by the bootloader
    err = elf2acd(cmd.FlashSize(), cmd.FlashArraySize(), cmd.FlashRowSize(), meta_addr,
        true, cmd.EepromRowSize(), cmd.EepromArraySize(), cmd.EepromArray(), cmd.OffsetBootloadable(), elf, acdLines);
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"Failed to generate %ls: %ls", acdFileName.c_str(), err.Message().c_str());
    CyPsocHexFile::CyPSoCHexMetaData *meta;
    if ((err = LoadCyMetaSection(elf, &meta)).IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());
    CyPSoCACDFile acd(meta->SiliconID(), meta->SiliconRev(), checksumType, acdLines);
    delete meta;

    // Write ACD file
    file_delete(acdFileName.c_str());
#ifdef _MSC_VER
    ofstream acdFile(acdFileName.c_str());
#else
    ofstream acdFile(wstr_to_str(acdFileName).c_str());
#endif
    if (acdFile.fail())
        errx(EXIT_FAILURE, L"Unable to open %ls for writing.", acdFileName.c_str());
    if ((err = acd.Write(acdFile)).IsNotOK())
    {
        acdFile.close();
        file_delete(acdFileName.c_str());
        errx(EXIT_FAILURE, L"Error writing %ls: %ls", acdFileName.c_str(), err.Message().c_str());
    }
    acdFile.close();

    // Generate hex file
    wstring hexFileName = ReplaceExtension(elf.Path(), L".hex");
    err = elf2hex(elf, hexFileName, cmd.FlashSize(), cmd.FlashOffset(), cmd.getFillValue());
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"Failed to generate %ls: %ls", hexFileName.c_str(), err.Message().c_str());
}

} // namespace cmd
} // namespace cyelftool
