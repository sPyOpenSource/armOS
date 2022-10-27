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
#include "elfxx.h"
#include "cyelfutil.h"
#include "utils.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>

namespace cyelflib {
namespace elf {

CyElfFile::CyElfFile(const std::wstring &path) :
    m_path(path),
    m_file(-1),
    m_elf(0)
{
}

CyElfFile::~CyElfFile()
{
    Cleanup();
}

void CyElfFile::Cleanup()
{
    if (m_elf != 0)
    {
        elf_end(m_elf);
        m_elf = 0;
    }
    if (m_file != -1)
    {
        close(m_file);
        m_file = -1;
    }
    for (void *p : m_allocations)
        free(p);
    m_allocations.clear();
}

CyErr CyElfFile::NewFile()
{
    assert(m_elf == 0);

    if (elf_version(EV_CURRENT) == EV_NONE)
        return CyErr(CyErr::FAIL, L"ELF library initialization failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
#ifdef _MSC_VER
    if ((m_file = _wopen(m_path.c_str(), _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE)) < 0)
#else
    if ((m_file = open(wstr_to_str(m_path).c_str(), O_RDWR, 0644)) < 0)
#endif
        return CyErr(CyErr::FAIL, L"Failed to open %ls", m_path.c_str());

    if ((m_elf = elf_begin(m_file, ELF_C_WRITE, NULL)) == NULL)
    {
        Cleanup();
        return CyErr(CyErr::FAIL, L"elf_begin() failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
    }

    return CyErr();
}

CyErr CyElfFile::Read(bool openForWriting/* = false*/)
{
    assert(m_elf == 0);
    CyErr err;

    if (elf_version(EV_CURRENT) == EV_NONE)
        return CyErr(CyErr::FAIL, L"ELF library initialization failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());

#ifdef _MSC_VER
    int openFlags = _O_BINARY | (openForWriting ? _O_RDWR : _O_RDONLY);
    if ((m_file = _wopen(m_path.c_str(), openFlags, 0)) < 0)
#else
    int openFlags = openForWriting ? O_RDWR : O_RDONLY;
    if ((m_file = open(wstr_to_str(m_path).c_str(), openFlags, 0)) < 0)  // TODO: need to convert to multibyte
#endif
        return CyErr(CyErr::FAIL, L"Failed to open %ls", m_path.c_str());

    if ((m_elf = elf_begin(m_file, openForWriting ? ELF_C_RDWR : ELF_C_READ, NULL)) == NULL)
    {
        err = CyErr(CyErr::FAIL, L"elf_begin() failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
        Cleanup();
        return err;
    }

    if (elf_kind(m_elf) != ELF_K_ELF)
    {
        err = CyErr(CyErr::FAIL, L"%ls is not an ELF file", m_path.c_str());
        Cleanup();
        return err;
    }

    if (gelf_getehdr(m_elf, &m_ehdr) != &m_ehdr)
    {
        err = CyErr(CyErr::FAIL, L"gelf_getehdr() failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
        Cleanup();
        return err;
    }

    err = ReadPhdrs();
    if (err.IsNotOK())
        return err;

    err = ReadShdrs();
    if (err.IsNotOK())
        return err;

    if (elf_getshdrstrndx(m_elf, &m_shStrTabIdx) < 0)
    {
        err = CyErr(CyErr::FAIL, L"gelf_getshdrstrndx() failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
        Cleanup();
        return err;
    }

    return err;
}

CyErr CyElfFile::ReadPhdrs()
{
    size_t phdrCount = 0;
    if (elf_getphdrnum(m_elf, &phdrCount) < 0)
        return CyErr(CyErr::FAIL, L"elf_getphdrnum() failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());

    m_phdrs.resize(phdrCount);
    for (size_t i = 0; i < phdrCount; i++)
    {
        GElf_Phdr *phdr = &(m_phdrs[i]);
        if (gelf_getphdr(m_elf, i, phdr) < (GElf_Phdr*)0)
            return CyErr(CyErr::FAIL, L"gelf_getphdr(%u) failed: %s", i, str_to_wstr(elf_errmsg(-1)).c_str());
    }

    return CyErr();
}

CyErr CyElfFile::ReadShdrs()
{
    size_t shdrCount = 0;
    if (elf_getshdrnum(m_elf, &shdrCount) < 0)
        return CyErr(CyErr::FAIL, L"elf_getshdrnum() failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());

    m_shdrs.resize(shdrCount);
    for (size_t i = 0; i < shdrCount; i++)
    {
        Elf_Scn *scn = elf_getscn(m_elf, i);
        if (scn == 0)
            return CyErr(CyErr::FAIL, L"elf_getscn(%u) failed: %s", i, str_to_wstr(elf_errmsg(-1)).c_str());
        GElf_Shdr *shdr = &(m_shdrs[i]);
        if (gelf_getshdr(scn, shdr) != shdr)
            return CyErr(CyErr::FAIL, L"gelf_getshdr(%u) failed: %s", i, str_to_wstr(elf_errmsg(-1)).c_str());
    }

    return CyErr();
}

CyErr CyElfFile::Write()
{
    assert(m_elf != 0);
    if (elf_update(m_elf, ELF_C_WRITE) < 0)
        return CyErr(CyErr::FAIL, L"%ls: elf_update() failed: %ls",
            m_path.c_str(), str_to_wstr(elf_errmsg(-1)).c_str());
    return CyErr();
}

CyErr CyElfFile::SetEhdr(const GElf_Ehdr *ehdr)
{
    if (gelf_update_ehdr(m_elf, const_cast<GElf_Ehdr *>(ehdr)) == 0)
        return CyErr(CyErr::FAIL, L"%ls: gelf_update_ehdr() failed: %ls",
            m_path.c_str(), str_to_wstr(elf_errmsg(-1)).c_str());
    DirtyEhdr();
    return CyErr();
}

CyErr CyElfFile::NewPhdrTable(size_t numPhdrs)
{
    if (!gelf_newphdr(m_elf, numPhdrs) &&  numPhdrs != 0)
        return CyErr(CyErr::FAIL, L"%ls: gelf_newphdr() failed: %ls",
            m_path.c_str(), str_to_wstr(elf_errmsg(-1)).c_str());
    ReadPhdrs();
    return CyErr();
}

CyErr CyElfFile::SetPhdr(size_t index, GElf_Phdr *src)
{
    m_phdrs.at(index) = *src;
    if (gelf_update_phdr(m_elf, index, src) == 0)
        return CyErr(CyErr::FAIL, L"gelf_update_phdr(%u) failed: %s", index, str_to_wstr(elf_errmsg(-1)).c_str());
    return CyErr();
}

CyErr CyElfFile::AddPhdr(const GElf_Phdr *src)
{
    if (!gelf_newphdr(m_elf, m_phdrs.size() + 1))
        return CyErr(CyErr::FAIL, L"gelf_newphdr failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
    m_phdrs.push_back(*src);
    CyErr err;
    for (size_t i = 0; i < m_phdrs.size() && err.IsOK(); i++)
    {
        if (gelf_update_phdr(m_elf, i, &m_phdrs.at(i)) == 0)
            err = CyErr(CyErr::FAIL, L"gelf_update_phdr(%u) failed: %s", i, str_to_wstr(elf_errmsg(-1)).c_str());
    }
    return CyErr();
}

int CyElfFile::GetShdr(const std::string &name, GElf_Shdr *dst) const
{
    for (size_t i = 0; i < m_shdrs.size(); i++)
    {
        std::string sh_name = GetString(m_shdrs[i].sh_name);

        if (0 == CompareSecnNameEx(sh_name, name)) // Fix JIRA CYELFTOOL-10
        {
            *dst = m_shdrs[i];
            return i;
        }
    }
    return -1;
}

CyErr CyElfFile::GetShdr(Elf_Scn *scn, GElf_Shdr *dst) const
{
    if (gelf_getshdr(scn, dst) != dst)
        return CyErr(CyErr::FAIL, L"gelf_getshdr failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
    return CyErr();
}

CyErr CyElfFile::SetShdr(Elf_Scn *scn, GElf_Shdr *shdr)
{
    if (gelf_update_shdr(scn, shdr) == 0)
        return CyErr(CyErr::FAIL, L"gelf_update_shdr failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
    elf_flagshdr(scn, ELF_C_SET, ELF_F_DIRTY);
    return CyErr();
}

Elf_Scn *CyElfFile::GetSection(size_t index) const
{
    return elf_getscn(m_elf, index);
}

Elf_Scn *CyElfFile::GetSection(const std::string &name) const
{
    GElf_Shdr shdr;
    int idx = GetShdr(name, &shdr);
    if (idx < 0)
        return 0;
    return GetSection((size_t)idx);
}

Elf_Scn *CyElfFile::AddSection(const std::string &name, GElf_Word type, GElf_Addr address, Elf64_Xword flags,
    uint32_t align, size_t size)
{
    Elf_Scn *scn = elf_newscn(m_elf);
    if (!scn)
        errx(EXIT_FAILURE, "elf_newscn failed: %s", elf_errmsg(-1));

    if (type != SHT_NOBITS)
    {
        Elf_Data *data = elf_newdata(scn);
        if (0 == data)
            errx(EXIT_FAILURE, "elf_newdata failed: %s", elf_errmsg(-1));
        data->d_align = align;
        data->d_off = 0;
        data->d_buf = calloc(size, 1);
        if (!data->d_buf)
            errx(EXIT_FAILURE, "calloc failed");
        m_allocations.push_back(data->d_buf);
        data->d_size = size;
        data->d_type = ELF_T_BYTE; 
        data->d_version = EV_CURRENT;
    }

    // Update section header
    GElf_Shdr shdr;
    if (gelf_getshdr(scn, &shdr) != &shdr)
        errx(EXIT_FAILURE, "gelf_getshdr failed: %s", elf_errmsg(-1));
    shdr.sh_name = AddString(name.c_str());
    shdr.sh_type = type;
    shdr.sh_addr = address;
    shdr.sh_size = size;
    shdr.sh_flags = flags;
    shdr.sh_addralign = align;
    shdr.sh_entsize = 0;
    if (gelf_update_shdr(scn, &shdr) < 0)
        errx(EXIT_FAILURE, "gelf_update_shdr failed: %s", elf_errmsg(-1));

    // Apply updates to in memory version of ELF
    if (elf_update(m_elf, ELF_C_NULL) < 0)
        errx(EXIT_FAILURE, "elf_update() failed: %s", elf_errmsg(-1));

    return scn;
}

Elf_Data *CyElfFile::GetFirstData(Elf_Scn *scn) const
{
    return elf_getdata(scn, 0);
}

Elf_Data *CyElfFile::GetNextData(Elf_Scn *scn, Elf_Data *data) const
{
    return elf_getdata(scn, data);
}

CyErr CyElfFile::ReadData(Elf_Scn *scn, size_t offset, size_t length, void *buf) const
{
    uint8_t *buf8 = (uint8_t *)buf;
    memset(buf8, 0, length);

    GElf_Shdr shdr;
    if (gelf_getshdr(scn, &shdr) != &shdr)
        return CyErr(CyErr::FAIL, L"gelf_getshdr failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());

    Elf_Data *data = nullptr;
    while (0 != (data = elf_getdata(scn, data)) && data->d_buf)
    {
        if ((size_t)data->d_off >= offset + length || (size_t)data->d_off + data->d_size <= offset)
            continue;
        const uint8_t *src = (const uint8_t *)data->d_buf;
        size_t count = data->d_size;
        size_t srcOff = data->d_off;
        if (srcOff < offset)
        {
            size_t diff = offset - srcOff;
            srcOff += diff;
            src += diff;
            count -= diff;
        }
        if (srcOff + count > offset + length)
        {
            size_t diff = (srcOff + count) - (offset + length);
            count -= diff;
        }
        memcpy(&buf8[srcOff - offset], src, count);
    }

    return CyErr();
}

CyErr CyElfFile::ReadPhdrData(const GElf_Phdr *phdr, void *buf) const
{
    return ReadPhdrData(phdr, 0, (uint32_t)phdr->p_filesz, buf);
}

CyErr CyElfFile::ReadPhdrData(const GElf_Phdr *phdr, Elf64_Off offset, uint32_t length, void *buf) const
{
    if (offset >= phdr->p_filesz)
        return CyErr();
    length = std::min(length, (uint32_t)(phdr->p_filesz - offset));
    if (length == 0)
        return CyErr();

    if (cylseek(m_file, phdr->p_offset + offset, SEEK_SET) < 0)
        return CyErr(L"lseek failed");
    ssize_t r = readfd(m_file, buf, (uint32_t)length);
    if (r < 0)
        return CyErr(L"Error reading bytes for phdr");
    if ((uint32_t)r < length)
        return CyErr(CyErr::FAIL, L"Failed to read data for phdr: Expected %u bytes but got %d",
            length, r);
    return CyErr();
}

const char * CyElfFile::GetString(size_t offset) const
{
    const char *str = elf_strptr(m_elf, m_shStrTabIdx, offset);
    if (!str)
        str = "";
    return str;
}

size_t CyElfFile::AddString(const char *str)
{
    return InsertStringToShStrTab(m_elf, str);
}

bool CyElfFile::GetSymbol(int index, GElf_Sym *dst) const
{
    memset(dst, 0, sizeof(GElf_Sym));
    Elf_Scn *scn = GetSection(".symtab");
    if (!scn)
        return false;
    Elf_Data *data = elf_getdata(scn, NULL);
    if (!data)
        return false;
    size_t entSize = gelf_getclass(m_elf) == ELFCLASS64 ? sizeof(Elf64_Sym) : sizeof(Elf32_Sym);
    // Entry 0 is reserved
    if (index < 1 || index > (int)(data->d_size / entSize))
        return false;
    return gelf_getsym(data, index, dst) == dst;
}

bool CyElfFile::GetSymbol(Elf_Data* data, int index, GElf_Sym* dst) const
{
    memset(dst, 0, sizeof(GElf_Sym));
    size_t entSize = gelf_getclass(m_elf) == ELFCLASS64 ? sizeof(Elf64_Sym) : sizeof(Elf32_Sym);
    if (index < 1 || index >(int)(data->d_size / entSize))
        return false;
    return gelf_getsym(data, index, dst) == dst;
}

std::string CyElfFile::GetSymbolName(Elf_Data* strData, GElf_Sym* sym) const
{
    size_t offset = sym->st_name;
    if (offset < strData->d_size)
    {
        const char *symName = (const char *)strData->d_buf + offset;
        return std::string(symName);
    }
    else
    {
        return std::string("");
    }
}

bool CyElfFile::GetSymbol(const char *name, GElf_Sym *dst) const
{
    memset(dst, 0, sizeof(GElf_Sym));
    if (name[0] == 0)
        return false;
    Elf_Scn *scn = GetSection(".symtab");
    if (!scn)
        return false;
    Elf_Data *data = elf_getdata(scn, NULL);
    if (!data)
        return false;
    Elf_Scn *strTabScn = GetSection(".strtab");
    if (!strTabScn)
        return false;
    Elf_Data *strData = elf_getdata(strTabScn, NULL);
    if (!strData)
        return false;
    size_t entSize = gelf_getclass(m_elf) == ELFCLASS64 ? sizeof(Elf64_Sym) : sizeof(Elf32_Sym);
    // Entry 0 is reserved
    for (size_t i = 1; i < (data->d_size / entSize); i++)
    {
        GElf_Sym sym;
        if (gelf_getsym(data, i, &sym) == &sym)
        {
            size_t nameOffset = sym.st_name;
            if (nameOffset < strData->d_size)
            {
                const char *symName = (const char *)strData->d_buf + nameOffset;
                if (0 == strcmp(name, symName))
                {
                    *dst = sym;
                    return true;
                }
            }
        }
    }
    return false;
}

bool CyElfFile::IsInDataRange(Elf_Data* data, int index) const
{
    size_t entSize = gelf_getclass(m_elf) == ELFCLASS64 ? sizeof(Elf64_Sym) : sizeof(Elf32_Sym);
    if (index < 1 || index >(int)(data->d_size / entSize))
        return false;
    else
        return true;
}

void CyElfFile::DirtyElf()
{
    elf_flagelf(m_elf, ELF_C_SET, ELF_F_DIRTY);
}

void CyElfFile::DirtyEhdr()
{
    elf_flagehdr(m_elf, ELF_C_SET, ELF_F_DIRTY);
}

void CyElfFile::DirtyPhdr()
{
    elf_flagphdr(m_elf, ELF_C_SET, ELF_F_DIRTY);
}

void CyElfFile::DirtySection(Elf_Scn *scn)
{
    elf_flagscn(scn, ELF_C_SET, ELF_F_DIRTY);
}

void CyElfFile::DirtyShdr(Elf_Scn *scn)
{
    elf_flagshdr(scn, ELF_C_SET, ELF_F_DIRTY);
}

void CyElfFile::DirtyData(Elf_Data *data)
{
    elf_flagdata(data, ELF_C_SET, ELF_F_DIRTY);
}

CyErr CyElfFile::Update()
{
    CyErr err;
    if (elf_update(m_elf, ELF_C_NULL) < 0)
        return CyErr(CyErr::FAIL, L"elf_update() failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
    if (gelf_getehdr(m_elf, &m_ehdr) != &m_ehdr)
        return CyErr(CyErr::FAIL, L"elf_getehdr() failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
    if ((err = ReadShdrs()).IsNotOK())
        return err;
    if ((err = ReadPhdrs()).IsNotOK())
        return err;
    if (elf_getshdrstrndx(m_elf, &m_shStrTabIdx) < 0)
        return CyErr(CyErr::FAIL, L"gelf_getshdrstrndx() failed: %s", str_to_wstr(elf_errmsg(-1)).c_str());
    return CyErr();
}

} // namespace elf
} // namespace cyelflib
