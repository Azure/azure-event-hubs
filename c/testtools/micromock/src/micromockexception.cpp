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

/*defines*/
#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
#include "micromockexception.h"
#include "micromocktestrunnerhooks.h"
#define _MACROSTR(a) _T(#a)
/*types*/

/*static variables*/
/*static functions*/
/*variable exports*/

const TCHAR* MicroMockExceptionToString(_In_ MICROMOCK_EXCEPTION exceptionCode)
{
    switch (exceptionCode)
    {
        default:
        return "Invalid exception code";

        case MICROMOCK_EXCEPTION_INVALID_VALIDATE_BUFFERS:
	        return _MACROSTR(MICROMOCK_EXCEPTION_INVALID_VALIDATE_BUFFERS);
        case MICROMOCK_EXCEPTION_ALLOCATION_FAILURE:
	        return _MACROSTR(MICROMOCK_EXCEPTION_ALLOCATION_FAILURE);
        case MICROMOCK_EXCEPTION_INVALID_ARGUMENT:
	        return _MACROSTR(MICROMOCK_EXCEPTION_INVALID_ARGUMENT);
        case MICROMOCK_EXCEPTION_INVALID_CALL_MODIFIER_COMBINATION:
	        return _MACROSTR(MICROMOCK_EXCEPTION_INVALID_CALL_MODIFIER_COMBINATION);
        case MICROMOCK_EXCEPTION_MOCK_NOT_FOUND:
	        return _MACROSTR(MICROMOCK_EXCEPTION_MOCK_NOT_FOUND);
        case MICROMOCK_EXCEPTION_SET_TIME_BEFORE_CALL:
	        return _MACROSTR(MICROMOCK_EXCEPTION_SET_TIME_BEFORE_CALL);
        case MICROMOCK_EXCEPTION_SET_ARRAY_SIZE_BEFORE_CALL:
	        return _MACROSTR(MICROMOCK_EXCEPTION_SET_ARRAY_SIZE_BEFORE_CALL);
        case MICROMOCK_EXCEPTION_INTERNAL_ERROR:
	        return _MACROSTR(MICROMOCK_EXCEPTION_INTERNAL_ERROR);
    }
};

/*function exports*/

