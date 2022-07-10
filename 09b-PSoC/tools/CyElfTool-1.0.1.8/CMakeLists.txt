#=========================================================================
# CMake file for cyelftool, and cyelflib
# Usage:
#   UNIX:
#     mkdir build
#     cd build
#     cmake -DLIBELF_DIR=/path/to/libelf path/to/source/dir
#     make
#   MAC:
#     same as UNIX
#   WINDOWS: (for visual studio 2013. For other versions, change the '12' below)
#     mkdir build
#     cd build
#     cmake -G "Visual Studio 12" -DLIBELF_DIR=/path/to/libelf path/to/source/dir
#     MSBuild CyElftool.sln
#==========================================================================

cmake_minimum_required( VERSION 2.6 )
project(CyElftool)
  
#_______/ ENSURE OUT-OF-SOURCE BUILD \___________________________#
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR
    "   ${CMAKE_PROJECT_NAME} requires an out-of-source build.\n"
    "   please create a build directory and run 'cmake path/to/source_directory' from there")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})


#_________/ SOLUTION SUBDIRS (PROJECTS) \_______________________#
add_subdirectory(${CMAKE_SOURCE_DIR}/cyelflib)
add_subdirectory(${CMAKE_SOURCE_DIR}/cyelftool)
