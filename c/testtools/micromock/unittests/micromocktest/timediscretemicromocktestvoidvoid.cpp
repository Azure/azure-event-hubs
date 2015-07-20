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

void a(void);
void b(void);

TD_MOCK_CLASS(MyMocks)
{
public: 
    MOCK_STATIC_TD_METHOD_0(, void, a)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_TD_METHOD_0(, void, b)
    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_0(MyMocks, , void, a)
DECLARE_GLOBAL_MOCK_METHOD_0(MyMocks, , void, b)


static int iCall=0;
static void theTask(void)
{
    if(iCall==1)
    {
        a();
    }    
    if(iCall==10)
    {
        a();
        b();
    }
    iCall++;
    
}
static HANDLE g_hMutex=NULL;

BEGIN_TEST_SUITE(MicroMockVoidVoidTest)

#ifdef _MSC_VER
    TEST_SUITE_INITIALIZE(a)
    {
            g_hMutex = CreateMutex(NULL, FALSE, _T("onlyOneTestAtATime5"));
            ASSERT_ARE_NOT_EQUAL(HANDLE, (HANDLE)NULL, g_hMutex);
    }

    TEST_SUITE_CLEANUP(b)
    {
        if (g_hMutex != NULL)
        {
            CloseHandle(g_hMutex);
            g_hMutex = NULL;
        }
    }
     

        #pragma warning(disable: 26135)
    TEST_FUNCTION_INITIALIZE(c)
    {
        DWORD r = WaitForSingleObject(g_hMutex, INFINITE);
        if (r != WAIT_OBJECT_0)
        {
            ASSERT_FAIL(_T("WAitForSingleOPbject failed"));
        }
        iCall = 0;
    }

    TEST_FUNCTION_CLEANUP(d)
    {
        if (ReleaseMutex(g_hMutex) == 0)
        {
            ASSERT_FAIL(_T("ReleaseMutex(hOneByOne)==0"));
        }
    }

    TEST_FUNCTION(DiscreteMicroMock_ThereIsNothingCalledAtTimeZero)
    {
        // arrange
        MyMocks mocks(theTask);

        // act
        mocks.RunUntilTick(0);

        // assert - left to uMTD (test would fail if at time = 0 a() would be called*/
    }

    TEST_FUNCTION(DiscreteMicroMock_AisCalledAtTimeOne)
    {
        // arrange
        MyMocks mocks(theTask);
        STRICT_EXPECTED_CALL_AT(mocks, 1, a());

        // act
        mocks.RunUntilTick(1);

        // assert - left to uMTD - this test together with the test before guarantee that at t=1 precisely a() is called
    }

    TEST_FUNCTION(DiscreteMicroMock_AisNotCalledAtTimeTwo)
    {
        // arrange
        MyMocks mocks(theTask);
        STRICT_EXPECTED_CALL_AT(mocks, 1, a());

        // act
        mocks.RunUntilTick(2);

        // assert - left to uMTD
    }

    TEST_FUNCTION(DiscreteMicroMock_InTheSameTickOrderOfCallsIsDisregarded_1)
    {
        ///arrange
        MyMocks mocks(theTask);
        STRICT_EXPECTED_CALL_AT(mocks, 1, a());
        STRICT_EXPECTED_CALL_AT(mocks, 10, a());
        STRICT_EXPECTED_CALL_AT(mocks, 10, b());

        ///act
        mocks.RunUntilTick(20);

        ///assert - left to uM_TD
    }

    TEST_FUNCTION(DiscreteMicroMock_InTheSameTickOrderOfCallsIsDisregarded_2)
    {
        ///arrange
        MyMocks mocks(theTask);
        STRICT_EXPECTED_CALL_AT(mocks, 1, a());
        STRICT_EXPECTED_CALL_AT(mocks, 10, b());
        STRICT_EXPECTED_CALL_AT(mocks, 10, a());

        ///act
        mocks.RunUntilTick(20);

        ///assert - left to uM_TD
    }
#endif

END_TEST_SUITE(MicroMockVoidVoidTest)


