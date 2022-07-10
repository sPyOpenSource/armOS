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

#ifndef INCLUDED_ELFXX_H
#define INCLUDED_ELFXX_H

#include <libelf/libelf.h>
#include <libelf/gelf.h>
#include <cassert>
#include <string>
#include <vector>
#include "utils.h"
#include "cyerr.h"

namespace cyelflib
{
namespace elf
{

#ifndef PT_ARM_EXIDX
#define PT_ARM_EXIDX 0x70000001
#endif
#ifndef SHT_ARM_EXIDX
#define SHT_ARM_EXIDX 0x70000001
#endif

/// \brief Allows reading and writing of 32-bit ELF files.
///
/// This class wraps libelf with a C++-style interface.
/// Use this class from a single thread as libelf is not thread-safe.
class CyElfFile : noncopyable
{
public:
    typedef std::vector<GElf_Phdr>::const_iterator phdr_iterator;
    typedef std::vector<GElf_Shdr>::const_iterator shdr_iterator;

    /// Creates an empty ELF file.
    explicit CyElfFile(const std::wstring &path);
    virtual ~CyElfFile();

    CyErr NewFile();
    CyErr Read(bool openForWriting = false);
    CyErr Write();

    /* This function cleans up the handle to the elf file owned by this instance.
       If you call this, you can no longer use the Write and similar functions.
    */
    void Cleanup();

    const std::wstring &Path() const { return m_path; }
    Elf *Handle() { return m_elf; }

    const GElf_Ehdr *GetEhdr() const { return &m_ehdr; }
    CyErr SetEhdr(const GElf_Ehdr *ehdr);

    int PhdrCount() const { return m_phdrs.size(); }
    const GElf_Phdr &GetPhdr(size_t index) const { return m_phdrs.at(index); }
    CyErr NewPhdrTable(size_t numPhdrs);
    CyErr SetPhdr(size_t index, GElf_Phdr *src);
    CyErr AddPhdr(const GElf_Phdr *src);
    phdr_iterator PhdrBegin() const { return m_phdrs.begin(); }
    phdr_iterator PhdrEnd() const { return m_phdrs.end(); }

    // Note: Section 0 is reserved (SHN_UNDEF).
    int ShdrCount() const { return m_shdrs.size(); }
    const GElf_Shdr &GetShdr(size_t index) const { return m_shdrs.at(index); }
    CyErr SetShdr(Elf_Scn *scn, GElf_Shdr *shdr);
    int GetShdr(const std::string &name, GElf_Shdr *dst) const;
    CyErr GetShdr(Elf_Scn *scn, GElf_Shdr *dst) const;
    shdr_iterator ShdrBegin() const { return m_shdrs.begin(); }
    shdr_iterator ShdrEnd() const { return m_shdrs.end(); }
    Elf_Scn *GetSection(size_t index) const;
    Elf_Scn *GetSection(const std::string  &name) const;

    // ReadShdrs() should be called after one or more invocations of AddSection()
    Elf_Scn *AddSection(const std::string &name, GElf_Word type, GElf_Addr address, Elf64_Xword flags, uint32_t align, size_t size);
    CyErr ReadShdrs();

    size_t GetShStrTabIdx() const { return m_shStrTabIdx; }
    const char *GetString(size_t offset) const;
    size_t AddString(const char *str);

    CyErr ReadData(Elf_Scn *scn, size_t offset, size_t length, void *buf) const;
    CyErr ReadPhdrData(const GElf_Phdr *phdr, void *buf) const;
    CyErr ReadPhdrData(const GElf_Phdr *phdr, Elf64_Off offset, uint32_t length, void *buf) const;
    Elf_Data *GetFirstData(Elf_Scn *scn) const;
    Elf_Data *GetNextData(Elf_Scn *scn, Elf_Data *data) const;

    CyErr Update();
    void DirtyElf();
    void DirtyEhdr();
    void DirtyPhdr();
    void DirtySection(Elf_Scn *scn);
    void DirtyShdr(Elf_Scn *scn);
    void DirtyData(Elf_Data *data);

    bool GetSymbol(int index, GElf_Sym *dst) const;
    bool GetSymbol(const char *name, GElf_Sym *dst) const;
    bool GetSymbol(Elf_Data* data, int index, GElf_Sym* dsy) const;
    std::string GetSymbolName(Elf_Data* strData, GElf_Sym* sym) const;
    bool IsInDataRange(Elf_Data* data, int index) const;

    /// Gets the endian of the ELF file. Assumes little endian if the file does not specify.
    CyEndian Endian() const { return m_ehdr.e_ident[EI_DATA] == ELFDATA2MSB ? CYENDIAN_BIG : CYENDIAN_LITTLE; }

private:
    std::wstring m_path;
    int m_file;
    Elf *m_elf;
    size_t m_shStrTabIdx;
    GElf_Ehdr m_ehdr;
    std::vector<GElf_Phdr> m_phdrs;
    std::vector<GElf_Shdr> m_shdrs;
    std::vector<void *> m_allocations;

    CyErr ReadPhdrs();
};

} // namespace elf
} // namespace cyelflib

#endif
