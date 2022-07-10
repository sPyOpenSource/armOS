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
#include "version.h"
#include <iostream>

namespace cyelflib
{

const wchar_t *APPLICATION_COPYRIGHT =
    L"Copyright (C) 2013-2017 Cypress Semiconductor Corporation.\n"
    L"This program comes with ABSOLUTELY NO WARRANTY\n"
    L"This is free software, and you are welcome to redistribute it under\n"
    L"the terms of the LGPLv2.0 license\n";

void DisplayVersion(const wchar_t* name, const wchar_t* version)
{
    std::wcout << name << ' ' << version << '\n'
        << APPLICATION_COPYRIGHT << std::endl;
}

} // namespace cyelflib
