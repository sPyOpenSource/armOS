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
#include "cybootloaderutil.h"
#include "elf/cyelfutil.h"
#include "utils.h"
#include "cyerr.h"
#include <vector>
#include <iostream>

using std::wstring;
using std::vector;
using std::cout;
using std::endl;

namespace cyelftool {
namespace cmd {

void DisplayNvlValues(const std::wstring &filename)
{
    uint8_t cunvl[4], wonvl[4];

    CyErr err(GetNvlValues(filename, cunvl, wonvl));
    if (err.IsNotOK())
        errx(EXIT_FAILURE, L"%ls", err.Message().c_str());

    cout << cyelflib::elf::CUSTNVL_SECTION << ':';
    HexDump(cout, cunvl, sizeof(cunvl));
    cout << endl;
    cout << cyelflib::elf::WOLATCH_SECTION << ':';
    HexDump(cout, wonvl, sizeof(wonvl));
    cout << endl;
}

} // namespace cmd
} // namespace cyelftool
