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
#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "micromock.h"
#include "testrunnerswitcher.h"
#include "crt_abstractions.h"
#include "lock.h"


DEFINE_MICROMOCK_ENUM_TO_STRING(LOCK_RESULT, LOCK_RESULT_VALUES);

LOCK_RESULT Lock_Handle_ToString(LOCK_HANDLE handle)
{
	LOCK_RESULT result = LOCK_OK;

	if (handle == NULL)
	{
		result = LOCK_ERROR;
	}

	return result;
}

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(Lock_UnitTests)

TEST_SUITE_INITIALIZE(a)
{
    INITIALIZE_MEMORY_DEBUG(g_dllByDll);
}
TEST_SUITE_CLEANUP(b)
{
    DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}
TEST_FUNCTION(Test_Lock_Lock_Unlock)
{
	//arrange
	LOCK_HANDLE handle=NULL;
	LOCK_RESULT result;
	//act
	handle =Lock_Init();
    auto res = Lock_Handle_ToString(handle);
	//assert
	ASSERT_ARE_EQUAL(LOCK_RESULT, LOCK_OK, res);

	result=Lock(handle);
	/*Tests_SRS_LOCK_99_005:[ This API on success should return LOCK_OK]*/
	ASSERT_ARE_EQUAL(LOCK_RESULT, LOCK_OK, result);
	/*Tests_SRS_LOCK_99_009:[ This API on success should return LOCK_OK]*/
	result = Unlock(handle);
	ASSERT_ARE_EQUAL(LOCK_RESULT, LOCK_OK, result);
	//free
	result = Lock_Deinit(handle);
	ASSERT_ARE_EQUAL(LOCK_RESULT, LOCK_OK, result);
}

TEST_FUNCTION(Test_Lock_Init_DeInit)
{
	//arrange
	LOCK_HANDLE handle = NULL;
	//act
	handle = Lock_Init();
	//assert
	/*Tests_SRS_LOCK_99_002:[ This API on success will return a valid lock handle which should be a non NULL value]*/
    auto res = Lock_Handle_ToString(handle);
	//assert
	ASSERT_ARE_EQUAL(LOCK_RESULT, LOCK_OK, res);
	//free
	LOCK_RESULT result = Lock_Deinit(handle);
	ASSERT_ARE_EQUAL(LOCK_RESULT, LOCK_OK, result);
}

/*Tests_SRS_LOCK_99_007:[ This API on NULL handle passed returns LOCK_ERROR]*/
TEST_FUNCTION(Test_Lock_Lock_NULL)
{
	//arrange
	//act
	LOCK_RESULT result =Lock(NULL);
	/*Tests_SRS_LOCK_99_007:[ This API on NULL handle passed returns LOCK_ERROR]*/
	ASSERT_ARE_EQUAL(LOCK_RESULT, LOCK_ERROR, result);
}

/*Tests_SRS_LOCK_99_011:[ This API on NULL handle passed returns LOCK_ERROR]*/
TEST_FUNCTION(Test_Lock_Unlock_NULL)
{
	//arrange
	//act
	LOCK_RESULT result = Unlock(NULL);
	/*Tests_SRS_LOCK_99_011:[ This API on NULL handle passed returns LOCK_ERROR]*/
	ASSERT_ARE_EQUAL(LOCK_RESULT, LOCK_ERROR, result);
}

TEST_FUNCTION(Test_Lock_Init_DeInit_NULL)
{
	//arrange
	//act
	LOCK_RESULT result = Lock_Deinit(NULL);
	//assert
	/*Tests_SRS_LOCK_99_013:[ This API on NULL handle passed returns LOCK_ERROR]*/
	ASSERT_ARE_EQUAL(LOCK_RESULT, LOCK_ERROR, result);
}


END_TEST_SUITE(Lock_UnitTests);
