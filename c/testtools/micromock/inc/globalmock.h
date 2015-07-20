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

#ifndef GLOBALMOCK_H
#define GLOBALMOCK_H

#pragma once

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
#include "mock.h"
#include "mockcallrecorder.h"
#include "micromockexception.h"

template<class C>
class CGlobalMock : public CMock<C>
{
public:
    CGlobalMock(_In_ AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_ON) :
        CMock<C>(performAutomaticCallComparison)
    {
        m_GlobalMockInstance = this;
    }

    virtual ~CGlobalMock()
    {
        m_GlobalMockInstance = NULL;
    }

    _Check_return_
    _Post_satisfies_(return != NULL)
    static CGlobalMock<C>* GetSingleton()
    {
        if (NULL == m_GlobalMockInstance)
        {
            MOCK_THROW(CMicroMockException(MICROMOCK_EXCEPTION_INTERNAL_ERROR,
                _T("Error retrieving singleton")));
        }

        return m_GlobalMockInstance;
    }

protected:
    static CGlobalMock<C>* m_GlobalMockInstance;
};

template<class C>
CGlobalMock<C>* CGlobalMock<C>::m_GlobalMockInstance;

#endif // GLOBALMOCK_H
