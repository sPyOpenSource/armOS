// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef NO_PRECOMPILED_HEADER

#include "targetver.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <codecvt>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <list>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#include <libelf/libelf.h>

#ifdef _MSC_VER
#include <tchar.h>
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#endif
