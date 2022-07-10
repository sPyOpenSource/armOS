#  mkdir build
#  cd build
#  cmake -DLIBELF_DIR=/path/to/libelf ../
#  make

cmake_minimum_required(VERSION 2.6)
project(cyelftool)

#_______/ ENSURE OUT-OF-SOURCE BUILD \_____________#
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR
    "   ${CMAKE_PROJECT_NAME} requires an out-of-source build.\n"
    "   please create a build directory and run 'cmake path/to/source_directory' from there")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

#_________/ PLATFORM SPEC. BUILD SETTINGS \_________#
set(CYELFLIB_SRC_DIR "${PROJECT_SOURCE_DIR}/../cyelflib")
set(CYELFLIB_DIR "${PROJECT_BINARY_DIR}/../cyelflib")
if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -Wall -std=gnu++11")
elseif(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -Wall -std=gnu++11")
  set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static" )
elseif(WIN32)
  set(LIBELF_DIR "${PROJECT_BINARY_DIR}/../libelf_win32")
  set(LIBELF_LINK "${LIBELF_DIR}/lib/libelf.lib")
  add_definitions( -DUNICODE )
  add_definitions( -D_UNICODE )
  add_definitions( -DNOMINMAX )
  add_definitions( -D_CRT_SECURE_NO_DEPRECATE )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
  add_definitions( -D_CRT_NONSTDC_NO_WARNINGS )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
endif()
if(NOT DEFINED ELFTOOL_VER)
  set(ELFTOOL_VER UNSPECIFIED_VERSION)
endif()
add_definitions( "-DAPPLICATION_VERSION=L\"${ELFTOOL_VER}\"" )

#_________/ SET PROJECT SRC & HEADERS \__________#
set(SOURCES
    commandline.cpp
    cybootloaderutil.cpp
    cyelftool.cpp
    elf2hex.cpp
    cmd/cycmdbuildbootloadable.cpp
    cmd/cycmdbuildbootloader.cpp
    cmd/cycmdchecksum.cpp
    cmd/cycmddisplaynvl.cpp
    cmd/cycmddisplaysize.cpp 
    cmd/cycmdextractbootloader.cpp
    cmd/cycmdmultiappmerge.cpp
    cmd/cycodeshareprovides.cpp
#   stdafx.cpp # include if using pre-compiled headers
)
set(HEADERS
    ../cyelflib/stdafx.h
    cmd/cyelfcmd.h
    commandline.h
    cybootloaderutil.h
    elf2hex.h
)


#_____________/ FINAL STEPS \____________________#
include_directories(${CYELFLIB_SRC_DIR} ${PROJECT_SOURCE_DIR} "${LIBELF_DIR}/include")
link_directories("${LIBELF_DIR}" "${CYELFLIB_DIR}/lib")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
# do not link "elf" directly, as this might use libelf included with system instead of from cyelftool package
if(UNIX)
target_link_libraries(${PROJECT_NAME} 
     cyelflib
     ${LIBELF_DIR}/lib/libelf.a
)
elseif(WIN32)
  target_link_libraries(${PROJECT_NAME} cyelflib )
  target_link_libraries(${PROJECT_NAME} debug ${LIBELF_DIR}/lib/static/libelf-debug.lib )
  target_link_libraries(${PROJECT_NAME} optimized ${LIBELF_DIR}/lib/static/libelf.lib )
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES "${LIBELF_DIR}/lib/libelf.so.${LIBELF_VER}" DESTINATION lib)
