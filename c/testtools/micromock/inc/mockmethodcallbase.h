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

#ifndef MOCKMETHODCALLBASE_H
#define MOCKMETHODCALLBASE_H

#pragma once

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
#include "mockvaluebase.h"
#include "mockcallargumentbase.h"

class CMockMethodCallBase
{
public:
    CMockMethodCallBase();
    virtual ~CMockMethodCallBase();
    CMockMethodCallBase(std::tstring methodName, size_t argCount = 0,
        CMockCallArgumentBase** arguments = NULL);

    std::tstring ToString();
    std::tstring GetArgumentsString();
    bool operator==(const CMockMethodCallBase& right);
    void CopyOutArgumentBuffers(CMockMethodCallBase* sourceMockMethodCall);
    void AddExtraCallArgument(CMockCallArgumentBase* callArgument);

    CMockMethodCallBase* m_MatchedCall;
    bool m_OnlySpecifiesActions;
	bool m_IgnoreAllCalls;
    bool m_AlwaysReport;
    size_t m_ExpectedTimes;
    size_t m_MatchedTimes;
    bool m_ExactExpectedTimes;
    virtual CMockValueBase* GetReturnValue() { return m_ReturnValue; }
    _Must_inspect_result_ bool HasMatch() const { return (NULL != m_MatchedCall); }
    void RollbackMatch();

protected:
    void Init(std::tstring methodName);

    std::vector<CMockCallArgumentBase*> m_MockCallArguments;
    std::tstring m_MethodName;
    CMockValueBase* m_ReturnValue;
};

#endif // MOCKMETHODCALLBASE_H
