cmake_minimum_required(VERSION 2.6)
project(cymcuelftool)

#_______/ ENSURE OUT-OF-SOURCE BUILD \_____________#
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR
    "   ${CMAKE_PROJECT_NAME} requires an out-of-source build.\n"
    "   please create a build directory and run 'cmake path/to/source_directory' from there")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

# Set cymcuelftool version
if(DEFINED ELFTOOL_VER)
  add_definitions( -DELFTOOL_VER=L"${ELFTOOL_VER}" )
endif(DEFINED ELFTOOL_VER)

#_________/ SET ENV VARIABLES \___________________#
set(CYELFLIB_SRC_DIR "${PROJECT_SOURCE_DIR}/../cyelflib")
set(CYELFLIB_DIR "${PROJECT_BINARY_DIR}/../cyelflib")
if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -Wall -std=gnu++1y")
  set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++" )
#  set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${LIBELF_DIR}/lib/libelf.a ${CYELFLIB_DIR}/libcyelflib.a -static-libstdc++" )
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
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
endif(WIN32)


#_________/ SET PROJECT SRC & HEADERS \__________#
set(SOURCES
    commandline.cpp
    cymcuelftool.cpp
    cymcuelfutil.cpp
    cmd/cycmddisplaysize.cpp
    cmd/cycmdmerge.cpp
    cmd/cycmdpatch.cpp
    cmd/cycmdshare.cpp
    cmd/cycmdsign.cpp
    cmd/cyopenssl.cpp
    elf2hex.cpp
#   stdafx.cpp # include if using pre-compiled headers
)
set(HEADERS
    elf2hex.h
    commandline.h
    cymcuelfutil.h
    cmd/cyelfcmd.h
    cmd/cyopenssl.h
)



#_____________/ FINAL STEPS \____________________#
include_directories(${CYELFLIB_SRC_DIR} ${PROJECT_SOURCE_DIR} "${LIBELF_DIR}/include")
link_directories("${LIBELF_DIR}/lib")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
# do not link "elf" directly, as this might use libelf included with system instead of from cyelftool package
if(UNIX)
target_link_libraries(${PROJECT_NAME} 
     cyelflib
     ${LIBELF_DIR}/lib/libelf.a
)
endif(UNIX)
if(WIN32)
  target_link_libraries(${PROJECT_NAME} cyelflib )
  # Static linking approved for CyMCUElfTool per FYA-354
  target_link_libraries(${PROJECT_NAME} debug ${LIBELF_DIR}/lib/static/libelf-debug.lib ) 
  target_link_libraries(${PROJECT_NAME} optimized ${LIBELF_DIR}/lib/static/libelf.lib )
endif(WIN32)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES "${LIBELF_DIR}/lib/libelf.so.${LIBELF_VER}" DESTINATION lib)
