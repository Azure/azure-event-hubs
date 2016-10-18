// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "eventhubclient.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/xlogging.h"
#include "version.h"
#include "azure_c_shared_utility/lock.h"
#include "eventhubclient_ll.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/condition.h"

#define GBALLOC_H

DEFINE_ENUM_STRINGS(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_RESULT_VALUES)
DEFINE_MICROMOCK_ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_RESULT_VALUES);

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
#define Condition_Post(x) (COND_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
#define Condition_Wait(x,y) (COND_OK + gballocState - gballocState)
#define Condition_Init() (COND_HANDLE)0x42
#define Condition_Deinit(x) (COND_OK + gballocState - gballocState)
#include "gballoc.c"
#undef Lock
#undef Unlock
#undef Lock_Init
#undef Lock_Deinit
#undef Condition_Post
#undef Condition_Wait
#undef Condition_Init
#undef Condition_Deinit
};

static MICROMOCK_MUTEX_HANDLE g_testByTest;

#define TEST_EVENTDATA_HANDLE (EVENTDATA_HANDLE)0x45
#define TEST_ENCODED_STRING_HANDLE (STRING_HANDLE)0x47
#define TEST_EVENTCLIENT_LL_HANDLE (EVENTHUBCLIENT_LL_HANDLE)0x49

static bool g_confirmationCall = true;
static EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK g_ConfirmationCallback;
static void* g_userContextCallback;
static EVENTHUBCLIENT_CONFIRMATION_RESULT g_callbackConfirmationResult;

static const char* CONNECTION_STRING = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";
static const char* EVENTHUB_PATH = "eventHubName";

static const char* TEXT_MESSAGE = "Hello From EventHubClient Unit Tests";
static const char* TEST_CHAR = "TestChar";
static const char* PARTITION_KEY_VALUE = "PartitionKeyValue";
static const char* PARTITION_KEY_EMPTY_VALUE = "";
static const char* PROPERTY_NAME = "PropertyName";
const unsigned char PROPERTY_BUFF_VALUE[] = { 0x7F, 0x40, 0x3F, 0x6D, 0x6A, 0x20, 0x05, 0x60 };
static const int BUFFER_SIZE = 8;
static bool g_bSetProperty = false;
static bool g_lockInitFail = false;
static bool g_condInitFail = false;

#define EVENT_HANDLE_COUNT  3

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

static size_t currentSTRING_construct_call;
static size_t whenShallSTRING_construct_fail;

static size_t g_currentlock_call;
static size_t g_whenShalllock_fail;

static size_t g_currentcond_call;
static size_t g_whenShallcond_fail;

static size_t g_whenThreadApi_Fail;
static size_t g_currentThreadApi_Fail;

static void EventHubSendAsycConfirmCallback(EVENTHUBCLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)result;
    bool* callbackNotified = (bool*)userContextCallback;
    *callbackNotified = true;
}

typedef struct LOCK_TEST_STRUCT_TAG
{
    char* dummy;
} LOCK_TEST_STRUCT;

typedef struct COND_TEST_STRUCT_TAG
{
    char* dummy;
} COND_TEST_STRUCT;

// ** Mocks **
TYPED_MOCK_CLASS(CEventHubClientMocks, CGlobalMock)
{
public:
    MOCK_STATIC_METHOD_3(, THREADAPI_RESULT, ThreadAPI_Create, THREAD_HANDLE*, threadHandle, THREAD_START_FUNC, func, void*, arg)
        THREADAPI_RESULT threadResult;
    g_currentThreadApi_Fail++;
    if (g_whenThreadApi_Fail > 0 && g_currentThreadApi_Fail == g_whenThreadApi_Fail)
    {
        *threadHandle = NULL;
        threadResult = THREADAPI_ERROR;
    }
    else
    {
        *threadHandle = malloc(1);
        threadResult = THREADAPI_OK;
    }
    MOCK_METHOD_END(THREADAPI_RESULT, threadResult)


        MOCK_STATIC_METHOD_2(, THREADAPI_RESULT, ThreadAPI_Join, THREAD_HANDLE, threadHandle, int*, res)
        free(threadHandle);
    MOCK_METHOD_END(THREADAPI_RESULT, THREADAPI_OK)

        MOCK_STATIC_METHOD_2(, EVENTHUBCLIENT_LL_HANDLE, EventHubClient_LL_CreateFromConnectionString, const char*, connectionString, const char*, eventHubPath)
        EVENTHUBCLIENT_LL_HANDLE resultHandle;
    if (connectionString == NULL || eventHubPath == NULL)
    {
        resultHandle = NULL;
    }
    else
    {
        resultHandle = TEST_EVENTCLIENT_LL_HANDLE;
    }
    MOCK_METHOD_END(EVENTHUBCLIENT_LL_HANDLE, resultHandle)

        MOCK_STATIC_METHOD_5(, EVENTHUBCLIENT_RESULT, EventHubClient_LL_SendBatchAsync, EVENTHUBCLIENT_LL_HANDLE, eventHubClientLLHandle, EVENTDATA_HANDLE*, eventDataList, size_t, count, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK, telemetryConfirmationCallback, void*, userContextCallback)
        if (g_confirmationCall)
        {
            g_ConfirmationCallback = telemetryConfirmationCallback;
            g_userContextCallback = userContextCallback;
            g_ConfirmationCallback(g_callbackConfirmationResult, g_userContextCallback);
        }
    MOCK_METHOD_END(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK)

        MOCK_STATIC_METHOD_4(, EVENTHUBCLIENT_RESULT, EventHubClient_LL_SendAsync, EVENTHUBCLIENT_LL_HANDLE, eventHubClientLLHandle, EVENTDATA_HANDLE, eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK, telemetryConfirmationCallback, void*, userContextCallback)
        if (g_confirmationCall)
        {
            g_ConfirmationCallback = telemetryConfirmationCallback;
            g_userContextCallback = userContextCallback;
            g_ConfirmationCallback(g_callbackConfirmationResult, g_userContextCallback);
        }
    MOCK_METHOD_END(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK)

        MOCK_STATIC_METHOD_1(, void, EventHubClient_LL_DoWork, EVENTHUBCLIENT_LL_HANDLE, eventHubClientLLHandle)
        if (g_ConfirmationCallback)
        {
            g_ConfirmationCallback(g_callbackConfirmationResult, g_userContextCallback);
        }
    MOCK_VOID_METHOD_END()

        MOCK_STATIC_METHOD_1(, void, EventHubClient_LL_Destroy, EVENTHUBCLIENT_LL_HANDLE, eventHubClientLLHandle)
        MOCK_VOID_METHOD_END()

        /* EventData Mocks */
        MOCK_STATIC_METHOD_2(, EVENTDATA_HANDLE, EventData_CreateWithNewMemory, const unsigned char*, data, size_t, length)
        MOCK_METHOD_END(EVENTDATA_HANDLE, (EVENTDATA_HANDLE)malloc(1))

        MOCK_STATIC_METHOD_3(, EVENTDATA_RESULT, EventData_GetData, EVENTDATA_HANDLE, eventDataHandle, const unsigned char**, data, size_t*, dataLength)
    {
        *data = (unsigned char*)TEXT_MESSAGE;
        *dataLength = strlen((const char*)TEXT_MESSAGE);
    }
    MOCK_METHOD_END(EVENTDATA_RESULT, EVENTDATA_OK)

        MOCK_STATIC_METHOD_1(, void, EventData_Destroy, EVENTDATA_HANDLE, eventDataHandle)
        free(eventDataHandle);
    MOCK_VOID_METHOD_END()

        MOCK_STATIC_METHOD_1(, const char*, EventData_GetPartitionKey, EVENTDATA_HANDLE, eventDataHandle)
        MOCK_METHOD_END(const char*, NULL)
        MOCK_STATIC_METHOD_5(, EVENTDATA_RESULT, EventData_GetPropertyByIndex, EVENTDATA_HANDLE, eventDataHandle, size_t, propertyIndex, const char**, propertyName, const unsigned char**, propertyValue, size_t*, propertySize)
        EVENTDATA_RESULT eventdataResult = EVENTDATA_MISSING_PROPERTY_NAME;
    if (g_bSetProperty)
    {
        *propertyName = PROPERTY_NAME;
        *propertyValue = PROPERTY_BUFF_VALUE;
        *propertySize = BUFFER_SIZE;
        eventdataResult = EVENTDATA_OK;
    }
    MOCK_METHOD_END(EVENTDATA_RESULT, eventdataResult)
        MOCK_STATIC_METHOD_1(, size_t, EventData_GetPropertyCount, EVENTDATA_HANDLE, eventDataHandle)
        MOCK_METHOD_END(size_t, 0)

        /* Version Mocks */
        MOCK_STATIC_METHOD_0(, const char*, EventHubClient_GetVersionString)
        MOCK_METHOD_END(const char*, nullptr);

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
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_0(, LOCK_HANDLE, Lock_Init)
        LOCK_HANDLE handle;
    if (g_lockInitFail)
    {
        handle = NULL;
    }
    else
    {
        LOCK_TEST_STRUCT* lockTest = (LOCK_TEST_STRUCT*)malloc(sizeof(LOCK_TEST_STRUCT));
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


    MOCK_STATIC_METHOD_0(, COND_HANDLE, Condition_Init)
        COND_HANDLE chandle;
    if (g_condInitFail)
    {
        chandle = NULL;
    }
    else
    {
        COND_TEST_STRUCT* condTest = (COND_TEST_STRUCT*)malloc(sizeof(COND_TEST_STRUCT));
        chandle = condTest;
    }
    MOCK_METHOD_END(COND_HANDLE, chandle);

    MOCK_STATIC_METHOD_1(, COND_RESULT, Condition_Post, COND_HANDLE, handle)
        COND_RESULT condResult;

    g_currentcond_call++;
    if (g_whenShallcond_fail > 0)
    {
        if (g_currentcond_call == g_whenShallcond_fail)
        {
            condResult = COND_ERROR;
        }
        else
        {
            condResult = COND_OK;
        }
    }
    else
    {
        condResult = COND_OK;
    }
    if (condResult == COND_OK && handle != NULL)
    {
        COND_TEST_STRUCT* condTest = (COND_TEST_STRUCT*)handle;
        condTest->dummy = (char*)malloc(1);
    }
    MOCK_METHOD_END(COND_RESULT, condResult);

    MOCK_STATIC_METHOD_3(, COND_RESULT, Condition_Wait, COND_HANDLE, handle, LOCK_HANDLE, lock, int, timeout)
        if (handle != NULL)
        {
            COND_TEST_STRUCT* condTest = (COND_TEST_STRUCT*)handle;
            free(condTest->dummy);
        }
    MOCK_METHOD_END(COND_RESULT, COND_OK);

    MOCK_STATIC_METHOD_1(, void, Condition_Deinit, COND_HANDLE, handle)
        free(handle);
    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientMocks, , EVENTHUBCLIENT_LL_HANDLE, EventHubClient_LL_CreateFromConnectionString, const char*, connectionString, const char*, eventHubPath);
DECLARE_GLOBAL_MOCK_METHOD_4(CEventHubClientMocks, , EVENTHUBCLIENT_RESULT, EventHubClient_LL_SendAsync, EVENTHUBCLIENT_LL_HANDLE, eventHubClientLLHandle, EVENTDATA_HANDLE, eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK, telemetryConfirmationCallback, void*, userContextCallback);
DECLARE_GLOBAL_MOCK_METHOD_5(CEventHubClientMocks, , EVENTHUBCLIENT_RESULT, EventHubClient_LL_SendBatchAsync, EVENTHUBCLIENT_LL_HANDLE, eventHubClientLLHandle, EVENTDATA_HANDLE*, eventDataList, size_t, count, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK, telemetryConfirmationCallback, void*, userContextCallback);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , void, EventHubClient_LL_DoWork, EVENTHUBCLIENT_LL_HANDLE, eventHubClientLLHandle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , void, EventHubClient_LL_Destroy, EVENTHUBCLIENT_LL_HANDLE, eventHubClientLLHandle);

DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientMocks, , THREADAPI_RESULT, ThreadAPI_Create, THREAD_HANDLE*, threadHandle, THREAD_START_FUNC, func, void*, arg);

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientMocks, , THREADAPI_RESULT, ThreadAPI_Join, THREAD_HANDLE, threadHandle, int*, res);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , void, gballoc_free, void*, ptr)
DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientMocks, , LOCK_HANDLE, Lock_Init);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , LOCK_RESULT, Lock, LOCK_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , LOCK_RESULT, Unlock, LOCK_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle)
DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientMocks, , COND_HANDLE, Condition_Init);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , COND_RESULT, Condition_Post, COND_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientMocks, , COND_RESULT, Condition_Wait, COND_HANDLE, handle, LOCK_HANDLE, lock, int, timeout);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , void, Condition_Deinit, COND_HANDLE, handle)

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientMocks, , EVENTDATA_HANDLE, EventData_CreateWithNewMemory, const unsigned char*, data, size_t, length);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientMocks, , EVENTDATA_RESULT, EventData_GetData, EVENTDATA_HANDLE, eventDataHandle, const unsigned char**, data, size_t*, dataLength);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , void, EventData_Destroy, EVENTDATA_HANDLE, eventDataHandle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , const char*, EventData_GetPartitionKey, EVENTDATA_HANDLE, eventDataHandle);
DECLARE_GLOBAL_MOCK_METHOD_5(CEventHubClientMocks, , EVENTDATA_RESULT, EventData_GetPropertyByIndex, EVENTDATA_HANDLE, eventDataHandle, size_t, propertyIndex, const char**, propertyName, const unsigned char**, propertyValue, size_t*, propertySize);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientMocks, , size_t, EventData_GetPropertyCount, EVENTDATA_HANDLE, eventDataHandle);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientMocks, , const char*, EventHubClient_GetVersionString);

// ** End of Mocks **
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(eventhubclient_unittests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = MicroMockCreateMutex();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    MicroMockDestroyMutex(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (!MicroMockAcquireMutex(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    currentmalloc_call = 0;
    whenShallmalloc_fail = 0;
    g_currentlock_call = 0;
    g_whenShalllock_fail = 0;

    g_bSetProperty = false;
    g_confirmationCall = true;
    g_ConfirmationCallback = NULL;
    g_userContextCallback = NULL;
    g_callbackConfirmationResult = EVENTHUBCLIENT_CONFIRMATION_OK;
    g_lockInitFail = false;
    g_whenThreadApi_Fail = 0;
    g_currentThreadApi_Fail = 0;
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    if (!MicroMockReleaseMutex(g_testByTest))
    {
        ASSERT_FAIL("failure in test framework at ReleaseMutex");
    }
}

/*** EventHubClient_CreateFromConnectionString ***/
/* Tests_SRS_EVENTHUBCLIENT_03_004: [EventHubClient_ CreateFromConnectionString shall pass the connectionString and eventHubPath variables to EventHubClient_CreateFromConnectionString_LL.] */
/* Tests_SRS_EVENTHUBCLIENT_03_006: [EventHubClient_ CreateFromConnectionString shall return a NULL value if EventHubClient_CreateFromConnectionString_LL  returns NULL.] */
TEST_FUNCTION(EventHubClient_CreateFromConnectionString_Lower_Layer_Fails)
{
    // arrange
    CEventHubClientMocks ehMocks;
    STRICT_EXPECTED_CALL(ehMocks, EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH))
        .SetReturn((EVENTHUBCLIENT_LL_HANDLE)NULL);

    // act
    EVENTHUBCLIENT_HANDLE result = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.AssertActualAndExpectedCalls();

    // assert
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_EVENTHUBCLIENT_03_002: [Upon Success of EventHubClient_CreateFromConnectionString_LL,  EventHubClient_CreateFromConnectionString shall allocate the internal structures required by this module.] */
TEST_FUNCTION(EventHubClient_CreateFromConnectionString_Succeeds)
{
    // arrange
    CEventHubClientMocks ehMocks;
    ehMocks.ResetAllCalls();
    STRICT_EXPECTED_CALL(ehMocks, EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH));
    STRICT_EXPECTED_CALL(ehMocks, Lock_Init());
    EXPECTED_CALL(ehMocks, gballoc_malloc(IGNORED_NUM_ARG));

    // act
    EVENTHUBCLIENT_HANDLE result = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(result);
}

TEST_FUNCTION(EventHubClient_CreateFromConnectionString_Lock_Init_Fails)
{
    // arrange
    CEventHubClientMocks ehMocks;
    STRICT_EXPECTED_CALL(ehMocks, EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH));
    STRICT_EXPECTED_CALL(ehMocks, Lock_Init());
    EXPECTED_CALL(ehMocks, gballoc_malloc(0));
    EXPECTED_CALL(ehMocks, gballoc_free(0));
    STRICT_EXPECTED_CALL(ehMocks, EventHubClient_LL_Destroy(TEST_EVENTCLIENT_LL_HANDLE));
    g_lockInitFail = true;

    // act
    EVENTHUBCLIENT_HANDLE result = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.AssertActualAndExpectedCalls();

    // assert
    ASSERT_IS_NULL(result);

    // cleanup
    EventHubClient_Destroy(result);
}

/*** EventHubClient_Destroy ***/
/* Tests_SRS_EVENTHUBCLIENT_03_018: [If the eventHubHandle is NULL, EventHubClient_Destroy shall not do anything.] */
TEST_FUNCTION(EventHubClient_Destroy_with_NULL_eventHubHandle_Does_Nothing)
{
    // arrange
    CEventHubClientMocks ehMocks;

    // act
    EventHubClient_Destroy(NULL);

    // assert
    // Implicit
}

/*** EventHubClient_Send ***/
/* Tests_SRS_EVENTHUBCLIENT_03_007: [EventHubClient_Send shall return EVENTHUBCLIENT_INVALID_ARG if either eventHubHandle or eventDataHandle is NULL.] */
TEST_FUNCTION(EventHubClient_Send_with_NULL_eventHubHandle_Fails)
{
    // arrange
    CEventHubClientMocks ehMocks;
    unsigned char testData[] = { 0x42, 0x43, 0x44 };
    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(testData, sizeof(testData) / sizeof(testData[0]));
    ehMocks.ResetAllCalls();

    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_Send(NULL, eventDataHandle);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventData_Destroy(eventDataHandle);
}

/* Tests_SRS_EVENTHUBCLIENT_03_007: [EventHubClient_Send shall return EVENTHUBCLIENT_INVALID_ARG if either eventHubHandle or eventDataHandle is NULL.] */
TEST_FUNCTION(EventHubClient_Send_with_NULL_eventDataHandle_Fails)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString("j", "b");
    ehMocks.ResetAllCalls();

    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_Send(eventHubHandle, NULL);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Tests_SRS_EVENTHUBCLIENT_03_008: [EventHubClient_Send shall call into the Execute_LowerLayerSendAsync function to send the eventDataHandle parameter to the EventHub.] */
/* Tests_SRS_EVENTHUBCLIENT_03_013: [EventHubClient_Send shall return EVENTHUBCLIENT_OK upon successful completion of the Execute_LowerLayerSendAsync and the callback function.] */
/* Tests_SRS_EVENTHUBCLIENT_03_010: [Upon success of Execute_LowerLayerSendAsync, then EventHubClient_Send wait until the EVENTHUB_CALLBACK_STRUCT callbackStatus variable is set to CALLBACK_NOTIFIED.] */
TEST_FUNCTION(EventHubClient_Send_Succeeds)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendAsync(IGNORED_PTR_ARG, TEST_EVENTDATA_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, gballoc_malloc(0));
    EXPECTED_CALL(ehMocks, gballoc_free(0));
    EXPECTED_CALL(ehMocks, Condition_Init());
    EXPECTED_CALL(ehMocks, Condition_Post(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Condition_Wait(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(ehMocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock_Init());
    EXPECTED_CALL(ehMocks, Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));

    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_Send(eventHubHandle, TEST_EVENTDATA_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Tests_SRS_EVENTHUBCLIENT_07_012: [EventHubClient_Send shall return EVENTHUBCLIENT_ERROR if the EVENTHUB_CALLBACK_STRUCT confirmationResult variable does not equal EVENTHUBCLIENT_CONFIMRATION_OK.] */
TEST_FUNCTION(EventHubClient_Send_ConfirmationResult_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendAsync(IGNORED_PTR_ARG, TEST_EVENTDATA_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, gballoc_malloc(0));
    EXPECTED_CALL(ehMocks, gballoc_free(0));
    EXPECTED_CALL(ehMocks, Condition_Init());
    EXPECTED_CALL(ehMocks, Condition_Post(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Condition_Wait(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(ehMocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock_Init());
    EXPECTED_CALL(ehMocks, Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));


    g_callbackConfirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_Send(eventHubHandle, TEST_EVENTDATA_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Tests_SRS_EVENTHUBCLIENT_03_009: [EventHubClient_Send shall return EVENTHUBCLIENT_ERROR on any failure that is encountered.] */
TEST_FUNCTION(EventHubClient_Send_EventhubClient_LL_SendAsync_Fails)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendAsync(IGNORED_PTR_ARG, TEST_EVENTDATA_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(EVENTHUBCLIENT_ERROR);
    EXPECTED_CALL(ehMocks, gballoc_malloc(0));
    EXPECTED_CALL(ehMocks, gballoc_free(0));
    EXPECTED_CALL(ehMocks, Condition_Init());
    //EXPECTED_CALL(ehMocks, Condition_Post(IGNORED_PTR_ARG));
    //EXPECTED_CALL(ehMocks, Condition_Wait(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(ehMocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock_Init());
    EXPECTED_CALL(ehMocks, Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));

    g_confirmationCall = false;

    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_Send(eventHubHandle, TEST_EVENTDATA_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/*** EventHubClient_SendAsync ***/
/* Tests_SRS_EVENTHUBCLIENT_07_037: [On Success EventHubClient_SendAsync shall return EVENTHUBCLIENT_OK.] */
/* Tests_SRS_EVENTHUBCLIENT_07_038: [Execute_LowerLayerSendAsync shall call EventHubClient_LL_SendAsync to send data to the Eventhub Endpoint.] */
/* Tests_SRS_EVENTHUBCLIENT_07_028: [If Execute_LowerLayerSendAsync is successful then it shall return 0.] */
/* Tests_SRS_EVENTHUBCLIENT_07_034: [Create_DoWorkThreadIfNeccesary shall use the ThreadAPI_Create API to create a thread and execute EventhubClientThread function.] */
/* Tests_SRS_EVENTHUBCLIENT_07_029: [Execute_LowerLayerSendAsync shall Lock on the EVENTHUBCLIENT_STRUCT lockInfo to protect calls to Lower Layer and Thread function calls.] */
/* Tests_SRS_EVENTHUBCLIENT_07_031: [Execute_LowerLayerSendAsync shall call into the Create_DoWorkThreadIfNeccesary function to create the DoWork thread.] */
TEST_FUNCTION(EventHubClient_SendAsync_Succeeds)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendAsync(IGNORED_PTR_ARG, TEST_EVENTDATA_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    bool callbackNotified = false;
    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Tests_SRS_EVENTHUBCLIENT_03_021: [EventHubClient_SendAsync shall return EVENTHUBCLIENT_INVALID_ARG if either eventHubHandle or eventDataHandle is NULL.] */
TEST_FUNCTION(EventHubClient_SendAsync_EventHubHandle_NULL_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    ehMocks.ResetAllCalls();

    bool callbackNotified = false;
    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendAsync(NULL, TEST_EVENTDATA_HANDLE, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
}

/* Tests_SRS_EVENTHUBCLIENT_03_021: [EventHubClient_SendAsync shall return EVENTHUBCLIENT_INVALID_ARG if either eventHubHandle or eventDataHandle is NULL.] */
TEST_FUNCTION(EventHubClient_SendAsync_EVENTDATA_HANDLE_NULL_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    bool callbackNotified = false;
    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendAsync(eventHubHandle, NULL, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Tests_SRS_EVENTHUBCLIENT_07_039: [If the EventHubClient_LL_SendAsync call fails then Execute_LowerLayerSendAsync shall return a nonzero value.] */
TEST_FUNCTION(EventHubClient_SendAsync_EventHubClient_LL_SendAsync_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendAsync(IGNORED_PTR_ARG, TEST_EVENTDATA_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(EVENTHUBCLIENT_ERROR);

    bool callbackNotified = false;
    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Tests_SRS_EVENTHUBCLIENT_07_035: [Create_DoWorkThreadIfNeccesary shall return a nonzero value if any failure is encountered.] */
/* Tests_SRS_EVENTHUBCLIENT_07_032: [If Create_DoWorkThreadIfNeccesary does not return 0 then Execute_LowerLayerSendAsync shall return a nonzero value.] */
TEST_FUNCTION(EventHubClient_SendAsync_ThreadApi_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(THREADAPI_ERROR);

    bool callbackNotified = false;
    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Tests_SRS_EVENTHUBCLIENT_07_027: [If the threadHandle is not NULL then the CreateThread_And_SendAsync shall return EVENTHUBCLIENT_OK.] */
/* Tests_SRS_EVENTHUBCLIENT_07_033: [Create_DoWorkThreadIfNeccesary shall set return 0 if threadHandle parameter is not a NULL value.] */
TEST_FUNCTION(EventHubClient_SendAsync_2nd_Call_Succeeds)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG)).
        ExpectedTimesExactly(2);
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG)).
        ExpectedTimesExactly(2);
    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendAsync(IGNORED_PTR_ARG, TEST_EVENTDATA_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).
        ExpectedTimesExactly(2);
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    bool callbackNotified = false;
    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, EventHubSendAsycConfirmCallback, &callbackNotified);
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);

    result = EventHubClient_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Tests_SRS_EVENTHUBCLIENT_07_028: [CreateThread_And_SendAsync shall return EVENTHUBCLIENT_ERROR on any error that occurs.] */
/* Tests_SRS_EVENTHUBCLIENT_07_030: [Execute_LowerLayerSendAsync shall return a nonzero value if it is unable to obtain the lock with the Lock function.]*/
TEST_FUNCTION(EventHubClient_SendAsync_Lock_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));

    g_whenShalllock_fail = 1;
    bool callbackNotified = false;
    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/*** EventHubClient_SendBatchAsync ***/
/* Tests_SRS_EVENTHUBCLIENT_07_040: [EventHubClient_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL or count is zero.] */
TEST_FUNCTION(EventHubClient_SendBatchAsync_EventHandle_NULL_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    ehMocks.ResetAllCalls();

    // act
    bool callbackNotified = false;
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatchAsync(NULL, eventhandleList, EVENT_HANDLE_COUNT, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
}

/* Tests_SRS_EVENTHUBCLIENT_07_040: [EventHubClient_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL or count is zero.] */
TEST_FUNCTION(EventHubClient_SendBatchAsync_EventHandleList_NULL_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    // act
    bool callbackNotified = false;
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatchAsync(eventHubHandle, NULL, EVENT_HANDLE_COUNT, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Tests_SRS_EVENTHUBCLIENT_07_040: [EventHubClient_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL or count is zero.] */
TEST_FUNCTION(EventHubClient_SendBatchAsync_EventHandle_Count_Zero_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    // act
    bool callbackNotified = false;
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatchAsync(eventHubHandle, eventhandleList, 0, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Test_SRS_EVENTHUBCLIENT_07_042: [On Success EventHubClient_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
/* Test_SRS_EVENTHUBCLIENT_07_043: [Execute_LowerLayerSendBatchAsync shall Lock on the EVENTHUBCLIENT_STRUCT lockInfo to protect calls to Lower Layer and Thread function calls.] */
/* Test_SRS_EVENTHUBCLIENT_07_045: [Execute_LowerLayerSendAsync shall call into the Create_DoWorkThreadIfNeccesary function to create the DoWork thread.] */
/* Test_SRS_EVENTHUBCLIENT_07_047: [If Execute_LowerLayerSendAsync is successful then it shall return 0.] */
TEST_FUNCTION(EventHubClient_SendBatchAsync_Succeed)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendBatchAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    bool callbackNotified = false;
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Test_SRS_EVENTHUBCLIENT_07_043: [Execute_LowerLayerSendBatchAsync shall Lock on the EVENTHUBCLIENT_STRUCT lockInfo to protect calls to Lower Layer and Thread function calls.] */
TEST_FUNCTION(EventHubClient_SendBatchAsync_Lock_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));

    // act
    g_whenShalllock_fail = 1;
    bool callbackNotified = false;
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Test_SRS_EVENTHUBCLIENT_07_046: [If Create_DoWorkThreadIfNeccesary does not return 0 then Execute_LowerLayerSendAsync shall return a nonzero value.] */
TEST_FUNCTION(EventHubClient_SendBatchAsync_ThreadApi_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    bool callbackNotified = false;
    g_whenThreadApi_Fail = 1;
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

TEST_FUNCTION(EventHubClient_SendBatchAsync_LL_SendBatchAsync_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendBatchAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(EVENTHUBCLIENT_ERROR);
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    bool callbackNotified = false;
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, EventHubSendAsycConfirmCallback, &callbackNotified);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/*** EventHubClient_SendBatch ***/
/* Test_SRS_EVENTHUBCLIENT_07_050: [EventHubClient_SendBatch shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL.] */
TEST_FUNCTION(EventHubClient_SendBatch_EVENTHUBCLIENT_NULLL_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    ehMocks.ResetAllCalls();


    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatch(NULL, eventhandleList, EVENT_HANDLE_COUNT);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
}

/* Test_SRS_EVENTHUBCLIENT_07_050: [EventHubClient_SendBatch shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL.] */
TEST_FUNCTION(EventHubClient_SendBatch_EVENTHANDLELIST_NULL_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatch(eventHubHandle, NULL, EVENT_HANDLE_COUNT);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Test_SRS_EVENTHUBCLIENT_07_050: [EventHubClient_SendBatch shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL.] */
TEST_FUNCTION(EventHubClient_SendBatch_Handle_count_zero_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatch(eventHubHandle, eventhandleList, 0);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Test_SRS_EVENTHUBCLIENT_07_051: [EventHubClient_SendBatch shall call into the Execute_LowerLayerSendBatchAsync function to send the eventDataHandle parameter to the EventHub.] */
/* Test_SRS_EVENTHUBCLIENT_07_053: [Upon success of Execute_LowerLayerSendBatchAsync, then EventHubClient_SendBatch shall wait until the EVENTHUB_CALLBACK_STRUCT callbackStatus variable is set to CALLBACK_NOTIFIED.] */
/* Test_SRS_EVENTHUBCLIENT_07_054: [EventHubClient_SendBatch shall return EVENTHUBCLIENT_OK upon successful completion of the Execute_LowerLayerSendBatchAsync and the callback function.] */
TEST_FUNCTION(EventHubClient_SendBatch_Succeed)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendBatchAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, gballoc_malloc(IGNORE));
    EXPECTED_CALL(ehMocks, gballoc_free(IGNORE));
    EXPECTED_CALL(ehMocks, Condition_Init());
    EXPECTED_CALL(ehMocks, Condition_Post(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Condition_Wait(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(ehMocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock_Init());
    EXPECTED_CALL(ehMocks, Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));

    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatch(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Test_SRS_EVENTHUBCLIENT_07_052: [EventHubClient_SendBatch shall return EVENTHUBCLIENT_ERROR on any failure that is encountered.] */
TEST_FUNCTION(EventHubClient_SendBatch_LowerLayerSendBatch_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendBatchAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(EVENTHUBCLIENT_ERROR);
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, gballoc_malloc(IGNORE));
    EXPECTED_CALL(ehMocks, gballoc_free(IGNORE));
    EXPECTED_CALL(ehMocks, Condition_Init());
    //EXPECTED_CALL(ehMocks, Condition_Post(IGNORED_PTR_ARG));
    //EXPECTED_CALL(ehMocks, Condition_Wait(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(ehMocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock_Init());
    EXPECTED_CALL(ehMocks, Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));

    g_confirmationCall = false;

    // act
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatch(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

/* Test_SRS_EVENTHUBCLIENT_07_052: [EventHubClient_SendBatch shall return EVENTHUBCLIENT_ERROR on any failure that is encountered.] */
TEST_FUNCTION(EventHubClient_SendBatch_Confirmation_Result_Fail)
{
    // arrange
    CEventHubClientMocks ehMocks;
    EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

    eventhandleList[0] = (EVENTDATA_HANDLE)1;
    eventhandleList[1] = (EVENTDATA_HANDLE)2;
    eventhandleList[2] = (EVENTDATA_HANDLE)4;

    EVENTHUBCLIENT_HANDLE eventHubHandle = EventHubClient_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
    ehMocks.ResetAllCalls();

    EXPECTED_CALL(ehMocks, EventHubClient_LL_SendBatchAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORE, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, gballoc_malloc(IGNORE));
    EXPECTED_CALL(ehMocks, gballoc_free(IGNORE));
    EXPECTED_CALL(ehMocks, Condition_Init());
    EXPECTED_CALL(ehMocks, Condition_Post(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Condition_Wait(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(ehMocks, Condition_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock_Init());
    EXPECTED_CALL(ehMocks, Lock_Deinit(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Lock(IGNORED_PTR_ARG));
    EXPECTED_CALL(ehMocks, Unlock(IGNORED_PTR_ARG));

    // act
    g_callbackConfirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;
    EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatch(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT);

    // assert
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
    ehMocks.AssertActualAndExpectedCalls();

    // cleanup
    EventHubClient_Destroy(eventHubHandle);
}

END_TEST_SUITE(eventhubclient_unittests)
