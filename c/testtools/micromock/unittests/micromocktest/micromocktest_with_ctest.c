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

#include "testrunnerswitcher.h"
int main(void)
{
	RUN_TEST_SUITE(CMockValue_tests);
	RUN_TEST_SUITE(MicroMockCallComparisonUnitTests);
	RUN_TEST_SUITE(MicroMockTest);
	RUN_TEST_SUITE(MicroMockValidateArgumentBufferTests);
	RUN_TEST_SUITE(NULLArgsStringificationTests);
	RUN_TEST_SUITE(MicroMockVoidVoidTest);
	RUN_TEST_SUITE(MicroMockStimTest);
	RUN_TEST_SUITE(MicroMockTestWithReturnAndParameters);
	return 0;
}

