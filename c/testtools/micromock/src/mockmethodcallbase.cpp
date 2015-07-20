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
#include "mockmethodcallbase.h"

CMockMethodCallBase::CMockMethodCallBase()
{
    Init(_T(""));
}

CMockMethodCallBase::~CMockMethodCallBase()
{
    if (NULL != m_ReturnValue)
    {
        delete m_ReturnValue;
    }

    for (size_t i = 0; i < m_MockCallArguments.size(); i++)
    {
        delete m_MockCallArguments[i];
    }
}

CMockMethodCallBase::CMockMethodCallBase(std::tstring methodName, size_t argCount, CMockCallArgumentBase** arguments)
{
    Init(methodName);

    for (unsigned char i = 0; i < argCount; i++)
    {
        m_MockCallArguments.push_back(arguments[i]);
    }
}

void CMockMethodCallBase::Init(std::tstring methodName)
{
    m_ReturnValue = NULL;
    m_MethodName = methodName;
    m_MatchedCall = NULL;
    m_OnlySpecifiesActions = false;
    m_ExpectedTimes = 1;
    m_MatchedTimes = 0;
    m_AlwaysReport = false;
    m_ExactExpectedTimes = false;
}

std::tstring CMockMethodCallBase::GetArgumentsString()
{
    std::tstring result;

    for (size_t i = 0; i < m_MockCallArguments.size(); i++)
    {
        if (result.length() > 0)
        {
            result += _T(",");
        }

        result += m_MockCallArguments[i]->ToString();
    }

    return result;
}

std::tstring CMockMethodCallBase::ToString()
{
    std::tstring result = m_MethodName;

    result += _T("(");
    result += GetArgumentsString();
    result += _T(")");

    return result;
}

void CMockMethodCallBase::RollbackMatch()
{
    m_MatchedCall = NULL;
}

void CMockMethodCallBase::AddExtraCallArgument(CMockCallArgumentBase* callArgument)
{
    m_MockCallArguments.push_back(callArgument);
}

bool CMockMethodCallBase::operator==(const CMockMethodCallBase& right)
{
    bool result = (m_MethodName == right.m_MethodName);
    result = result && (m_MockCallArguments.size() == right.m_MockCallArguments.size());
    if (result)
    {
        for (size_t i = 0; i < m_MockCallArguments.size(); i++)
        {
            if (!(m_MockCallArguments[i]->EqualTo(right.m_MockCallArguments[i])))
            {
                result = false;
            }
        }
    }

    return result;
}

void CMockMethodCallBase::CopyOutArgumentBuffers(CMockMethodCallBase* sourceMockMethodCall)
{
    if (m_MockCallArguments.size() == sourceMockMethodCall->m_MockCallArguments.size())
    {
        for (size_t i = 0; i < m_MockCallArguments.size(); i++)
        {
            // TODO: This should also be handled when comparing calls ...
            (void)m_MockCallArguments[i]->CopyOutArgumentDataFrom(sourceMockMethodCall->m_MockCallArguments[i]);
        }
    }
}
