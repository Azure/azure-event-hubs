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

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "eventdata.h"
#include "buffer_.h"
#include "strings.h"
#include "lock.h"
#include "vector.h"
#include "crt_abstractions.h"
#include "map.h"

DEFINE_MICROMOCK_ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_RESULT_VALUES);

#define GBALLOC_H

extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{
    /*if malloc is defined as gballoc_malloc at this moment, there'd be serious trouble*/
    #define Lock(x) (LOCK_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
    #define Unlock(x) (LOCK_OK + gballocState - gballocState)
    #define Lock_Init() (LOCK_HANDLE)0x42
    #define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
    #include "gballoc.c"
    #include "vector.c"
    #include "strings.c"
    #undef Lock
    #undef Unlock
    #undef Lock_Init
    #undef Lock_Deinit
};

#define NUMBER_OF_CHAR      8

static const BUFFER_HANDLE TEST_BUFFER_HANDLE = (BUFFER_HANDLE)0x42;

static MICROMOCK_MUTEX_HANDLE g_testByTest;

static const char* PARTITION_KEY_VALUE = "EventDataPartitionKey";
static const char* PARTITION_KEY_ZERO_VALUE = "";
static const char* PROPERTY_NAME = "EventDataPropertyName";
static const char* PROPERTY_NAME_INIT = "Initialize";
static const char* PROPERTY_NAME_2 = "EventDataPropertyName2";
static const char* PROPERTY_VALUE_2 = "Value2";

#define TEST_STRING_HANDLE (STRING_HANDLE)0x46
#define TEST_BUFFER_SIZE 6
#define INITIALIZE_BUFFER_SIZE 256

static unsigned char TEST_BUFFER_VALUE[] = { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47 };
static const char TEST_STRING_VALUE[] = "Property_String_Value_1";
static const char TEST_STRING_VALUE2[] = "Property_String_Value_2";
static const char EMPTY_STRING[] = "";

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

static size_t g_currentvector_call;
static size_t g_whenShallvector_fail;

static size_t g_currentvectorElement_call;
static size_t g_whenShallvectorElement_fail;

static size_t g_vector_Pushback_fail;

static size_t g_bufferNewFail;
static size_t g_lockInitFail;

static size_t g_currentBufferClone_call;
static size_t g_whenBufferClone_fail;

static size_t g_currentStringHandle_call;
static size_t g_whenStringHandle_fail;

static size_t g_currentStringClone_call;
static size_t g_whenStringClone_fail;

static size_t g_currentStringCompareAreEqual;
static size_t g_whenStringCompareAreEqual;

static size_t g_currentlock_call;
static size_t g_whenShalllock_fail;

static size_t currentMap_Create_call;
static size_t whenShallMap_Create_fail;

static size_t currentMap_Clone_call;
static size_t whenShallMap_Clone_fail;

MAP_FILTER_CALLBACK g_mapFilterFunc;

typedef struct EVENT_PROPERTY_TEST_TAG
{
    STRING_HANDLE key;
    BUFFER_HANDLE value;
} EVENT_PROPERTY_TEST;

typedef struct LOCK_TEST_STRUCT_TAG
{
    char* dummy;
} LOCK_TEST_STRUCT;

// ** Mocks **
TYPED_MOCK_CLASS(CEventDataMocks, CGlobalMock)
{
public:
    /* Buffer mocks */
    MOCK_STATIC_METHOD_0(, BUFFER_HANDLE, BUFFER_new)
        BUFFER_HANDLE handle;
        if (g_bufferNewFail > 0)
        {
            handle = NULL;
        }
        else
        {
            handle = (BUFFER_HANDLE)malloc(1);
        }
    MOCK_METHOD_END(BUFFER_HANDLE, handle)

    MOCK_STATIC_METHOD_1(, size_t, BUFFER_length, BUFFER_HANDLE, handle)
    MOCK_METHOD_END(size_t, 0)

    MOCK_STATIC_METHOD_1(, void, BUFFER_delete, BUFFER_HANDLE, handle)
        if (handle != NULL)
        {
            free(handle);
        }
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, BUFFER_clone, BUFFER_HANDLE, handle)
        g_currentBufferClone_call++;
        BUFFER_HANDLE handleClone;
        if (handle == NULL)
        {
            handleClone = NULL;
        }
        else
        {
            if (g_whenBufferClone_fail > 0 && g_currentBufferClone_call == g_whenBufferClone_fail)
            {
                handleClone = NULL;
            }
            else
            {
                handleClone = (BUFFER_HANDLE)malloc(1);
            }
        }
    MOCK_METHOD_END(BUFFER_HANDLE, handleClone)

    MOCK_STATIC_METHOD_3(, int, BUFFER_build, BUFFER_HANDLE, handle, const unsigned char*, source, size_t, size)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, BUFFER_content, BUFFER_HANDLE, handle, const unsigned char**, content)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, BUFFER_size, BUFFER_HANDLE, handle, size_t*, size)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, unsigned char*, BUFFER_u_char, BUFFER_HANDLE, handle)
    MOCK_METHOD_END(unsigned char*, TEST_BUFFER_VALUE)

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, handle)
        BASEIMPLEMENTATION::STRING_delete(handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, psz)
        g_currentStringHandle_call++;
        STRING_HANDLE handle;
        if (psz == NULL)
        {
            handle = NULL;
        }
        else
        {
            if (g_whenStringHandle_fail > 0 && g_currentStringHandle_call == g_whenStringHandle_fail)
            {
                handle = NULL;
            }
            else
            {
                handle = BASEIMPLEMENTATION::STRING_construct(psz);
            }
        }
    MOCK_METHOD_END(STRING_HANDLE, handle)

    MOCK_STATIC_METHOD_1(, const char*, STRING_c_str, STRING_HANDLE, s)
    MOCK_METHOD_END(const char*, BASEIMPLEMENTATION::STRING_c_str(s) )

    MOCK_STATIC_METHOD_2(, int, STRING_compare, STRING_HANDLE, s1, STRING_HANDLE, s2)
        int compareResult = 1;
        g_currentStringCompareAreEqual++;
        if (g_whenStringCompareAreEqual > 0)
        {
            if (g_currentStringCompareAreEqual >= g_whenStringCompareAreEqual)
            {
                compareResult = 0;
            }
            else
            {
                compareResult = BASEIMPLEMENTATION::STRING_compare(s1, s2);
            }
        }
        else
        {
            compareResult = BASEIMPLEMENTATION::STRING_compare(s1, s2);
        }
    MOCK_METHOD_END(int, compareResult)

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_clone, STRING_HANDLE, handle)
        g_currentStringHandle_call++;
        STRING_HANDLE handleClone;
        if (handle == NULL)
        {
            handleClone = NULL;
        }
        else
        {
            if (g_whenStringClone_fail > 0 && g_currentStringHandle_call == g_whenStringClone_fail)
            {
                handleClone = NULL;
            }
            else
            {
                handleClone = BASEIMPLEMENTATION::STRING_clone(handle);
            }
        }
    MOCK_METHOD_END(STRING_HANDLE, handleClone)

    MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
        VECTOR_HANDLE handle;
        g_currentvector_call++;
        if (g_whenShallvector_fail > 0)
        {
            if (g_currentvector_call == g_whenShallvector_fail)
            {
                handle = NULL;
            }
            else
            {
                handle = BASEIMPLEMENTATION::VECTOR_create(elementSize);
            }
        }
        else
        {
            handle = BASEIMPLEMENTATION::VECTOR_create(elementSize);
        }
    MOCK_METHOD_END(VECTOR_HANDLE, handle)

    MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, handle)
        BASEIMPLEMENTATION::VECTOR_destroy(handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, void, VECTOR_clear, VECTOR_HANDLE, handle)
        BASEIMPLEMENTATION::VECTOR_clear(handle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, void*, VECTOR_element, VECTOR_HANDLE, handle, size_t, index)
        void* element;
        g_currentvectorElement_call++;
        if (g_whenShallvectorElement_fail > 0)
        {
            if (g_currentvectorElement_call == g_whenShallvectorElement_fail)
            {
                element = NULL;
            }
            else
            {
                element = BASEIMPLEMENTATION::VECTOR_element(handle, index);
            }
        }
        else
        {
            element = BASEIMPLEMENTATION::VECTOR_element(handle, index);
        }
    MOCK_METHOD_END(void*, element)

    MOCK_STATIC_METHOD_1(, size_t, VECTOR_size, VECTOR_HANDLE, handle)
        size_t size = BASEIMPLEMENTATION::VECTOR_size(handle);
    MOCK_METHOD_END(size_t, size)

    MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements)
        int resultPushback;
        if (g_vector_Pushback_fail > 0)
        {
            resultPushback = __LINE__;
        }
        else
        {
            resultPushback = BASEIMPLEMENTATION::VECTOR_push_back(handle, elements, numElements);
        }
    MOCK_METHOD_END(int, resultPushback)

    MOCK_STATIC_METHOD_3(, void*, VECTOR_find_if, VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value)
        void* elementFind;
        g_currentvector_call++;
        if (g_whenShallvector_fail > 0)
        {
            if (g_currentvector_call == g_whenShallvector_fail)
            {
                elementFind = NULL;
            }
            else
            {
                elementFind = BASEIMPLEMENTATION::VECTOR_find_if(handle, pred, value);
            }
        }
        else
        {
            elementFind = BASEIMPLEMENTATION::VECTOR_find_if(handle, pred, value);
        }
    MOCK_METHOD_END(void*, elementFind)

    /*Memory allocation*/
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2;
        currentmalloc_call++;
        if (whenShallmalloc_fail>0)
        {
            if (currentmalloc_call == whenShallmalloc_fail)
            {
                result2 = NULL;
            }
            else
            {
                result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
            }
        }
        else
        {
            result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
        }
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_2(, void*, gballoc_realloc, void*, ptr, size_t, size)
        MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_realloc(ptr, size));

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_0(, LOCK_HANDLE, Lock_Init)
        LOCK_HANDLE handle;
        if (g_lockInitFail > 0)
        {
            handle = NULL;
        }
        else
        {
            LOCK_TEST_STRUCT* lockTest = (LOCK_TEST_STRUCT*)malloc(sizeof(LOCK_TEST_STRUCT) );
            handle = lockTest;
        }
    MOCK_METHOD_END(LOCK_HANDLE, handle);

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock, LOCK_HANDLE, handle)
        LOCK_RESULT lockResult;

        g_currentlock_call++;
        if (g_whenShalllock_fail > 0)
        {
            if (g_currentlock_call == g_whenShalllock_fail)
            {
                lockResult = LOCK_ERROR;
            }
            else
            {
                lockResult = LOCK_OK;
            }
        }
        else
        {
            lockResult = LOCK_OK;
        }
        if (lockResult == LOCK_OK && handle != NULL)
        {
            LOCK_TEST_STRUCT* lockTest = (LOCK_TEST_STRUCT*)handle;
            lockTest->dummy = (char*)malloc(1);
        }
    MOCK_METHOD_END(LOCK_RESULT, lockResult);
    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Unlock, LOCK_HANDLE, handle)
        if (handle != NULL)
        {
            LOCK_TEST_STRUCT* lockTest = (LOCK_TEST_STRUCT*)handle;
            free(lockTest->dummy);
        }
    MOCK_METHOD_END(LOCK_RESULT, LOCK_OK);

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle)
        free(handle);
    MOCK_METHOD_END(LOCK_RESULT, LOCK_OK);

    MOCK_STATIC_METHOD_1(, MAP_HANDLE, Map_Create, MAP_FILTER_CALLBACK, mapFilterFunc)
        MAP_HANDLE result2;
        g_mapFilterFunc = mapFilterFunc;
        currentMap_Create_call++;
        if (currentMap_Create_call == whenShallMap_Create_fail)
        {
            result2 = (MAP_HANDLE)NULL;
        }
        else
        {
            result2 = (MAP_HANDLE)malloc(1);
        }
    MOCK_METHOD_END(MAP_HANDLE, result2)

    MOCK_STATIC_METHOD_1(, MAP_HANDLE, Map_Clone, MAP_HANDLE, handle)
        MAP_HANDLE result2;
        currentMap_Clone_call++;
        if (currentMap_Clone_call == whenShallMap_Clone_fail)
        {
            result2 = (MAP_HANDLE)NULL;
        }
        else
        {
            result2 = (MAP_HANDLE)malloc(1);
        }
    MOCK_METHOD_END(MAP_HANDLE, result2)


    MOCK_STATIC_METHOD_1(, void, Map_Destroy, MAP_HANDLE, handle)
        free(handle);
    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_0(CEventDataMocks, , BUFFER_HANDLE, BUFFER_new);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , void, BUFFER_delete, BUFFER_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventDataMocks, , int, BUFFER_build, BUFFER_HANDLE, handle, const unsigned char*, source, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventDataMocks, , int, BUFFER_content, BUFFER_HANDLE, handle, const unsigned char**, content);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventDataMocks, , int, BUFFER_size, BUFFER_HANDLE, handle, size_t*, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , unsigned char*, BUFFER_u_char, BUFFER_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , size_t, BUFFER_length, BUFFER_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , BUFFER_HANDLE, BUFFER_clone, BUFFER_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , void, STRING_delete, STRING_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , STRING_HANDLE, STRING_construct, const char*, psz);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , const char*, STRING_c_str, STRING_HANDLE, s);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventDataMocks, , int, STRING_compare, STRING_HANDLE, s1, STRING_HANDLE, s2);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , STRING_HANDLE, STRING_clone, STRING_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , void, VECTOR_destroy, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , void, VECTOR_clear, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventDataMocks, , void*, VECTOR_element, VECTOR_HANDLE, handle, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , size_t, VECTOR_size, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventDataMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventDataMocks, , void*, VECTOR_find_if, VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventDataMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , void, gballoc_free, void*, ptr)
DECLARE_GLOBAL_MOCK_METHOD_0(CEventDataMocks, , LOCK_HANDLE, Lock_Init);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , LOCK_RESULT, Lock, LOCK_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , LOCK_RESULT, Unlock, LOCK_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle)

DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , MAP_HANDLE, Map_Create, MAP_FILTER_CALLBACK, mapFilterFunc);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , void, Map_Destroy, MAP_HANDLE, handle)
DECLARE_GLOBAL_MOCK_METHOD_1(CEventDataMocks, , MAP_HANDLE, Map_Clone, MAP_HANDLE, handle);

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(eventdata_unittests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    INITIALIZE_MEMORY_DEBUG(g_dllByDll);

    g_testByTest = MicroMockCreateMutex();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    MicroMockDestroyMutex(g_testByTest);
    DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (!MicroMockAcquireMutex(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
    g_currentvector_call = 0;
    g_whenShallvector_fail = 0;

    g_currentvectorElement_call = 0;
    g_whenShallvectorElement_fail = 0;

    g_currentBufferClone_call = 0;
    g_whenBufferClone_fail = 0;

    g_currentStringHandle_call = 0;
    g_whenStringHandle_fail = 0;

    g_currentStringClone_call = 0;
    g_whenStringClone_fail = 0;

    g_whenStringCompareAreEqual = 0;
    g_currentStringCompareAreEqual = 0;

    g_vector_Pushback_fail = 0;
    g_bufferNewFail = 0;
    g_lockInitFail = 0;

    g_currentlock_call = 0;
    g_whenShalllock_fail = 0;

    currentMap_Create_call = 0;
    whenShallMap_Create_fail = 0;

    currentMap_Clone_call = 0;
    whenShallMap_Clone_fail = 0;

    g_mapFilterFunc = NULL;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    if (!MicroMockReleaseMutex(g_testByTest))
    {
        ASSERT_FAIL("failure in test framework at ReleaseMutex");
    }
}

/* EventData_CreateWithNewMemory */

/* Tests_SRS_EVENTDATA_03_003: [EventData_Create shall return a NULL value if length is not zero and data is NULL.] */
/* Tests_SRS_EVENTDATA_03_002: [EventData_CreateWithNewMemory shall provide a none-NULL handle encapsulating the storage of the data provided.] */
TEST_FUNCTION(EventData_CreateWithNewMemory_with_zero_length_and_null_data_Succeeds)
{
    // arrange
    CNiceCallComparer<CEventDataMocks> niceMocks;

    // act
    EVENTDATA_HANDLE result = EventData_CreateWithNewMemory(NULL, 0);

    // assert
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    EventData_Destroy(result);
}

/* Tests_SRS_EVENTDATA_03_003: [If data is not NULL and length is zero, EventData_Create shall return a NULL value.]  */
/* Tests_SRS_EVENTDATA_03_002: [EventData_CreateWithNewMemory shall provide a none-NULL handle encapsulating the storage of the data provided.] */
TEST_FUNCTION(EventData_CreateWithNewMemory_with_zero_length_and_none_null_data_Succeeds)
{
    // arrange
    CNiceCallComparer<CEventDataMocks> niceMocks;
    unsigned char myData[] = { 0x42, 0x43, 0x44 };

    // act
    EVENTDATA_HANDLE result = EventData_CreateWithNewMemory(myData, 0);

    // assert
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    EventData_Destroy(result);
}

/* Tests_SRS_EVENTDATA_03_003: [EventData_Create shall return a NULL value if length is not zero and data is NULL.] */
/* Tests_SRS_EVENTDATA_03_002: [EventData_CreateWithNewMemory shall provide a none-NULL handle encapsulating the storage of the data provided.] */
TEST_FUNCTION(EventData_CreateWithNewMemory_with_none_zero_length_and_none_null_data_Succeeds)
{
    // arrange
    CNiceCallComparer<CEventDataMocks> niceMocks;
    unsigned char myData[] = { 0x42, 0x43, 0x44 };
    size_t length = sizeof(myData);

    // act
    EVENTDATA_HANDLE result = EventData_CreateWithNewMemory(myData, length);

    // assert
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    EventData_Destroy(result);
}

/* Tests_SRS_EVENTDATA_03_003: [EventData_Create shall return a NULL value if length is not zero and data is NULL.] */
TEST_FUNCTION(EventData_CreateWithNewMemory_with_none_zero_length_and_null_data_Fails)
{
    // arrange
    CNiceCallComparer<CEventDataMocks> niceMocks;

    // act
    EVENTDATA_HANDLE result = EventData_CreateWithNewMemory(NULL, (size_t)25);

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_EVENTDATA_03_002: [EventData_CreateWithNewMemory shall provide a none-NULL handle encapsulating the storage of the data provided.] */
TEST_FUNCTION(EventData_CreateWithNewMemory_with_none_null_data_and_none_matching_length_Succeeds)
{
    // arrange
    CNiceCallComparer<CEventDataMocks> niceMocks;
    unsigned char myData[] = { 0x42, 0x43, 0x44 };
    size_t length = 1;

    // act
    EVENTDATA_HANDLE result = EventData_CreateWithNewMemory(myData, length);

    // assert
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    EventData_Destroy(result);
}

/* EventData_Destroy */

/* Tests_SRS_EVENTDATA_03_005: [EventData_Destroy shall deallocate all resources related to the eventDataHandle specified.] */
TEST_FUNCTION(EvenData_Destroy_with_valid_handle_Succeeds)
{
    // arrange
    CNiceCallComparer<CEventDataMocks> niceMocks;
    unsigned char myData[] = { 0x42, 0x43, 0x44 };
    size_t length = sizeof(myData);
    EVENTDATA_HANDLE eventHandle = EventData_CreateWithNewMemory(myData, length);

    // act
    EventData_Destroy(eventHandle);

    // assert
    // Implicit - No crash
}

/* Tests_SRS_EVENTDATA_03_006: [EventData_Destroy shall not do anything if eventDataHandle is NULL.] */
TEST_FUNCTION(EvenData_Destroy_with_NULL_handle_NoAction)
{
    // arrange
    CNiceCallComparer<CEventDataMocks> niceMocks;

    // act
    EventData_Destroy(NULL);

    // assert
    // Implicit - No crash
}


/* EventData_GetData */

/* Tests_SRS_EVENTDATA_03_022: [If any of the arguments passed to EventData_GetData is NULL, EventData_GetData shall return EVENTDATA_INVALID_ARG.] */
TEST_FUNCTION(EventData_GetData_with_NULL_handle_fails)
{
    // arrange
    CEventDataMocks mocks;
    const unsigned char* buffer;
    size_t size;

    // act
    EVENTDATA_RESULT result = EventData_GetData(NULL, &buffer, &size);

    // assert
    ASSERT_ARE_EQUAL(EVENTDATA_RESULT, EVENTDATA_INVALID_ARG, result);

    // cleanup
}

/* Tests_SRS_EVENTDATA_03_022: [If any of the arguments passed to EventData_GetData is NULL, EventData_GetData shall return EVENTDATA_INVALID_ARG.] */
TEST_FUNCTION(EventData_GetData_with_NULL_buffer_fails)
{
    // arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    size_t expectedSize = sizeof(expectedData);
    size_t actualSize;
    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    // act
    EVENTDATA_RESULT result = EventData_GetData(eventDataHandle, NULL, &actualSize);

    // assert
    ASSERT_ARE_EQUAL(EVENTDATA_RESULT, EVENTDATA_INVALID_ARG, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTDATA_03_022: [If any of the arguments passed to EventData_GetData is NULL, EventData_GetData shall return EVENTDATA_INVALID_ARG.] */
TEST_FUNCTION(EventData_GetData_with_NULL_size_fails)
{
    // arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    size_t expectedSize = sizeof(expectedData);
    const unsigned char* actualDdata;
    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    // act
    EVENTDATA_RESULT result = EventData_GetData(eventDataHandle, &actualDdata, NULL);

    // assert
    ASSERT_ARE_EQUAL(EVENTDATA_RESULT, EVENTDATA_INVALID_ARG, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTDATA_03_019: [EventData_GetData shall provide a pointer and size for the data associated with the eventDataHandle.] */
/* Tests_SRS_EVENTDATA_03_020: [The pointer shall be obtained by using BUFFER_content and it shall be copied in the buffer argument. The size of the associated data shall be obtained by using BUFFER_size and it shall be copied to the size argument.] */
/* Tests_SRS_EVENTDATA_03_021: [On success, EventData_GetData shall return EVENTDATA_OK.] */
TEST_FUNCTION(EventData_GetData_with_valid_args_retrieves_data_and_size_from_the_underlying_BUFFER_Successfully)
{
    // arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    unsigned char* testBuffer = expectedData;
    size_t expectedSize = sizeof(expectedData);
    const unsigned char* actualDdata;
    size_t actualSize;
    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, BUFFER_content(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &testBuffer, sizeof(testBuffer));
    EXPECTED_CALL(mocks, BUFFER_size(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &expectedSize, sizeof(expectedSize));

    // act
    EVENTDATA_RESULT result = EventData_GetData(eventDataHandle, &actualDdata, &actualSize);

    // assert
    ASSERT_ARE_EQUAL(EVENTDATA_RESULT, EVENTDATA_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, testBuffer, actualDdata);
    ASSERT_ARE_EQUAL(size_t, expectedSize, actualSize);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTDATA_03_023: [If EventData_GetData fails because of any other error it shall return EVENTDATA_ERROR.]*/
TEST_FUNCTION(When_BUFFER_size_fails_EventData_GetData_fails)
{
    // arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    unsigned char* testBuffer = expectedData;
    size_t expectedSize = sizeof(expectedData);
    const unsigned char* actualDdata;
    size_t actualSize;
    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, BUFFER_content(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &testBuffer, sizeof(testBuffer));
    EXPECTED_CALL(mocks, BUFFER_size(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &expectedSize, sizeof(expectedSize))
        .SetReturn(1);

    // act
    EVENTDATA_RESULT result = EventData_GetData(eventDataHandle, &actualDdata, &actualSize);

    // assert
    ASSERT_ARE_EQUAL(EVENTDATA_RESULT, EVENTDATA_ERROR, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTDATA_03_023: [If EventData_GetData fails because of any other error it shall return EVENTDATA_ERROR.]*/
TEST_FUNCTION(When_BUFFER_content_fails_EventData_GetData_fails)
{
    // arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    unsigned char* testBuffer = expectedData;
    size_t expectedSize = sizeof(expectedData);
    const unsigned char* actualDdata;
    size_t actualSize;
    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, BUFFER_content(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &testBuffer, sizeof(testBuffer))
        .SetReturn(1);

    // act
    EVENTDATA_RESULT result = EventData_GetData(eventDataHandle, &actualDdata, &actualSize);

    // assert
    ASSERT_ARE_EQUAL(EVENTDATA_RESULT, EVENTDATA_ERROR, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTDATA_07_031: [EventData_SetPartitionKey shall return EVENTDATA_INVALID_ARG if eventDataHandle parameter is NULL.] */
TEST_FUNCTION(EventData_SetPartitionKey_EVENTDATA_HANDLE_NULL_FAIL)
{
    // arrange
    CEventDataMocks mocks;
    EVENTDATA_RESULT result;
    mocks.ResetAllCalls();

    // act
    result = EventData_SetPartitionKey(NULL, PARTITION_KEY_VALUE);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTDATA_INVALID_ARG, result);

    mocks.AssertActualAndExpectedCalls();

    // cleanup
}

/* Tests_SRS_EVENTDATA_07_029: [if the partitionKey parameter is NULL EventData_SetPartitionKey shall not assign any value and return EVENTDATA_OK.] */
TEST_FUNCTION(EventData_SetPartitionKey_Partition_key_NULL_FAIL)
{
    // arrange
    CEventDataMocks mocks;
    EVENTDATA_RESULT result;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    size_t expectedSize = sizeof(expectedData);
    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    // act
    result = EventData_SetPartitionKey(eventDataHandle, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTDATA_OK, result);

    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTDATA_07_030: [On Success EventData_SetPartitionKey shall return EVENTDATA_OK.] */
/* Tests_SRS_EVENTDATA_07_028: [On success EventData_SetPartitionKey shall store the const char* partitionKey parameter in the EVENTDATA_HANDLE data structure partitionKey variable.] */
TEST_FUNCTION(EventData_SetPartitionKey_Partition_Key_SUCCEED)
{
    // arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    size_t expectedSize = sizeof(expectedData);
    EVENTDATA_RESULT result;

    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, STRING_construct(PARTITION_KEY_VALUE));

    // act
    result = EventData_SetPartitionKey(eventDataHandle, PARTITION_KEY_VALUE);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTDATA_OK, result);

    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTDATA_07_029: [if the partitionKey parameter is a zero length string EventData_SetPartitionKey shall return a nonzero value and will remove an existing partition Key value.] */
TEST_FUNCTION(EventData_SetPartitionKey_Partition_key_Zero_String_SUCCEED)
{
    // arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    size_t expectedSize = sizeof(expectedData);
    EVENTDATA_RESULT result;

    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, STRING_construct(PARTITION_KEY_ZERO_VALUE));

    // act
    result = EventData_SetPartitionKey(eventDataHandle, PARTITION_KEY_ZERO_VALUE);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTDATA_OK, result);

    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTDATA_07_027: [If the partitionKey variable contained in the eventDataHandle parameter is not NULL then EventData_SetPartitionKey shall delete the partitionKey STRING_HANDLE.] */
TEST_FUNCTION(EventData_SetPartitionKey_Partition_key_NOT_NULL_SUCCEED)
{
    // arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    size_t expectedSize = sizeof(expectedData);
    EVENTDATA_RESULT result;

    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(mocks, STRING_construct(PARTITION_KEY_VALUE)).ExpectedTimesExactly(2);

    // act
    result = EventData_SetPartitionKey(eventDataHandle, PARTITION_KEY_VALUE);
    ASSERT_ARE_EQUAL(int, EVENTDATA_OK, result);

    result = EventData_SetPartitionKey(eventDataHandle, PARTITION_KEY_VALUE);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTDATA_OK, result);

    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTDATA_07_024: [EventData_GetPartitionKey shall return NULL if the eventDataHandle parameter is NULL.] */
TEST_FUNCTION(EventData_GetPartitionKey_EVENTDATA_HANDLE_NULL_FAIL)
{
    // arrange

    // act
    const char* result = EventData_GetPartitionKey(NULL);

    // assert
    ASSERT_IS_NULL(result);

    // cleanup
}

/* Tests_SRS_EVENTDATA_07_025: [EventData_GetPartitionKey shall return NULL if the partitionKey is in the EVENTDATA_HANDLE is NULL.] */
TEST_FUNCTION(EventData_GetPartitionKey_PartitionKey_NULL_FAIL)
{
    // arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    size_t expectedSize = sizeof(expectedData);

    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    // act
    const char* result = EventData_GetPartitionKey(eventDataHandle);

    // assert
    ASSERT_IS_NULL(result);

    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTDATA_07_026: [On success EventData_GetPartitionKey shall return a const char* variable that is pointing to the Partition Key value that is stored in the EVENTDATA_HANDLE.] */
TEST_FUNCTION(EventData_GetPartitionKey_SUCCEED)
{
    // arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    size_t expectedSize = sizeof(expectedData);

    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    int partitionKeyRet = EventData_SetPartitionKey(eventDataHandle, PARTITION_KEY_VALUE);
    ASSERT_ARE_EQUAL(int, 0, partitionKeyRet);

    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    // act
    const char* result = EventData_GetPartitionKey(eventDataHandle);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, result, PARTITION_KEY_VALUE);

    mocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

TEST_FUNCTION(EventData_Properties_with_NULL_handle_returns_NULL)
{
    ///arrange
    CEventDataMocks mocks;
    mocks.ResetAllCalls();

    ///act
    MAP_HANDLE result = EventData_Properties(NULL);

    ///assert
    ASSERT_IS_NULL(result);
    mocks.AssertActualAndExpectedCalls();

    ///cleanup
}

TEST_FUNCTION(EventData_Map_Filter_Succeed)
{
    ///arrange
    CEventDataMocks mocks;
    unsigned char expectedData[] = { 0x42, 0x43, 0x44 };
    size_t expectedSize = sizeof(expectedData);

    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(expectedData, expectedSize);
    mocks.ResetAllCalls();

    ASSERT_IS_NOT_NULL(g_mapFilterFunc);

    ///act
    char* validNameChar = "validNameChar";
    char* validValueChar = "validValueChar";

    char invalidNameChar[NUMBER_OF_CHAR];
    char invalidValueChar[NUMBER_OF_CHAR];
    for (size_t index = 0; index < NUMBER_OF_CHAR; index++)
    {
        invalidNameChar[index] = (char)index+2;
        invalidValueChar[index] = (char)index+2;
    }

    auto result1 = g_mapFilterFunc(validNameChar, validValueChar);
    auto result2 = g_mapFilterFunc(invalidNameChar, invalidValueChar);
    auto result3 = g_mapFilterFunc(invalidNameChar, validValueChar);
    auto result4 = g_mapFilterFunc(validNameChar, invalidValueChar);
    auto result5 = g_mapFilterFunc(NULL, validValueChar);
    auto result6 = g_mapFilterFunc(validNameChar, NULL);
    auto result7 = g_mapFilterFunc(NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result1);
    ASSERT_ARE_NOT_EQUAL(int, 0, result2);
    ASSERT_ARE_NOT_EQUAL(int, 0, result3);
    ASSERT_ARE_NOT_EQUAL(int, 0, result4);
    ASSERT_ARE_EQUAL(int, 0, result5);
    ASSERT_ARE_EQUAL(int, 0, result6);
    ASSERT_ARE_EQUAL(int, 0, result7);

    mocks.AssertActualAndExpectedCalls();

    ///cleanup
    EventData_Destroy(eventDataHandle);
}

END_TEST_SUITE(eventdata_unittests)
