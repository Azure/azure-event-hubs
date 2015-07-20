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

#ifndef MOCKCALLRECORDER_H
#define MOCKCALLRECORDER_H

#pragma once

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
#include "mockmethodcallbase.h"
#include "micromocktestrunnerhooks.h"
#include "mockcallcomparer.h"

typedef enum AUTOMATIC_CALL_COMPARISON_TAG
{
    AUTOMATIC_CALL_COMPARISON_OFF,
    AUTOMATIC_CALL_COMPARISON_ON
} AUTOMATIC_CALL_COMPARISON;

class CMockCallRecorder
{
public:
    CMockCallRecorder(AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_ON,
        CMockCallComparer* mockCallComparer = NULL);
    virtual ~CMockCallRecorder(void);

public:
    void RecordExpectedCall(CMockMethodCallBase* mockMethodCall);
    CMockValueBase* RecordActualCall(CMockMethodCallBase* mockMethodCall);
    CMockValueBase* MatchActualCall(CMockMethodCallBase* mockMethodCall);
    void AssertActualAndExpectedCalls(void);
    std::tstring CompareActualAndExpectedCalls(void);
    std::tstring GetUnexpectedCalls(std::tstring unexpectedCallPrefix = _T(""));
    std::tstring GetMissingCalls(std::tstring missingCallPrefix = _T(""));
    void ResetExpectedCalls();
    void ResetActualCalls();
    void ResetAllCalls();
    SAL_Acquires_lock_(m_MockCallRecorderCS) void Lock();
    SAL_Releases_lock_(m_MockCallRecorderCS) void Unlock();
    void SetPerformAutomaticCallComparison(AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison);
    void SetMockCallComparer(CMockCallComparer* mockCallComparer) { m_MockCallComparer = mockCallComparer; };

protected:
    std::vector<CMockMethodCallBase*> m_ExpectedCalls;
    std::vector<CMockMethodCallBase*> m_ActualCalls;

protected:
    MICROMOCK_CRITICAL_SECTION m_MockCallRecorderCS;
    AUTOMATIC_CALL_COMPARISON m_PerformAutomaticCallComparison;
    CMockCallComparer* m_MockCallComparer;
};

#endif // MOCKCALLRECORDER_H
