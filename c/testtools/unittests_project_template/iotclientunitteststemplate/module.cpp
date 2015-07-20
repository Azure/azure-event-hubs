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

#include "stdafx.h"

#include "CommonIncludes.h"

class PackageModule :
	public CAtlDllModuleT<PackageModule> 
{
};

PackageModule _AtlModule;

// This macro is used as default registry root when a NULL parameter is passed to VSDllRegisterServer
// or VSDllUnregisterServer. For sample code we set as default the experimental instance, but for production
// code you should change it to the standard VisualStudio instance that is LREGKEY_VISUALSTUDIOROOT.
#define DEFAULT_REGISTRY_ROOT LREGKEY_VISUALSTUDIOROOT

// Since this project defines an oleautomation interface, the typelib needs to be registered.
#define VSL_REGISTER_TYPE_LIB TRUE

// Must come after declaration of _AtlModule and DEFAULT_REGISTRY_ROOT
#include <VSLPackageDllEntryPoints.cpp>

