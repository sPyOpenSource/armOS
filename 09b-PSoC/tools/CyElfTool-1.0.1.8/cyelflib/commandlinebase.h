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

#ifndef INCLUDED_COMMANDLINEBASE_H
#define INCLUDED_COMMANDLINEBASE_H

#include <string>
#include <vector>
#include <unordered_set>
#include "utils.h"
#include "cyerr.h"

namespace cyelflib
{

class CommandLineBase : cyelflib::noncopyable
{
public:
    typedef std::vector<std::wstring>::const_iterator const_string_iterator;

    CommandLineBase() :
        m_fillValue(0)
    {
    }
    virtual ~CommandLineBase() { }

    int ReadArguments(int argc, char *argv[]);
    int ReadArguments(int argc, wchar_t *argv[]);
    virtual void DisplayHelp() const = 0;

    const std::wstring &PrimaryInput() const { return m_inputs.size() != 0 ? m_inputs[0] : EmptyString; }
    const std::wstring &PrimaryOutput() const { return m_output; }
    const_string_iterator InputsBegin() const { return m_inputs.begin(); }
    const_string_iterator InputsEnd() const { return m_inputs.end(); }
    const std::wstring &getInput(unsigned int i) const { return m_inputs.at(i); }
    /// Gets the file to write the output to
    const std::wstring &OutputFile() const { return m_output; }

    /// Gets the value to fill uninitialized values in flash
    uint8_t getFillValue() const { return m_fillValue; }
protected:
    virtual cyelflib::CyErr::CyErrStatus _ReadArguments(const std::vector<std::wstring> &argv) = 0;
    CyErr::CyErrStatus CheckArgs(const std::wstring &arg, int actual, int expected);

    void CopyToInputs(const std::vector<std::wstring> &src, int start, int count, std::vector<std::wstring> &dest);
    const void CheckForDuplicateInputs();
    uint8_t ReadUint8Arg(const std::vector<std::wstring> &argv, unsigned int prevIdx);
    uint32_t ReadUintArg(const std::vector<std::wstring> &argv, unsigned int prevIdx);
    const std::wstring &ReadStringArg(const std::vector<std::wstring> &argv, unsigned int prevIdx);

    static const std::wstring EmptyString;

    std::wstring m_output;
    std::vector<std::wstring> m_inputs;
    uint8_t m_fillValue;
private:
};

} // namespace cyelflib

#endif
