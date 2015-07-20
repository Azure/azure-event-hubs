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

#ifndef MICROMOCKEXCEPTION_H
#define MICROMOCKEXCEPTION_H

#pragma once

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
#include "macro_utils.h"

#define MICROMOCK_EXCEPTION_VALUES                        \
MICROMOCK_EXCEPTION_INVALID_VALIDATE_BUFFERS,             \
MICROMOCK_EXCEPTION_ALLOCATION_FAILURE,                   \
MICROMOCK_EXCEPTION_INVALID_ARGUMENT,                     \
MICROMOCK_EXCEPTION_INVALID_CALL_MODIFIER_COMBINATION,    \
MICROMOCK_EXCEPTION_MOCK_NOT_FOUND,                       \
MICROMOCK_EXCEPTION_SET_TIME_BEFORE_CALL,                 \
MICROMOCK_EXCEPTION_SET_ARRAY_SIZE_BEFORE_CALL,           \
MICROMOCK_EXCEPTION_INTERNAL_ERROR                        \


DEFINE_ENUM(MICROMOCK_EXCEPTION, MICROMOCK_EXCEPTION_VALUES)

extern const TCHAR* MicroMockExceptionToString(_In_ MICROMOCK_EXCEPTION exceptionCode);

class CMicroMockException
{
public:
    CMicroMockException(_In_ MICROMOCK_EXCEPTION exceptionCode, _In_ std::tstring exceptionText) :
        m_ExceptionCode(exceptionCode),
        m_ExceptionString(exceptionText)
    {
    }
    ~CMicroMockException()
    {
    }

    MICROMOCK_EXCEPTION GetMicroMockExceptionCode() const { return m_ExceptionCode; }
    std::tstring GetExceptionString() const { return m_ExceptionString; }

protected:
    MICROMOCK_EXCEPTION m_ExceptionCode;
    std::tstring m_ExceptionString;
};

#ifdef MOCK_TOSTRING
MOCK_TOSTRING<MICROMOCK_EXCEPTION>(const MICROMOCK_EXCEPTION& t) { return  MicroMockExceptionToString(t);}
#endif

#endif // MICROMOCKEXCEPTION_H
