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

#ifndef MOCKCALLARGUMENTBASE_H
#define MOCKCALLARGUMENTBASE_H

#pragma once

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
#include "mockvaluebase.h"


typedef struct BUFFER_ARGUMENT_DATA_TAG
{
    void*   m_Buffer;
    size_t  m_ByteCount;
    size_t  m_Offset;

    _Must_inspect_result_
    bool operator<(_In_ const BUFFER_ARGUMENT_DATA_TAG& rhs) const
    {
        if (m_Offset < rhs.m_Offset)
        {
            return true;
        }

        if (m_Offset > rhs.m_Offset)
        {
            return false;
        }

        if (m_ByteCount < rhs.m_ByteCount)
        {
            return true;
        }

        if (m_ByteCount > rhs.m_ByteCount)
        {
            return false;
        }

        return m_Buffer < rhs.m_Buffer;
    }
} BUFFER_ARGUMENT_DATA;

class CMockCallArgumentBase
{
public:
    virtual ~CMockCallArgumentBase() {};

    virtual void SetIgnored(_In_ bool ignored) = 0;
    virtual _Check_return_ std::tstring ToString() const = 0;
    virtual bool EqualTo(_In_ const CMockCallArgumentBase* right) = 0;
    virtual void AddCopyOutArgumentBuffer(_In_reads_bytes_(bytesToCopy) const void* injectedBuffer, _In_ size_t bytesToCopy, _In_ size_t byteOffset = 0) = 0;
    virtual void AddBufferValidation(_In_reads_bytes_(bytesToValidate) const void* expectedBuffer, _In_ size_t bytesToValidate, _In_ size_t byteOffset = 0) = 0;
    virtual void CopyOutArgumentDataFrom(_In_ const CMockCallArgumentBase* sourceMockCallArgument) = 0;
};

#endif // MOCKCALLARGUMENTBASE_H
