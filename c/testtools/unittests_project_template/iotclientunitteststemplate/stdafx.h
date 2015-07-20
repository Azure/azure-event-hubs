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

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently


#pragma once

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

// Windows Platform headers and control defines
#define STRICT
#define _WIN32_WINNT 0x0500 // Visual Studio requires Windows 2000 or better
#define NOMINMAX // Windows Platform min and max macros cause problems for the Standard C++ Library
#define WIN32_LEAN_AND_MEAN	// Exclude rarely-used stuff from the Windows Platform headers

// ATL headers and control defines
#define _ATL_APARTMENT_THREADED
#define _ATL_REGISTER_PER_USER

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlstr.h>
#include <atlfile.h>
#include <atlsafe.h>

// Windows Platform headers
#include <windowsx.h> // REVIEW - what is this for?


// Visual Studio Platform headers
#include <dte.h> // for extensibility
#include <objext.h> // for ILocalRegistry
#include <vshelp.h> // for Help
#include <uilocale.h> // for IUIHostLocale2
#include <IVsQueryEditQuerySave2.h> // for IVsQueryEditQuerySave2
#include <vbapkg.h> // for IVsMacroRecorder
#include <fpstfmt.h> // for IPersistFileFormat
#include <VSRegKeyNames.h>
#include <stdidcmd.h>
#include <stdiduie.h> // For status bar consts.
#include <textfind.h>
#include <textmgr.h>

// VSL headers
#define VSLASSERT _ASSERTE
#define VSLASSERTEX(exp, szMsg) _ASSERT_BASE(exp, szMsg)
#define VSLTRACE ATLTRACE

#include <VSLPackage.h>
#include <VSLCommandTarget.h>
#include <VSLWindows.h>

#include <VSLFile.h>
#include <VSLContainers.h>
#include <VSLComparison.h>
#include <VSLAutomation.h>
#include <VSLFindAndReplace.h>
#include <VSLShortNameDefines.h>

using namespace VSL;

