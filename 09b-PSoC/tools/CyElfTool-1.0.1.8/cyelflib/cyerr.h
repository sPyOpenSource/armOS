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

#ifndef CYERR_H
#define CYERR_H

#include <iostream>
#include <string>
#include <cstdarg>
#include "utils.h"

namespace cyelflib
{

class CyErr
{
public:
    typedef enum {
        /// <summary>
        /// OK is the passing status
        /// </summary>
        OK = 0,

        /// <summary>
        /// Right now we have only one failing status
        /// </summary>
        FAIL = 1
    } CyErrStatus;

    CyErr() : m_errId(OK), m_msg(L"Success") { }
    explicit CyErr(const std::string &msg) : m_errId(FAIL)
    {
        m_msg = str_to_wstr(msg);
    }
    // CyErrStatus is not really necessary here but disambiguates the overloads
    CyErr(CyErrStatus status, const char *fmt, ...) : m_errId(status)
    {
        static const int BUF_LEN = 512;
        char buf[BUF_LEN];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, BUF_LEN, fmt, args);
        buf[BUF_LEN - 1] = '\0';
        va_end(args);
        m_msg = str_to_wstr(std::string(buf));
    }
    explicit CyErr(const std::wstring &msg) : m_errId(FAIL), m_msg(msg) { }
    // CyErrStatus is not really necessary here but disambiguates the overloads
    CyErr(CyErrStatus status, const wchar_t *fmt, ...) : m_errId(status)
    {
        static const int BUF_LEN = 512;
        wchar_t buf[BUF_LEN];
        va_list args;
        va_start(args, fmt);
        vswprintf(buf, BUF_LEN, fmt, args);
        buf[BUF_LEN - 1] = L'\0';    // vswprintf doesn't terminate on truncation
        va_end(args);
        m_msg = buf;
    }
    bool IsOK() const { return m_errId == OK; }
    bool IsNotOK() const { return !IsOK(); }
    CyErrStatus ErrorId() const { return m_errId; }
    const std::wstring &Message() const { return m_msg; }

private:
    CyErrStatus m_errId;
    std::wstring m_msg;
};

static inline std::wostream &operator<<(std::wostream &os, const CyErr &err)
{
    os << err.Message();
    return os;
}

} // namespace cyelflib

#endif
