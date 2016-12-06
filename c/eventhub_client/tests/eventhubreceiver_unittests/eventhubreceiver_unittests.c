// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <signal.h>
#include <stddef.h>
#include <stdio.h>

static void* TestHook_malloc(size_t size)
{
    return malloc(size);
}

static void TestHook_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/gballoc.h"

#include "eventhubreceiver_ll.h"

#undef  ENABLE_MOCKS

// interface under test
#include "eventhubreceiver.h"


//#################################################################################################
// EventHubReceiver Test Defines and Data types
//#################################################################################################
#define TEST_LOCK_VALID                         (void*)0x40
#define TEST_EVENTHUB_RECEIVER_VALID            (EVENTHUBRECEIVER_HANDLE)0x42
#define TEST_EVENTHUB_RECEIVER_LL_VALID         (EVENTHUBRECEIVER_LL_HANDLE)0x43
#define TEST_THREAD_HANDLE_VALID                (THREAD_HANDLE)0x44
#define TEST_EVENTDATA_HANDLE_VALID             (EVENTDATA_HANDLE)0x45
#define TEST_EVENTHUB_RECEIVER_TIMEOUT          1000
#define TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP    (uint64_t)10000

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

typedef struct TEST_EVENTHUB_RECEIVER_ASYNC_CALLBACK_TAG
{
    EVENTHUBRECEIVER_HANDLE eventHubRxHandle;
    EVENTHUBRECEIVER_ASYNC_CALLBACK rxCallback;
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK rxErrorCallback;
    EVENTHUBRECEIVER_ASYNC_END_CALLBACK rxEndCallback;
    void* rxCallbackCtxt;
    void* rxErrorCallbackCtxt;
    void* rxEndCallbackCtxt; 
    int rxCallbackCalled;
    int rxErrorCallbackCalled;
    int rxEndCallbackCalled;
    int forceEndAsyncToFail;
    EVENTHUBRECEIVER_RESULT rxCallbackResult;
    EVENTHUBRECEIVER_RESULT rxErrorCallbackResult;
    EVENTHUBRECEIVER_RESULT rxEndCallbackResult;
    int lockDeinitInvoked;
} TEST_EVENTHUB_RECEIVER_ASYNC_CALLBACK;

//#################################################################################################
// EventHubReceiver LL Test Data
//#################################################################################################
static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

static const char* CONNECTION_STRING = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=ICT5KKSJR/DW7OZVEQ7OPSSXU5TRMR6AIWLGI5ZIT/8=";
static const char* EVENTHUB_PATH     = "eventHubName";
static const char* CONSUMER_GROUP    = "ConsumerGroup";
static const char* PARTITION_ID      = "0";

static TEST_EVENTHUB_RECEIVER_ASYNC_CALLBACK OnRxCBStruct;
static void* OnRxCBCtxt    = (void*)&OnRxCBStruct;
static void* OnRxEndCBCtxt = (void*)&OnRxCBStruct;
static void* OnErrCBCtxt   = (void*)&OnRxCBStruct;

static THREAD_START_FUNC threadEntry;
static void* threadEntryCtxt;

#ifdef __cplusplus
    extern "C" const size_t THREAD_STATE_OFFSET;
    extern "C" const sig_atomic_t THREAD_STATE_EXIT;
#else
    extern const size_t THREAD_STATE_OFFSET;
    extern const sig_atomic_t THREAD_STATE_EXIT;
#endif

//#################################################################################################
// EventHubReceiver Test Hook Implementations
//#################################################################################################    
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

void TestHook_DList_InitializeListHead(PDLIST_ENTRY ListHead)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

PDLIST_ENTRY TestHook_DList_RemoveHeadList(PDLIST_ENTRY ListHead)
{
    PDLIST_ENTRY Flink;
    PDLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}

void TestHook_DList_InsertTailList(PDLIST_ENTRY ListHead, PDLIST_ENTRY Entry)
{
    PDLIST_ENTRY Blink;
    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
    return;
}

static EVENTHUBRECEIVER_RESULT TestHook_EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync
(
    EVENTHUBRECEIVER_LL_HANDLE eventHubReceiverLLHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    uint64_t startTimestampInSec,
    unsigned int waitTimeoutInMs
)
{
    (void)eventHubReceiverLLHandle;
    (void)startTimestampInSec;
    (void)waitTimeoutInMs;
    OnRxCBStruct.rxCallback = onEventReceiveCallback;
    OnRxCBStruct.rxCallbackCalled = 0;
    OnRxCBStruct.rxCallbackCtxt = onEventReceiveUserContext;
    OnRxCBStruct.rxErrorCallback = onEventReceiveErrorCallback;
    OnRxCBStruct.rxErrorCallbackCalled = 0;
    OnRxCBStruct.rxErrorCallbackCtxt = onEventReceiveErrorUserContext;

    return EVENTHUBRECEIVER_OK;
}

static void TestHook_ThreadAPI_Sleep(unsigned int ms)
{
    (void)ms;
    uintptr_t threadStateAddress = (uintptr_t)OnRxCBStruct.eventHubRxHandle;
    threadStateAddress += THREAD_STATE_OFFSET;
    *((sig_atomic_t*)threadStateAddress) = THREAD_STATE_EXIT;
}

static THREADAPI_RESULT TestHook_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    *threadHandle = TEST_THREAD_HANDLE_VALID;
    threadEntry = func;
    threadEntryCtxt = arg;
    return THREADAPI_OK;
}

LOCK_RESULT TestHook_Lock_Deinit(LOCK_HANDLE handle)
{
    (void)handle;
    OnRxCBStruct.lockDeinitInvoked = 1;
    return LOCK_OK;
}

static EVENTHUBRECEIVER_RESULT TestHook_EventHubReceiver_LL_ReceiveEndAsync
(
    EVENTHUBRECEIVER_LL_HANDLE eventHubReceiverLLHandle,
    EVENTHUBRECEIVER_ASYNC_END_CALLBACK onEventReceiveEndCallback,
    void* onEventReceiveEndUserContext
)
{
    (void)eventHubReceiverLLHandle;
    OnRxCBStruct.rxEndCallback = onEventReceiveEndCallback;
    OnRxCBStruct.rxEndCallbackCtxt = onEventReceiveEndUserContext;

    return EVENTHUBRECEIVER_OK;
}

//#################################################################################################
// EventHubReceiver Callback Implementations
//#################################################################################################
static void EventHubHReceiver_OnRxCB(EVENTHUBRECEIVER_RESULT result, EVENTDATA_HANDLE eventDataHandle, void* userContext)
{
    (void)eventDataHandle;
    (void)userContext;
    OnRxCBStruct.rxCallbackCalled = 1;
    OnRxCBStruct.rxCallbackResult = result;
}

static void EventHubHReceiver_OnErrCB(EVENTHUBRECEIVER_RESULT errorCode, void* userContext)
{
    (void)userContext;
    OnRxCBStruct.rxErrorCallbackCalled = 1;
    OnRxCBStruct.rxErrorCallbackResult = errorCode;
}

static void EventHubHReceiver_OnRxEndCB(EVENTHUBRECEIVER_RESULT result, void* userContext)
{
    (void)userContext;
    OnRxCBStruct.rxEndCallbackCalled = 1;
    OnRxCBStruct.rxEndCallbackResult = result;
}

//#################################################################################################
// EventHubReceiver Test Helper Implementations
//#################################################################################################
static void ResetTestGlobalData(void)
{
    (void)memset(&OnRxCBStruct, 0, sizeof(TEST_EVENTHUB_RECEIVER_ASYNC_CALLBACK));
    threadEntry = NULL;
    threadEntryCtxt = NULL;
}

//**SRS_EVENTHUBRECEIVER_29_031: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall save off the user supplied callbacks and contexts.**\]**
//**SRS_EVENTHUBRECEIVER_29_033: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall pass `EHR_OnAsyncReceiveCB`, `EHR_OnAsyncReceiveErrorCB` along with eventHubReceiverHandle as corresponding context arguments to `EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` so as to defer execution of these callbacks to the workloop thread.**\]**
//**SRS_EVENTHUBRECEIVER_29_034: \[**`EventHubReceiver_ReceiveFromStartTimestampAsync` shall invoke `EventHubReceiver_LL_ReceiveFromStartTimestampAsync`.**\]**
//**SRS_EVENTHUBRECEIVER_29_035: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` invoke `EventHubReceiver_LL_ReceiveFromStartTimestampAsync` if waitTimeout is zero.**\]**
//**SRS_EVENTHUBRECEIVER_29_036: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` invoke `EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync` if waitTimeout is non zero.**\]**
//**SRS_EVENTHUBRECEIVER_29_038: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall create workloop thread which performs the requirements under "EHR_AsyncWorkLoopThreadEntry"**\]**
//**SRS_EVENTHUBRECEIVER_29_041: \[**Upon Success EVENTHUBRECEIVER_OK shall be returned.**\]**
void TestHelper_SetupReceiveFromStartTimestampAsyncCommon(unsigned int waitTimeoutInMs)
{
    ResetTestGlobalData();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_VALID));

    if (waitTimeoutInMs == 0)
    {
        STRICT_EXPECTED_CALL(EventHubReceiver_LL_ReceiveFromStartTimestampAsync(TEST_EVENTHUB_RECEIVER_LL_VALID,
            EventHubHReceiver_OnRxCB, OnRxCBCtxt,
            EventHubHReceiver_OnErrCB, OnErrCBCtxt, TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP))
            .IgnoreArgument_onEventReceiveCallback()
            .IgnoreArgument_onEventReceiveErrorCallback()
            .IgnoreArgument_onEventReceiveErrorUserContext()
            .IgnoreArgument_onEventReceiveUserContext();
    }
    else
    {
        STRICT_EXPECTED_CALL(EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(TEST_EVENTHUB_RECEIVER_LL_VALID,
            EventHubHReceiver_OnRxCB, OnRxCBCtxt,
            EventHubHReceiver_OnErrCB, OnErrCBCtxt, TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP, TEST_EVENTHUB_RECEIVER_TIMEOUT))
            .IgnoreArgument_onEventReceiveCallback()
            .IgnoreArgument_onEventReceiveErrorCallback()
            .IgnoreArgument_onEventReceiveErrorUserContext()
            .IgnoreArgument_onEventReceiveUserContext();
    }

    EXPECTED_CALL(ThreadAPI_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_VALID));
}

//**SRS_EVENTHUBRECEIVER_29_050: \[**`EventHubReceiver_Destroy` shall call ThreadAPI_Join if required to wait for the workloop thread.**\]**
//**SRS_EVENTHUBRECEIVER_29_051: \[**`EventHubReceiver_Destroy` shall pass the EventHubReceiver_LL_Handle to `EventHubReceiver_Destroy_LL`.**\]**
//**SRS_EVENTHUBRECEIVER_29_052: \[**After call to `EventHubReceiver_Destroy_LL`, free up any allocated structures.**\]**
//**SRS_EVENTHUBRECEIVER_29_053: \[**If any errors are seen, a message will be logged.**\]**
void TestHelper_SetupCommonDestroyCallStack(size_t freeDispatchCallbackCount, bool isEventData, bool isThreadActive)
{
    size_t idx;

    umock_c_reset_all_calls();

    if (isThreadActive)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Join(TEST_THREAD_HANDLE_VALID, IGNORED_PTR_ARG)).IgnoreArgument(2);
    }

    STRICT_EXPECTED_CALL(EventHubReceiver_LL_Destroy(TEST_EVENTHUB_RECEIVER_LL_VALID));
        
    if (!freeDispatchCallbackCount)
    {
        EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    }

    for (idx = 0; idx < freeDispatchCallbackCount; idx++)
    {
        EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));

        if (isEventData) EXPECTED_CALL(EventData_Destroy(IGNORED_PTR_ARG));

        EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    if (freeDispatchCallbackCount)
    {
        EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(Lock_Deinit(TEST_LOCK_VALID));

    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

//**SRS_EVENTHUBRECEIVER_29_080: \[**`EventHubReceiverAsyncThreadEntry` shall call EventHubReceiver_LL_DoWork.**\]**
//**SRS_EVENTHUBRECEIVER_29_081: \[**`EventHubReceiverAsyncThreadEntry` shall invoke any queued user callbacks as a result of `EventHubReceiver_LL_DoWork` calling after the lock has been released.**\]**
//**SRS_EVENTHUBRECEIVER_29_082: \[**`EventHubReceiverAsyncThreadEntry` shall poll using ThreadAPI_Sleep with an interval of 1ms.**\]**
//**SRS_EVENTHUBRECEIVER_29_085: \[**`EventHubReceiverAsyncThreadEntry` shall "Perform Destroy" after all references to the handle have been removed.**\]**
void TestHelper_SetupCommonWorkloopCallStack(size_t freeDispatchCallbackCount, bool isEventData)
{
    size_t idx;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_VALID));

    EXPECTED_CALL(EventHubReceiver_LL_DoWork(TEST_EVENTHUB_RECEIVER_LL_VALID));

    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_VALID));

    if (!freeDispatchCallbackCount)
    {
        EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    }

    for (idx = 0; idx < freeDispatchCallbackCount; idx++)
    {
        EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));

        if (isEventData) STRICT_EXPECTED_CALL(EventData_Destroy(TEST_EVENTDATA_HANDLE_VALID));

        EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    if (freeDispatchCallbackCount)
    {
        EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(1));
}

//**SRS_EVENTHUBRECEIVER_29_045: \[**The deferred callbacks shall allocate memory to dispatch the resulting callbacks after calling EventHubReceiver_LL_DoWork. **\]**
//**SRS_EVENTHUBRECEIVER_29_046: \[**The deferred callbacks shall save off the callback result. **\]**
//**SRS_EVENTHUBRECEIVER_29_047: \[**`EHR_OnAsyncReceiveCB` shall clone the event data using API EventData_Clone. **\]**
//**SRS_EVENTHUBRECEIVER_29_048: \[**The deferred callbacks shall enqueue the dispatch by calling DList_InsertTailList**\]**
//**SRS_EVENTHUBRECEIVER_29_042: \[**Upon failures, error messages shall be logged and any allocated memory or data structures shall be deallocated.**\]**
void TestHelper_SetupDeferredCallbackCommon(bool isEventData)
{
    umock_c_reset_all_calls();

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    if (isEventData) STRICT_EXPECTED_CALL(EventData_Clone(TEST_EVENTDATA_HANDLE_VALID));

    EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

//**SRS_EVENTHUBRECEIVER_29_062: \[**Upon Success, `EventHubReceiver_ReceiveEndAsync` shall return EVENTHUBRECEIVER_OK.**\]**
//**SRS_EVENTHUBRECEIVER_29_063: \[**Upon failure, `EventHubReceiver_ReceiveEndAsync` shall return EVENTHUBRECEIVER_ERROR and a message will be logged.**\]**
//**SRS_EVENTHUBRECEIVER_29_064: \[**`EventHubReceiver_ReceiveEndAsync` shall call EventHubReceiver_ReceiveEndAsync_LL.**\]**
//**SRS_EVENTHUBRECEIVER_29_065: \[**`EventHubReceiver_ReceiveEndAsync` shall pass `EHR_OnAsyncEndCB` along with eventHubReceiverHandle as corresponding context arguments to `EventHubReceiver_ReceiveEndAsync_LL` so as to defer execution of the user provided callbacks to the `EHR_AsyncWorkLoopThreadEntry` workloop thread. This step will only be required if the user has passed in a callback.**\]**
void TestHelper_SetupCommonReceiveEndAsyncStack(void)
{
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_VALID));

    STRICT_EXPECTED_CALL(EventHubReceiver_LL_ReceiveEndAsync(TEST_EVENTHUB_RECEIVER_LL_VALID, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument_onEventReceiveEndUserContext().IgnoreArgument_onEventReceiveEndCallback();

    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_VALID));
}

// **SRS_EVENTHUBRECEIVER_29_001: \[**`EventHubReceiver_Create` shall pass the connectionString, eventHubPath, consumerGroup and partitionId arguments to `EventHubReceiver_LL_Create`.**\]**
// **SRS_EVENTHUBRECEIVER_29_003: \[**Upon Success of `EventHubReceiver_LL_Create`, `EventHubReceiver_Create` shall allocate the internal structures as required by this module.**\]**
// **SRS_EVENTHUBRECEIVER_29_006: \[**`EventHubReceiver_Create` shall initialize a DLIST by calling DList_InitializeListHead for queuing callbacks resulting from the invocation of `EventHubReceiver_LL_DoWork` from the `EHR_AsyncWorkLoopThreadEntry` workloop thread. **\]**
// **SRS_EVENTHUBRECEIVER_29_004: \[**Upon Success `EventHubReceiver_Create` shall return the EVENTHUBRECEIVER_HANDLE.**\]**
// **SRS_EVENTHUBRECEIVER_29_002: \[**`EventHubReceiver_Create` shall return a NULL value if `EventHubReceiver_LL_Create` returns NULL.**\]**
// **SRS_EVENTHUBRECEIVER_29_005: \[**Upon Failure `EventHubReceiver_Create` shall return NULL.**\]**
void TestHelper_SetupCommonReceiverCreateStack(void)
{
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID));

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(Lock_Init());

    EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG));
}

//#################################################################################################
// EventHubReceiver Tests
//#################################################################################################
BEGIN_TEST_SUITE(eventhubreceiver_unittests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    REGISTER_UMOCK_ALIAS_TYPE(uint64_t, unsigned long long);
    REGISTER_UMOCK_ALIAS_TYPE(bool, unsigned char);
    
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, const void*);

    REGISTER_UMOCK_ALIAS_TYPE(EVENTHUBRECEIVER_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EVENTDATA_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EVENTHUBRECEIVER_ASYNC_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EVENTHUBRECEIVER_ASYNC_END_CALLBACK, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, TestHook_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, TestHook_free);

    REGISTER_GLOBAL_MOCK_RETURN(Lock_Init, TEST_LOCK_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Init, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Lock_Deinit, TestHook_Lock_Deinit);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock_Deinit, LOCK_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(Lock, LOCK_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Lock, LOCK_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(Unlock, LOCK_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Unlock, LOCK_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, TestHook_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(ThreadAPI_Join, THREADAPI_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Join, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Sleep, TestHook_ThreadAPI_Sleep);

    REGISTER_GLOBAL_MOCK_HOOK(DList_InitializeListHead, TestHook_DList_InitializeListHead);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveHeadList, TestHook_DList_RemoveHeadList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertTailList, TestHook_DList_InsertTailList);

    REGISTER_GLOBAL_MOCK_RETURN(EventData_CreateWithNewMemory, TEST_EVENTDATA_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventData_CreateWithNewMemory, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(EventData_Clone, TEST_EVENTDATA_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventData_Clone, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(EventHubReceiver_LL_Create, TEST_EVENTHUB_RECEIVER_LL_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventHubReceiver_LL_Create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(EventHubReceiver_LL_ReceiveEndAsync, TestHook_EventHubReceiver_LL_ReceiveEndAsync);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventHubReceiver_LL_ReceiveEndAsync, EVENTHUBRECEIVER_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync, TestHook_EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync, EVENTHUBRECEIVER_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(EventHubReceiver_LL_ReceiveFromStartTimestampAsync, EVENTHUBRECEIVER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventHubReceiver_LL_ReceiveFromStartTimestampAsync, EVENTHUBRECEIVER_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(EventHubReceiver_LL_SetConnectionTracing, EVENTHUBRECEIVER_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventHubReceiver_LL_SetConnectionTracing, EVENTHUBRECEIVER_ERROR);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("Mutex is ABANDONED. Failure in test framework");
    }
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

//#################################################################################################
//
// EventHubReceiver_Create Tests
//
//#################################################################################################

TEST_FUNCTION(EventHubReceiver_Create_Success)
{
    // arrange
    STRICT_EXPECTED_CALL(EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID));

    TestHelper_SetupCommonReceiverCreateStack();

    // act
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_IS_NOT_NULL_WITH_MSG(h, "Failed NULL Value Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_Create_Failure)
{
    // arrange
    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    TestHelper_SetupCommonReceiverCreateStack();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        if (i != 3)
        {
            // act
            EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
            // assert
            ASSERT_IS_NULL(h);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

//#################################################################################################
//
// EventHubReceiver_ReceiveFromStartTimestampAsync Tests
//
//#################################################################################################

//**SRS_EVENTHUBRECEIVER_29_030: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampAsync_NULL_Handle_Param)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = NULL;
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_ReceiveFromStartTimestampAsync(h,
        EventHubHReceiver_OnRxCB, OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, OnErrCBCtxt,
        nowTS);
    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

//**SRS_EVENTHUBRECEIVER_29_030: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampAsync_NULL_RxCallback_Param)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = TEST_EVENTHUB_RECEIVER_VALID;
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_ReceiveFromStartTimestampAsync(h,
        NULL, OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, OnErrCBCtxt,
        nowTS);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

//**SRS_EVENTHUBRECEIVER_29_030: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampAsync_NULL_RxErrorCallback_Param)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = TEST_EVENTHUB_RECEIVER_VALID;
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_ReceiveFromStartTimestampAsync(h,
        EventHubHReceiver_OnRxCB, OnRxCBCtxt,
        NULL, OnErrCBCtxt,
        nowTS);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampAsync_Success)
{
    // arrange
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_SetupReceiveFromStartTimestampAsyncCommon(0);

    // act
    EVENTHUBRECEIVER_RESULT result = EventHubReceiver_ReceiveFromStartTimestampAsync(h,
        EventHubHReceiver_OnRxCB, OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, OnErrCBCtxt,
        TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

// **SRS_EVENTHUBRECEIVER_29_039: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall return an error code of EVENTHUBRECEIVER_NOT_ALLOWED if a user called EventHubReceiver_Receive* more than once on the same handle.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampAsync_Multiple)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_ReceiveFromStartTimestampAsync(h,
        EventHubHReceiver_OnRxCB, OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, OnErrCBCtxt,
        nowTS);

    result = EventHubReceiver_ReceiveFromStartTimestampAsync(h,
        EventHubHReceiver_OnRxCB, OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, OnErrCBCtxt,
        nowTS);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_NOT_ALLOWED, result);

    // cleanup
    EventHubReceiver_Destroy(h);
}

//**SRS_EVENTHUBRECEIVER_29_042: \[**Upon failures, EVENTHUBRECEIVER_ERROR shall be returned.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampAsync_Failure)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;

    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    TestHelper_SetupReceiveFromStartTimestampAsyncCommon(0);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
        EVENTHUBRECEIVER_RESULT result;
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        if (i != 3)
        {
            // act
            result = EventHubReceiver_ReceiveFromStartTimestampAsync(h,
                EventHubHReceiver_OnRxCB, OnRxCBCtxt,
                EventHubHReceiver_OnErrCB, OnErrCBCtxt,
                nowTS);

            // assert
            ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_ERROR, result);
        }
        EventHubReceiver_Destroy(h);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

//#################################################################################################
//
// EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync Tests
//
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_Timeout_Zero_Success)
{
    // arrange
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_SetupReceiveFromStartTimestampAsyncCommon(0);

    // act
    EVENTHUBRECEIVER_RESULT result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, OnErrCBCtxt,
        TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP, 0);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_Timeout_NonZero_Success)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_SetupReceiveFromStartTimestampAsyncCommon(TEST_EVENTHUB_RECEIVER_TIMEOUT);

    // act
    EVENTHUBRECEIVER_RESULT result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
                                                                                                EventHubHReceiver_OnRxCB, OnRxCBCtxt,
                                                                                                EventHubHReceiver_OnErrCB, OnErrCBCtxt,
                                                                                                nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

// **SRS_EVENTHUBRECEIVER_29_030: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_NULL_Handle_Params)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(NULL,
        EventHubHReceiver_OnRxCB, OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);
    
    //assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

// **SRS_EVENTHUBRECEIVER_29_030: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_NULL_RxCallback_Params)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(TEST_EVENTHUB_RECEIVER_VALID,
        NULL, OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);
    
    //assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

// **SRS_EVENTHUBRECEIVER_29_030: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_NULL_RxErrorCallback_Params)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(TEST_EVENTHUB_RECEIVER_VALID,
        EventHubHReceiver_OnRxCB, OnRxCBCtxt,
        NULL, OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

// **SRS_EVENTHUBRECEIVER_29_039: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall return an error code of EVENTHUBRECEIVER_NOT_ALLOWED if a user called EventHubReceiver_Receive* more than once on the same handle.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_Multiple)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_NOT_ALLOWED, result);

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_Non_Zero_Timeout_Negative)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;

    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    TestHelper_SetupReceiveFromStartTimestampAsyncCommon(TEST_EVENTHUB_RECEIVER_TIMEOUT);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
        EVENTHUBRECEIVER_RESULT result;
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        if (i != 3)
        {
            // act
            result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
                EventHubHReceiver_OnRxCB, OnRxCBCtxt,
                EventHubHReceiver_OnErrCB, OnErrCBCtxt,
                nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

            // assert
            ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_ERROR, result);
        }        
        EventHubReceiver_Destroy(h);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_Zero_Timeout_Failure)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;

    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    TestHelper_SetupReceiveFromStartTimestampAsyncCommon(0);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
        EVENTHUBRECEIVER_RESULT result;
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        // act
        if (i != 3)
        {
            // act
            result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
                EventHubHReceiver_OnRxCB, OnRxCBCtxt,
                EventHubHReceiver_OnErrCB, OnErrCBCtxt,
                nowTS, 0);

            // assert
            ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_ERROR, result);
        }
        EventHubReceiver_Destroy(h);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

//#################################################################################################
//
// EventHubReceiver_ReceiveFromStartTimestamp*Async Callback Tests
//
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_ReceiveAsyncCallback_Success)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    ResetTestGlobalData();

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    TestHelper_SetupDeferredCallbackCommon(true);

    // act
    OnRxCBStruct.eventHubRxHandle = h;
    OnRxCBStruct.rxCallback(EVENTHUBRECEIVER_OK, TEST_EVENTDATA_HANDLE_VALID, OnRxCBStruct.rxCallbackCtxt);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveAsyncCallback_Failure)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    ResetTestGlobalData();

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    TestHelper_SetupDeferredCallbackCommon(true);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        
        // act
        OnRxCBStruct.rxCallback(EVENTHUBRECEIVER_OK, TEST_EVENTDATA_HANDLE_VALID, OnRxCBStruct.rxCallbackCtxt);

        // assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    }

    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveErrorAsyncCallback_Success)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    ResetTestGlobalData();

    umock_c_reset_all_calls();

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    TestHelper_SetupDeferredCallbackCommon(false);

    // act
    OnRxCBStruct.eventHubRxHandle = h;
    OnRxCBStruct.rxErrorCallback(EVENTHUBRECEIVER_ERROR, OnRxCBStruct.rxErrorCallbackCtxt);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveErrorAsyncCallback_Failure)
{
    // arrange
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_RESULT result;

    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    ResetTestGlobalData();

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    TestHelper_SetupDeferredCallbackCommon(false);

    umock_c_negative_tests_snapshot();

    OnRxCBStruct.eventHubRxHandle = h;
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        OnRxCBStruct.rxErrorCallback(EVENTHUBRECEIVER_ERROR, OnRxCBStruct.rxErrorCallbackCtxt);

        // assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    }

    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_Destroy(h);
}

//#################################################################################################
//
// EventHubReceiver_ReceiveFromStartTimestamp*Async Workloop Tests
//
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_ReceiveAsyncWorkloop_MultipleCallback_Success)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;
    int threadReturn;

    ResetTestGlobalData();

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    // setup receive 2 callbacks
    OnRxCBStruct.eventHubRxHandle = h;
    OnRxCBStruct.rxCallback(EVENTHUBRECEIVER_OK, TEST_EVENTDATA_HANDLE_VALID, OnRxCBStruct.rxCallbackCtxt);
    OnRxCBStruct.rxCallback(EVENTHUBRECEIVER_OK, TEST_EVENTDATA_HANDLE_VALID, OnRxCBStruct.rxCallbackCtxt);

    TestHelper_SetupCommonWorkloopCallStack(2, true);

    // act
    threadReturn = threadEntry(threadEntryCtxt);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, threadReturn, "Failed Thread Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, OnRxCBStruct.rxCallbackResult, "Failed Receive Callback Result Value Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveAsyncWorkloop_ZeroCallback_Success)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;
    int threadReturn;

    ResetTestGlobalData();

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    OnRxCBStruct.eventHubRxHandle = h;
    TestHelper_SetupCommonWorkloopCallStack(0, false);

    // act
    threadReturn = threadEntry(threadEntryCtxt);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, threadReturn, "Failed Thread Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveErrorAsyncWorkloop_Success)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;
    int threadReturn;

    ResetTestGlobalData();

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);
    
    // this will queue up 2 error callbacks
    OnRxCBStruct.eventHubRxHandle = h;
    OnRxCBStruct.rxErrorCallback(EVENTHUBRECEIVER_ERROR, OnRxCBStruct.rxErrorCallbackCtxt);
    OnRxCBStruct.rxErrorCallback(EVENTHUBRECEIVER_ERROR, OnRxCBStruct.rxErrorCallbackCtxt);

    TestHelper_SetupCommonWorkloopCallStack(2, false);

    // act
    threadReturn = threadEntry(threadEntryCtxt);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, threadReturn, "Failed Thread Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_ERROR, OnRxCBStruct.rxErrorCallbackResult, "Failed Receive Error Callback Result Value Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveTimeoutAsyncWorkloop_Success)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;
    int threadReturn;

    ResetTestGlobalData();

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    // this will queue up 2 timeout callbacks
    OnRxCBStruct.eventHubRxHandle = h;
    OnRxCBStruct.rxCallback(EVENTHUBRECEIVER_TIMEOUT, NULL, OnRxCBStruct.rxCallbackCtxt);
    OnRxCBStruct.rxCallback(EVENTHUBRECEIVER_TIMEOUT, NULL, OnRxCBStruct.rxCallbackCtxt);

    TestHelper_SetupCommonWorkloopCallStack(2, false);

    // act
    threadReturn = threadEntry(threadEntryCtxt);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, threadReturn, "Failed Thread Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_TIMEOUT, OnRxCBStruct.rxCallbackResult, "Failed Receive Callback Result Value Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveEndAsyncWorkloop_Success)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;
    int threadReturn;

    ResetTestGlobalData();

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    result = EventHubReceiver_ReceiveEndAsync(h, EventHubHReceiver_OnRxEndCB, OnRxEndCBCtxt);

    // this will queue up 2 async end callbacks
    OnRxCBStruct.eventHubRxHandle = h;
    OnRxCBStruct.rxEndCallback(EVENTHUBRECEIVER_OK, OnRxCBStruct.rxEndCallbackCtxt);
    OnRxCBStruct.rxEndCallback(EVENTHUBRECEIVER_OK, OnRxCBStruct.rxEndCallbackCtxt);

    TestHelper_SetupCommonWorkloopCallStack(2, false);

    // act
    threadReturn = threadEntry(threadEntryCtxt);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, threadReturn, "Failed Thread Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, OnRxCBStruct.rxEndCallbackResult, "Failed Receive Async End Callback Result Value Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveEndAsyncWorkloop_Negative)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;
    int threadReturn;

    ResetTestGlobalData();

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    OnRxCBStruct.eventHubRxHandle = h;
    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_VALID));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        // act
        threadReturn = threadEntry(threadEntryCtxt);
        // assert
        ASSERT_ARE_EQUAL(int, 0, threadReturn);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_Destroy(h);
}

//#################################################################################################
//
// EventHubReceiver_ReceiveEndAsync Tests
//
//#################################################################################################

// **SRS_EVENTHUBRECEIVER_29_060: \[**`EventHubReceiver_ReceiveEndAsync` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveEndAsync_NULL_Handle_Param)
{
    // arrange
    EVENTHUBRECEIVER_RESULT result;
    result = EventHubReceiver_ReceiveEndAsync(0, EventHubHReceiver_OnRxEndCB, OnRxEndCBCtxt);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

// **SRS_EVENTHUBRECEIVER_29_061: \[**`EventHubReceiver_ReceiveEndAsync` shall check if a receiver connection is currently active. If no receiver is active, EVENTHUBRECEIVER_NOT_ALLOWED shall be returned and a message will be logged.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveEndAsync_Inactive_Success)
{
    // arrange
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;
 
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_VALID));
    
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_VALID));

    // act
    result = EventHubReceiver_ReceiveEndAsync(h, EventHubHReceiver_OnRxEndCB, OnRxEndCBCtxt);
    
    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_NOT_ALLOWED, result, "Failed Return Value Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_ReceiveEndAsync_Active_Success)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    umock_c_reset_all_calls();

    TestHelper_SetupCommonReceiveEndAsyncStack();

    // act
    result = EventHubReceiver_ReceiveEndAsync(h, EventHubHReceiver_OnRxEndCB, OnRxEndCBCtxt);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

//**SRS_EVENTHUBRECEIVER_29_063: \[**Upon failure, `EventHubReceiver_ReceiveEndAsync` shall return EVENTHUBRECEIVER_ERROR and a message will be logged.**\]**
TEST_FUNCTION(EventHubReceiver_ReceiveEndAsync_Active_Failure)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    result = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    umock_c_reset_all_calls();

    TestHelper_SetupCommonReceiveEndAsyncStack();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        if (i != 2)
        {
            // act
            result = EventHubReceiver_ReceiveEndAsync(h, EventHubHReceiver_OnRxEndCB, OnRxEndCBCtxt);

            // assert
            ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_ERROR, result);
        }
    }
    
    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_Destroy(h);
}

//#################################################################################################
//
// EventHubReceiver_Destroy Tests
//
//#################################################################################################

TEST_FUNCTION(EventHubReceiver_Destroy_NULL)
{
    ResetTestGlobalData();

    // act
    EventHubReceiver_Destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test"); 
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.lockDeinitInvoked, "Lock Deinit State Not Valid");
    
    // cleanup
}

TEST_FUNCTION(EventHubReceiver_Destroy_ReceiverInActive_Success)
{
    // arrange
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    ResetTestGlobalData();
    OnRxCBStruct.eventHubRxHandle = h;
    
    TestHelper_SetupCommonDestroyCallStack(0, false, false);

    // act
    EventHubReceiver_Destroy(h);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.lockDeinitInvoked, "Lock Deinit State Not Valid");

    // cleanup
}

TEST_FUNCTION(EventHubReceiver_Destroy_ReceiverActive_Success)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    
    ResetTestGlobalData();

    (void)EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
        EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
        EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
        nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);

    // setup receive 2 callbacks
    OnRxCBStruct.eventHubRxHandle = h;
    OnRxCBStruct.rxCallback(EVENTHUBRECEIVER_OK, TEST_EVENTDATA_HANDLE_VALID, OnRxCBStruct.rxCallbackCtxt);
    OnRxCBStruct.rxCallback(EVENTHUBRECEIVER_OK, TEST_EVENTDATA_HANDLE_VALID, OnRxCBStruct.rxCallbackCtxt);

    TestHelper_SetupCommonDestroyCallStack(2, true, true);

    // act
    EventHubReceiver_Destroy(h);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.lockDeinitInvoked, "Lock Deinit State Not Valid");
}

TEST_FUNCTION(EventHubReceiver_Destroy_ReceiverActive_Negative)
{
    // arrange
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    ResetTestGlobalData();

    (void)EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(h,
                EventHubHReceiver_OnRxCB, &OnRxCBCtxt,
                EventHubHReceiver_OnErrCB, &OnErrCBCtxt,
                nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT);


    OnRxCBStruct.eventHubRxHandle = h;
    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ThreadAPI_Join(TEST_THREAD_HANDLE_VALID, IGNORED_PTR_ARG)).IgnoreArgument(2);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        
        // act
        EventHubReceiver_Destroy(h);

        // assert
        ASSERT_ARE_EQUAL(int, 1, OnRxCBStruct.lockDeinitInvoked);
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

//#################################################################################################
//
// EventHubReceiver_SetConnectionTracing Tests
//
//#################################################################################################

//**SRS_EVENTHUBRECEIVER_29_020: \[**`EventHubReceiver_SetConnectionTracing` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
TEST_FUNCTION(EventHubReceiver_SetConnectionTracing_NULL_Handle)
{
    // act
    EVENTHUBRECEIVER_RESULT result = EventHubReceiver_SetConnectionTracing(NULL, true);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

//**SRS_EVENTHUBRECEIVER_29_021: \[**`EventHubReceiver_SetConnectionTracing` shall pass the arguments to `EventHubReceiver_LL_SetConnectionTracing`.**\]**
//**SRS_EVENTHUBRECEIVER_29_023: \[**Upon Success, `EventHubReceiver_SetConnectionTracing` shall return EVENTHUBRECEIVER_OK.**\]**
TEST_FUNCTION(EventHubReceiver_SetConnectionTracing_Success)
{
    // arrange
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;
    bool traceFlag = true;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_VALID));

    STRICT_EXPECTED_CALL(EventHubReceiver_LL_SetConnectionTracing(TEST_EVENTHUB_RECEIVER_LL_VALID, traceFlag));

    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_VALID));
    
    // act
    result = EventHubReceiver_SetConnectionTracing(h, traceFlag);

    //assert
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");

    // cleanup
    EventHubReceiver_Destroy(h);
}

//**SRS_EVENTHUBRECEIVER_29_021: \[**`EventHubReceiver_SetConnectionTracing` shall pass the arguments to `EventHubReceiver_LL_SetConnectionTracing`.**\]**
//**SRS_EVENTHUBRECEIVER_29_023: \[**Upon Success, `EventHubReceiver_SetConnectionTracing` shall return EVENTHUBRECEIVER_OK.**\]**
TEST_FUNCTION(EventHubReceiver_SetConnectionTracing_MultipleSuccess)
{
    // arrange
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    // act 1
    result = EventHubReceiver_SetConnectionTracing(h, true);
    // assert 1
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Set Connection Tracing Test #1");

    // act 2
    result = EventHubReceiver_SetConnectionTracing(h, false);
    // assert 2
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Set Connection Tracing Test #2");

    // cleanup
    EventHubReceiver_Destroy(h);
}

//**SRS_EVENTHUBRECEIVER_29_021: \[**`EventHubReceiver_SetConnectionTracing` shall pass the arguments to `EventHubReceiver_LL_SetConnectionTracing`.**\]**
//**SRS_EVENTHUBRECEIVER_29_022: \[**If `EventHubReceiver_LL_SetConnectionTracing` returns an error, the code is returned to the user and a message will logged.**\]**
//**SRS_EVENTHUBRECEIVER_29_024: \[**Upon failure, `EventHubReceiver_SetConnectionTracing` shall return EVENTHUBRECEIVER_ERROR and a message will be logged.**\]**
TEST_FUNCTION(EventHubReceiver_Set_ConnectionTracing_Failure)
{
    // arrange
    EVENTHUBRECEIVER_HANDLE h = EventHubReceiver_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    bool traceFlag = true;

    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_VALID));

    STRICT_EXPECTED_CALL(EventHubReceiver_LL_SetConnectionTracing(TEST_EVENTHUB_RECEIVER_LL_VALID, traceFlag));

    umock_c_negative_tests_snapshot();
    
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        EVENTHUBRECEIVER_RESULT result;
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        // act
        result = EventHubReceiver_SetConnectionTracing(h, traceFlag);
        // assert
        ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_ERROR, result);
    }

    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_Destroy(h);
}

END_TEST_SUITE(eventhubreceiver_unittests)
