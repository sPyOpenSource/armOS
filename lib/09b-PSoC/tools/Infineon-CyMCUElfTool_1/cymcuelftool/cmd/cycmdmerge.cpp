/*
 *  CyMCUElfTool, an ELF file post-processing utility
 *  Copyright (C) 2016-2017 - Cypress Semiconductor Corp.
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
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "cyelfcmd.h"
#include "elf/elfxx.h"
#include "commandline.h"
#include "elf/cyelfutil.h"
#include "cymcuelfutil.h"
#include "utils.h"

using std::endl;
using std::make_unique;
using std::set;
using std::map;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using std::wstring;
using std::wcout;
using cyelflib::elf::CyElfFile;
using cyelflib::elf::elf_section;
using namespace cyelflib;

namespace cymcuelftool {
namespace cmd {

void PerformElfFileMerge(
    const wstring &primaryInput, 
    const vector<wstring>::const_iterator inputBegin, 
    const vector<wstring>::const_iterator inputEnd, 
    const uint8_t fillValue,
    const wstring &primaryOutput)
{
    if ((inputBegin + 1) == inputEnd){
        errx(EXIT_FAILURE, L"Only one input file specified, nothing to be merged.");
    }

    file_delete(primaryOutput.c_str());
    if (!file_copy(primaryInput.c_str(), primaryOutput.c_str())){
        errx(EXIT_FAILURE, L"Failed to copy %ls to %ls", primaryInput.c_str(), primaryOutput.c_str());
    }
    CyElfFile output(primaryOutput);
    CyErr err = output.Read(true);
    if (err.IsNotOK()){
        output.Cleanup();
        file_delete(primaryOutput.c_str());
        errx(EXIT_FAILURE, L"%ls: %ls", primaryOutput.c_str(), err.Message().c_str());
    }
    set<elf_section> sections;  // Sections are ordered by start address
    unordered_set<std::string> secNames;    // For detecting duplicate names
    for (int i = 0; i < output.ShdrCount(); i++){
        secNames.insert(output.GetString(output.GetShdr(i).sh_name));
    }

    // Read input files and check for overlapping sections
    unordered_map<wstring, unique_ptr<CyElfFile>> inputFiles;
    for (auto it = inputBegin; it != inputEnd; ++it)
    {
        auto elf = make_unique<CyElfFile>(*it);
        err = elf->Read();
        if (err.IsOK()){
            err = AddSections(*elf, *it, sections);
        }
        if (err.IsNotOK()){
            output.Cleanup();
            file_delete(primaryOutput.c_str());
            errx(EXIT_FAILURE, L"%ls: %ls", it->c_str(), err.Message().c_str());
        }
        inputFiles.emplace(*it, std::move(elf));
    }
    if (err.IsOK()){
        err = CheckForOverlaps(sections);
    }

    // Copy section data to output file
    if (err.IsOK()){
        int secIdx = 0;
        for (auto const &src : sections)
        {
            if (err.IsOK() && src.m_path != primaryInput)
                err = DuplicateSection(*src.m_elf, src, output, secNames, &secIdx);
        }
    }

    if (err.IsOK()){
        err = output.ReadShdrs(); // happens here instead of elfxx.cpp:AddSection(). Otherwise n^2 complexity
    }

    uint32_t sectionCount = 0;
    if (err.IsOK()){
        for (auto const &sec : sections)
        {
            const CyElfFile &input = *sec.m_elf;
            if (SectionHasData(input, sec.m_scn))
                ++sectionCount;
        }
        // Allocaate phdr table before calling Update--its size affects file offsets
        err = output.NewPhdrTable(sectionCount);
    }

    // Update data offsets
    if (err.IsOK()){
        err = output.Update();
    }

    // Create phdrs for merged sections
    if (err.IsOK()){
        auto phdrs = MakePhdrsForSections(output, primaryInput, sections);
        if (phdrs.size() > sectionCount)
            err = CyErr(CyErr::FAIL, L"Expected %d phdrs, got %d", sectionCount, phdrs.size());
        for (size_t i = 0; i < phdrs.size() && err.IsOK(); ++i){
            err = output.SetPhdr(i, &phdrs[i]);
        }
    }

    if (err.IsOK()){
        output.DirtyPhdr();
    }

    // Write output file
    if (err.IsOK()){
        err = output.Write();
    }

    output.Cleanup();

    if (err.IsOK()){
        err = PopulateSectionsCymetaCychecksum(primaryOutput, fillValue);
    }

    output.Cleanup();

    if (err.IsNotOK()){  // CDT 267750
        file_delete(primaryOutput.c_str());
        errx(EXIT_FAILURE, L"%ls: %ls", primaryOutput.c_str(), err.Message().c_str());
    }
}

// Merge two elf binaries into one large application. Fails if any sections overlap.
void MergeElfFiles(const CommandLine &cmd)
{
    std::wstring outputFile = cmd.PrimaryOutput();

    GenerateInTempFile(outputFile, 
        [&cmd](const std::wstring & file) { 
        PerformElfFileMerge(cmd.PrimaryInput(), cmd.InputsBegin(), cmd.InputsEnd(), cmd.getFillValue(), file);
    });

    if (!cmd.HexFile().empty())
    {
        GenerateHexFile(outputFile, cmd.HexFile(), cmd.getFillValue());
    }
}

} // namespace cmd
} // namespace cymcuelftool
