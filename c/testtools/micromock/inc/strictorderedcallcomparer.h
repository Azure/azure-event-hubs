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

#ifndef STRICTORDEREDCALLCOMPARER_H
#define STRICTORDEREDCALLCOMPARER_H

#pragma once

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
#include "mockcallcomparer.h"
#include "mockmethodcallbase.h"

template<class T>
class CStrictOrderedCallComparer : public T
{
public:
    CStrictOrderedCallComparer(_In_ AUTOMATIC_CALL_COMPARISON performAutomaticCallComparison = AUTOMATIC_CALL_COMPARISON_ON) :
        m_ExpectedCallIndex(0)
    {
        T::SetPerformAutomaticCallComparison(performAutomaticCallComparison);
    }

    _Must_inspect_result_
    virtual bool IsUnexpectedCall(_In_ const CMockMethodCallBase* actualCall)
    {
        return !actualCall->HasMatch();
    }

    _Must_inspect_result_
    virtual bool IsMissingCall(_In_ const CMockMethodCallBase* expectedCall)
    {
        return ((!expectedCall->HasMatch()) &&
                (expectedCall->m_ExpectedTimes > 0) &&
                (!expectedCall->m_OnlySpecifiesActions));
    }

    _Must_inspect_result_
    virtual CMockMethodCallBase* MatchCall(std::vector<CMockMethodCallBase*>& expectedCalls,
        CMockMethodCallBase* actualCall)
    {
        CMockMethodCallBase* result = NULL;

        // skip any records that do not enforce expected calls
        while ((m_ExpectedCallIndex < expectedCalls.size()) &&
               (expectedCalls[m_ExpectedCallIndex]->m_OnlySpecifiesActions))
        {
            m_ExpectedCallIndex++;
        }

        // do we still have items in the expected calls array?
        if (m_ExpectedCallIndex < expectedCalls.size())
        {
            CMockMethodCallBase* expectedCall = expectedCalls[m_ExpectedCallIndex];

            // if the expected call has not yet been matched
            if (!expectedCall->HasMatch() &&
                // and it matches the actual call
                (*expectedCall == *actualCall))
            {
                // check whether the number of expected calls of this type have been found
                // in the actual calls array
                if (expectedCall->m_MatchedTimes < expectedCall->m_ExpectedTimes)
                {
                    // record that we found yet another actual call matching the expected call
                    expectedCall->m_MatchedTimes++;
                    if (expectedCall->m_MatchedTimes == expectedCall->m_ExpectedTimes)
                    {
                        // mark the expected call as fully matched
                        // (no other actual call will be matched against it)
                        expectedCall->m_MatchedCall = actualCall;
                    }

                    // mark the actual call as having a match
                    actualCall->m_MatchedCall = expectedCall;
                    result = expectedCall;
                }
                else
                {
                    // too many calls, check if exact is specified
                    if (expectedCall->m_ExactExpectedTimes)
                    {
                        actualCall->m_AlwaysReport = true;
                    }
                }
            }
        }

        if ((NULL != result) &&
            (result->m_MatchedTimes == result->m_ExpectedTimes))
        {
            m_ExpectedCallIndex++;
        }
        else
        {
            // have a second loop to see if we need to get a later set return value
            for (int i = (int)expectedCalls.size() - 1; i >= 0; i--)
            {
                if ((expectedCalls[i]->m_OnlySpecifiesActions) &&
                    (*expectedCalls[i] == *actualCall))
                {
                    result = expectedCalls[i];
                    break;
                }
            }
        }

        return result;
    }

protected:
    size_t m_ExpectedCallIndex;
};

#endif // STRICTORDEREDCALLCOMPARER_H
