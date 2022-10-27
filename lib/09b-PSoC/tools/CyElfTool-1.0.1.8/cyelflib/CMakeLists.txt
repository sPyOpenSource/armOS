cmake_minimum_required(VERSION 2.6)
project(cyelflib)

#_______/ ENSURE OUT-OF-SOURCE BUILD \_____________#
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR
    "   ${CMAKE_PROJECT_NAME} requires an out-of-source build.\n"
    "   please create a build directory and run 'cmake path/to/source_directory' from there")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

#_________/ SET PROJECT SRC & HEADERS \__________#
set(SOURCES
    commandlinebase.cpp
#   stdafx.cpp # include if using pre-compiled headers
    utils.cpp
    version.cpp
    elf/cyelfutil.cpp
    elf/elfxx.cpp
    hex/cyhexfile.cpp
    hex/cypsocacdfile.cpp
    hex/cypsocacd2file.cpp
    hex/cypsochexfile.cpp
)
set(HEADERS
    commandlinebase.h
    stdafx.h
    targetver.h
    utils.h
    version.h
    cyerr.h
    elf/cyelfutil.h
    elf/elfxx.h
    hex/cyhexfile.h
    hex/cypsocacdfile.h
    hex/cypsocacd2file.h
    hex/cypsochexfile.h
)
    

#_____________/ FINAL STEPS \____________________#
if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -Wall -std=gnu++1y")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LIBELF_DIR}/lib/libelf.a" )
endif(UNIX)
if(WIN32)
  set(LIBELF_DIR "${PROJECT_BINARY_DIR}/../libelf_win32")
  add_definitions( -DUNICODE )
  add_definitions( -D_UNICODE )
  add_definitions( -DNOMINMAX )
  add_definitions( -D_CRT_SECURE_NO_DEPRECATE )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
  add_definitions( -D_CRT_NONSTDC_NO_WARNINGS )
  add_definitions( -D_SCL_SECURE_NO_WARNINGS )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif(WIN32)
include_directories(${PROJECT_SOURCE_DIR} "${LIBELF_DIR}/include")
link_directories("${LIBELF_DIR}/lib")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
