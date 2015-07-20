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

#include "ctest.h"

static int x = 0;
static int y = 0;

CTEST_BEGIN_TEST_SUITE(TestFunctionCleanupTests)

CTEST_FUNCTION_INITIALIZE()
{
}

CTEST_FUNCTION_CLEANUP()
{
    x++;
}

CTEST_FUNCTION(Test1)
{
    CTEST_ASSERT_ARE_EQUAL(int, y, x);
    y++;
}

CTEST_FUNCTION(Test2)
{
    CTEST_ASSERT_ARE_EQUAL(int, y, x);
    y++;
}

CTEST_FUNCTION(Test3)
{
    CTEST_ASSERT_ARE_EQUAL(int, y, x);
    y++;
}

CTEST_END_TEST_SUITE(TestFunctionCleanupTests)
