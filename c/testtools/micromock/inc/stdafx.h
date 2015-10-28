/*
Microsoft Azure IoT Device Libraries
Copyright (c) Microsoft Corporation
All rights reserved. 
MIT License
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
documentation files (the Software), to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
IN THE SOFTWARE.
*/

#pragma once

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "targetver.h"

#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "tchar.h"
#include "sal.h"
#include "basetsd.h"
#define SAL_Acquires_lock_(...) _Acquires_lock_(__VA_ARGS__)
#define SAL_Releases_lock_(...) _Releases_lock_(__VA_ARGS__)
typedef CRITICAL_SECTION MICROMOCK_CRITICAL_SECTION;
#define MicroMockEnterCriticalSection(...) EnterCriticalSection(__VA_ARGS__)
#define MicroMockLeaveCriticalSection(...) LeaveCriticalSection(__VA_ARGS__)

#if (defined _WIN32_WCE)
#define MicroMockInitializeCriticalSection(...) InitializeCriticalSection(__VA_ARGS__)
#else
#define MicroMockInitializeCriticalSection(...) InitializeCriticalSectionEx(__VA_ARGS__,2,CRITICAL_SECTION_NO_DEBUG_INFO)
#endif

#define MicroMockDeleteCriticalSection(...) DeleteCriticalSection(__VA_ARGS__)
#else
typedef unsigned char UINT8;
typedef char TCHAR;
#define _T(A)   A
typedef int MICROMOCK_CRITICAL_SECTION;
#define MicroMockEnterCriticalSection(...)
#define MicroMockLeaveCriticalSection(...)
#define MicroMockInitializeCriticalSection(...)
#define MicroMockDeleteCriticalSection(...)
#define SAL_Acquires_lock_(...)
#define SAL_Releases_lock_(...)
#endif

#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <set>

#include <sal.h>

#ifdef _MSC_VER
/*'function' : unreferenced local function has been removed*/
#pragma warning( disable: 4505 ) 

/*unreferenced inline function has been removed*/
#pragma warning( disable: 4514)
#endif

#define COUNT_OF(a)     (sizeof(a) / sizeof((a)[0]))

namespace std
{
    typedef std::basic_string<TCHAR> tstring;
    typedef std::basic_ostringstream<TCHAR> tostringstream;
}
