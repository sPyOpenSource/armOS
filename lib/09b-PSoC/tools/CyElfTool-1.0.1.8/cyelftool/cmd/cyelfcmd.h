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

#ifndef INCLUDED_CYELFCMD_H
#define INCLUDED_CYELFCMD_H

#include <string>

namespace cyelftool {

class CommandLine;

namespace cmd {

void ChecksumCommand(const CommandLine& cmd);
void DisplayNvlValues(const std::wstring &filename);
void DisplaySize(const CommandLine &cmd);
void BuildBootloaderCommand(const CommandLine &cmd);
void ExtractBootloaderCommand(const CommandLine &cmd);
void BuiltBootloadableCommand(const CommandLine &cmd);
void MultiAppMergeCommand(const CommandLine &cmd);
void GenerateCodeshareProvidesCmd(const CommandLine &cmd);
void RenameSymbolsCmd(const CommandLine &cmd);

} // namespace cmd
} // namespace cyelftool

#endif
