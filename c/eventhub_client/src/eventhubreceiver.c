// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <signal.h>

#include "version.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"

#include "eventhubreceiver.h"

#define THREAD_END					    0
#define THREAD_RUN						1

/* Event Hub Receiver States */
#define EVENTHUBRECEIVER_STATE_VALUES   \
        RECEIVER_STATE_INACTIVE,        \
        RECEIVER_STATE_ACTIVE

DEFINE_ENUM(EVENTHUBRECEIVER_STATE, EVENTHUBRECEIVER_STATE_VALUES);

#define DISPATCH_TYPE_VALUES            \
        DISPATCH_RECEIVEASYNC,          \
        DISPATCH_RECEIVEASYNC_ERROR,    \
        DISPATCH_RECEIVEASYNC_END

DEFINE_ENUM(DISPATCH_TYPE, DISPATCH_TYPE_VALUES);

// sleep for specified milliseconds
#define EVENTHUBRECEIVER_LL_DO_WORK_TIMEPERIOD_MS 1

typedef struct ASYNC_CALLBACK_DISPATCH_STRUCT_TAG
{
    DLIST_ENTRY             entry;
    DISPATCH_TYPE           type;
    EVENTHUBRECEIVER_RESULT callbackResultCode;
    EVENTDATA_HANDLE        dataHandle;
} ASYNC_CALLBACK_DISPATCH_STRUCT;

typedef struct EVENTHUBRECEIVER_STRUCT_TAG
{
    EVENTHUBRECEIVER_LL_HANDLE  eventHubReceiverLLHandle;
    THREAD_HANDLE               threadHandle;
    LOCK_HANDLE                 lockInfo;
    EVENTHUBRECEIVER_STATE      state;
    volatile sig_atomic_t       threadState;
    EVENTHUBRECEIVER_ASYNC_CALLBACK       onEventReceiveCallback;
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback;
    EVENTHUBRECEIVER_ASYNC_END_CALLBACK   onEventReceiveEndCallback;
    void* onEventReceiveUserContext;
    void* onEventReceiveErrorUserContext;
    void* onEventReceiveEndUserContext;
    DLIST_ENTRY receiveCallbackList;
} EVENTHUBRECEIVER_STRUCT;

typedef struct EVENTHUBRECEIVER_ASYNC_CALLBACKS_STRUCT_TAG
{
    EVENTHUBRECEIVER_ASYNC_CALLBACK       onEventReceiveCallback;
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback;
    EVENTHUBRECEIVER_ASYNC_END_CALLBACK   onEventReceiveEndCallback;
    void* onEventReceiveUserContext;
    void* onEventReceiveErrorUserContext;
    void* onEventReceiveEndUserContext;
} EVENTHUBRECEIVER_ASYNC_CALLBACKS_STRUCT;

static void AsyncDispatchCallbackListInit(EVENTHUBRECEIVER_STRUCT* ehStruct);
static void AsyncDispatchCallbackListDeInit(EVENTHUBRECEIVER_STRUCT* ehStruct);
static void AsyncDispatchCallbackListAdd(EVENTHUBRECEIVER_STRUCT* ehStruct, ASYNC_CALLBACK_DISPATCH_STRUCT* dispatch);
static void AsyncDispatchCallbackListProcess(EVENTHUBRECEIVER_STRUCT* ehStruct, EVENTHUBRECEIVER_ASYNC_CALLBACKS_STRUCT* callbacks);

static void EHR_OnAsyncReceiveCB(EVENTHUBRECEIVER_RESULT result, EVENTDATA_HANDLE eventDataHandle, void* context);
static void EHR_OnAsyncReceiveErrorCB(EVENTHUBRECEIVER_RESULT errorCode, void* context);
static void EHR_OnAsyncEndCB(EVENTHUBRECEIVER_RESULT result, void* userContextCallback);

const size_t THREAD_STATE_OFFSET = offsetof(EVENTHUBRECEIVER_STRUCT, threadState);
const int    THREAD_STATE_EXIT   = THREAD_END;

EVENTHUBRECEIVER_HANDLE EventHubReceiver_Create
(
    const char* connectionString,
    const char* eventHubPath,
    const char* consumerGroup,
    const char* partitionId
)
{
    EVENTHUBRECEIVER_LL_HANDLE llHandle;
    EVENTHUBRECEIVER_STRUCT* result;
    
    //**SRS_EVENTHUBRECEIVER_29_001: \[**`EventHubReceiver_Create` shall pass the connectionString, eventHubPath, consumerGroup and partitionId arguments to `EventHubReceiver_LL_Create`.**\]**
    llHandle = EventHubReceiver_LL_Create(connectionString, eventHubPath, consumerGroup, partitionId);
    if (llHandle == NULL)
    {
        //**SRS_EVENTHUBRECEIVER_29_002: \[**`EventHubReceiver_Create` shall return a NULL value if `EventHubReceiver_LL_Create` returns NULL.**\]**
        result = NULL;
        LogError("EventHubReceiver_LL_Create returned NULL.\r\n");
    }
    else
    {
        //**SRS_EVENTHUBRECEIVER_29_003: \[**Upon Success of `EventHubReceiver_LL_Create`, `EventHubReceiver_Create` shall allocate the internal structures as required by this module.**\]**
        result = (EVENTHUBRECEIVER_STRUCT*)malloc(sizeof(EVENTHUBRECEIVER_STRUCT));
        if (result == NULL)
        {
            //**SRS_EVENTHUBRECEIVER_29_005: \[**Upon Failure EventHubReceiver_Create shall return NULL.**\]**
            EventHubReceiver_LL_Destroy(llHandle);
            LogError("EventHubReceiver_Create no memory to allocate struct.\r\n");
        }
        else
        {
            if ((result->lockInfo = Lock_Init()) == NULL)
            {
                free(result);
                //**SRS_EVENTHUBRECEIVER_29_005: \[**Upon Failure EventHubReceiver_Create shall return NULL.**\]**
                result = NULL;
                EventHubReceiver_LL_Destroy(llHandle);
                LogError("EventHubReceiver_Create Lock init failed.\r\n");
            }
            else
            {
                result->eventHubReceiverLLHandle = llHandle;
                result->threadHandle = NULL;
                result->threadState = THREAD_END;
                result->state = RECEIVER_STATE_INACTIVE;
                result->onEventReceiveCallback = NULL;
                result->onEventReceiveErrorCallback = NULL;
                result->onEventReceiveEndCallback = NULL;
                result->onEventReceiveUserContext = NULL;
                result->onEventReceiveErrorUserContext = NULL;
                result->onEventReceiveEndUserContext = NULL;
                //**SRS_EVENTHUBRECEIVER_29_006: \[**`EventHubReceiver_Create` shall initialize a DLIST by calling DList_InitializeListHead for queuing callbacks resulting from the invocation of `EventHubReceiver_LL_DoWork` from the `EHR_AsyncWorkLoopThreadEntry` workloop thread. **\]**
                AsyncDispatchCallbackListInit(result);
            }
        }
    }
    //**SRS_EVENTHUBRECEIVER_29_004: \[**Upon Success EventHubReceiver_Create shall return the EVENTHUBRECEIVER_HANDLE.**\]**
    return (EVENTHUBRECEIVER_HANDLE)result;
}

void EventHubReceiver_Destroy(EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle)
{
    EVENTHUBRECEIVER_STRUCT* ehStruct = (EVENTHUBRECEIVER_STRUCT*)eventHubReceiverHandle;

    if (ehStruct != NULL)
    {
        THREADAPI_RESULT joinResult;
        int threadExitResult = 0;

        ehStruct->threadState = THREAD_END;
        //**SRS_EVENTHUBRECEIVER_29_050: \[**`EventHubReceiver_Destroy` shall call ThreadAPI_Join if required to wait for the workloop thread.**\]**
        if ((ehStruct->threadHandle != NULL) && ((joinResult = ThreadAPI_Join(ehStruct->threadHandle, &threadExitResult)) != THREADAPI_OK))
        {
            //**SRS_EVENTHUBRECEIVER_29_053: \[**If any errors are seen, a message will be logged.**\]**
            LogError("Join Failed.\r\n");
        }
        //**SRS_EVENTHUBRECEIVER_29_051: \[**`EventHubReceiver_Destroy` shall pass the EventHubReceiver_LL_Handle to `EventHubReceiver_Destroy_LL`.**\]**
        EventHubReceiver_LL_Destroy(ehStruct->eventHubReceiverLLHandle);
        //**SRS_EVENTHUBRECEIVER_29_052: \[**After call to `EventHubReceiver_LL_Destroy`, free up any allocated structures.**\]**
        AsyncDispatchCallbackListDeInit(ehStruct);
        Lock_Deinit(ehStruct->lockInfo);
        free(ehStruct);
    }
}

EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveEndAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_END_CALLBACK onEventReceiveEndCallback,
    void* onEventReceiveEndUserContext
)
{
    EVENTHUBRECEIVER_STRUCT* ehStruct = (EVENTHUBRECEIVER_STRUCT*)eventHubReceiverHandle;
    EVENTHUBRECEIVER_RESULT result;

    //**SRS_EVENTHUBRECEIVER_29_060: \[**`EventHubReceiver_ReceiveEndAsync` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
    if (ehStruct == NULL)
    {
        result = EVENTHUBRECEIVER_INVALID_ARG;
    }
    else
    {
        if (Lock(ehStruct->lockInfo) != LOCK_OK)
        {
            //**SRS_EVENTHUBRECEIVER_29_063: \[**Upon failure, `EventHubReceiver_ReceiveEndAsync` shall return EVENTHUBRECEIVER_ERROR and a message will be logged.**\]**
            LogError("Could not acquire lock!");
            result = EVENTHUBRECEIVER_ERROR;
        }
        else
        {
            //**SRS_EVENTHUBRECEIVER_29_061: \[**`EventHubReceiver_ReceiveEndAsync` shall check if a receiver connection is currently active, If no receiver is active, EVENTHUBRECEIVER_NOT_ALLOWED shall be returned and a message will be logged.**\]**
            if (ehStruct->state != RECEIVER_STATE_ACTIVE)
            {
                LogError("Operation not permitted as there is no active receiver connection.\r\n");
                result = EVENTHUBRECEIVER_NOT_ALLOWED;
            }
            else
            {
                EVENTHUBRECEIVER_ASYNC_END_CALLBACK eventRxEndCallback = NULL;
                void* eventRxEndCallbackContext = NULL;
                //**SRS_EVENTHUBRECEIVER_29_065: \[**`EventHubReceiver_ReceiveEndAsync` shall pass `EHR_OnAsyncEndCB` along with eventHubReceiverHandle as corresponding context arguments to `EventHubReceiver_ReceiveEndAsync_LL` so as to defer execution of the user provided callbacks to the `EHR_AsyncWorkLoopThreadEntry` workloop thread. This step will only be required if the user has passed in a callback.**\]**
                if (onEventReceiveEndCallback)
                {
                    eventRxEndCallback = EHR_OnAsyncEndCB;
                    eventRxEndCallbackContext = (void*)ehStruct;
                }
                //**SRS_EVENTHUBRECEIVER_29_064: \[**`EventHubReceiver_ReceiveEndAsync` shall call EventHubReceiver_ReceiveEndAsync_LL.**\]**
                if ((result = EventHubReceiver_LL_ReceiveEndAsync(ehStruct->eventHubReceiverLLHandle, eventRxEndCallback, eventRxEndCallbackContext)) != EVENTHUBRECEIVER_OK)
                {
                    //**SRS_EVENTHUBRECEIVER_29_063: \[**Upon failure, `EventHubReceiver_ReceiveEndAsync` shall return EVENTHUBRECEIVER_ERROR and a message will be logged.**\]**
                    LogError("EventHubReceiver_LL_ReceiveEndAsync Error Code:%u.\r\n", result);
                }
                else
                {
                    //**SRS_EVENTHUBRECEIVER_29_062: \[**Upon Success, `EventHubReceiver_ReceiveEndAsync` shall return EVENTHUBRECEIVER_OK.**\]**
                    ehStruct->onEventReceiveCallback = NULL;
                    ehStruct->onEventReceiveUserContext = NULL;
                    ehStruct->onEventReceiveErrorCallback = NULL;
                    ehStruct->onEventReceiveErrorUserContext = NULL;
                    ehStruct->onEventReceiveEndCallback = onEventReceiveEndCallback;
                    ehStruct->onEventReceiveEndUserContext = onEventReceiveEndUserContext;
                    ehStruct->state = RECEIVER_STATE_INACTIVE;
                }
            }
            (void)Unlock(ehStruct->lockInfo);
        }
    }
    
    return result;
}

static int EHR_AsyncWorkLoopThreadEntry(void* userContextCallback)
{
    EVENTHUBRECEIVER_STRUCT* ehStruct = (EVENTHUBRECEIVER_STRUCT*)userContextCallback;

    while (ehStruct->threadState != THREAD_END)
    {
        EVENTHUBRECEIVER_ASYNC_CALLBACKS_STRUCT callbacks;

        if (Lock(ehStruct->lockInfo) != LOCK_OK)
        {
            LogError("Could not acquire lock!");
        }
        else
        {
            //**SRS_EVENTHUBRECEIVER_29_080: \[**`EHR_AsyncWorkLoopThreadEntry` shall synchronously call EventHubReceiver_LL_DoWork.**\]**
            EventHubReceiver_LL_DoWork(ehStruct->eventHubReceiverLLHandle);
            callbacks.onEventReceiveCallback = ehStruct->onEventReceiveCallback;
            callbacks.onEventReceiveUserContext = ehStruct->onEventReceiveUserContext;
            callbacks.onEventReceiveErrorCallback = ehStruct->onEventReceiveErrorCallback;
            callbacks.onEventReceiveErrorUserContext = ehStruct->onEventReceiveErrorUserContext;
            callbacks.onEventReceiveEndCallback = ehStruct->onEventReceiveEndCallback;
            callbacks.onEventReceiveEndUserContext = ehStruct->onEventReceiveEndUserContext;
            (void)Unlock(ehStruct->lockInfo);
        }
        //**SRS_EVENTHUBRECEIVER_29_081: \[**`EHR_AsyncWorkLoopThreadEntry` shall invoke any queued user callbacks as a result of `EventHubReceiver_LL_DoWork` calling after the lock has been released.**\]**
        AsyncDispatchCallbackListProcess(ehStruct, &callbacks);
        //**SRS_EVENTHUBRECEIVER_29_082: \[**`EHR_AsyncWorkLoopThreadEntry` shall poll using ThreadAPI_Sleep with an interval of 1ms.**\]**
        ThreadAPI_Sleep(EVENTHUBRECEIVER_LL_DO_WORK_TIMEPERIOD_MS);
    }

    return 0;
}

ASYNC_CALLBACK_DISPATCH_STRUCT* DispatchCreateHelper(EVENTHUBRECEIVER_RESULT resultCode, EVENTDATA_HANDLE eventDataHandle, DISPATCH_TYPE type)
{
    ASYNC_CALLBACK_DISPATCH_STRUCT* dispatch;

    //**SRS_EVENTHUBRECEIVER_29_045: \[**The deferred callbacks shall allocate memory to dispatch the resulting callbacks after calling EventHubReceiver_LL_DoWork. **\]**
    if ((dispatch = (ASYNC_CALLBACK_DISPATCH_STRUCT*)malloc(sizeof(ASYNC_CALLBACK_DISPATCH_STRUCT))) == NULL)
    {
        //**SRS_EVENTHUBRECEIVER_29_042: \[**Upon failures, error messages shall be logged.**\]**
        LogError("Could not create dispatch struct.\r\n");
    }
    //**SRS_EVENTHUBRECEIVER_29_047: \[**`EHR_OnAsyncReceiveCB` shall clone the event data using API EventData_Clone. **\]**
    else if (eventDataHandle && ((dispatch->dataHandle = EventData_Clone(eventDataHandle)) == NULL))
    {
        //**SRS_EVENTHUBRECEIVER_29_042: \[**Upon failures, error messages shall be logged and any allocated memory or data structures shall be deallocated.**\]**
        LogError("Could not clone EVENTDATA handle.\r\n");
        free(dispatch);
        dispatch = NULL;
    }
    else
    {
        if (eventDataHandle == NULL)
        {
            dispatch->dataHandle = NULL;
        }
        //**SRS_EVENTHUBRECEIVER_29_046: \[**The deferred callbacks shall save off the callback result. **\]**
        dispatch->callbackResultCode = resultCode;
        dispatch->type = type;
    }

    return dispatch;
}

static void EHR_OnAsyncReceiveCB(EVENTHUBRECEIVER_RESULT result, EVENTDATA_HANDLE eventDataHandle, void* context)
{
    EVENTHUBRECEIVER_STRUCT* ehStruct = (EVENTHUBRECEIVER_STRUCT*)context;
    ASYNC_CALLBACK_DISPATCH_STRUCT* dispatch = DispatchCreateHelper(result, eventDataHandle, DISPATCH_RECEIVEASYNC);
    if (dispatch != NULL)
    {
        //**SRS_EVENTHUBRECEIVER_29_048: \[**The deferred callbacks shall enqueue the dispatch by calling DList_InsertTailList**\]**
        AsyncDispatchCallbackListAdd(ehStruct, dispatch);
    }
}

static void EHR_OnAsyncReceiveErrorCB(EVENTHUBRECEIVER_RESULT errorCode, void* context)
{
    EVENTHUBRECEIVER_STRUCT* ehStruct = (EVENTHUBRECEIVER_STRUCT*)context;
    ASYNC_CALLBACK_DISPATCH_STRUCT* dispatch = DispatchCreateHelper(errorCode, NULL, DISPATCH_RECEIVEASYNC_ERROR);
    if (dispatch != NULL)
    {
        //**SRS_EVENTHUBRECEIVER_29_048: \[**The deferred callbacks shall enqueue the dispatch by calling DList_InsertTailList**\]**
        AsyncDispatchCallbackListAdd(ehStruct, dispatch);
    }
}

static void EHR_OnAsyncEndCB(EVENTHUBRECEIVER_RESULT result, void* context)
{
    EVENTHUBRECEIVER_STRUCT* ehStruct = (EVENTHUBRECEIVER_STRUCT*)context;
    ASYNC_CALLBACK_DISPATCH_STRUCT* dispatch = DispatchCreateHelper(result, NULL, DISPATCH_RECEIVEASYNC_END);
    if (dispatch != NULL)
    {
        //**SRS_EVENTHUBRECEIVER_29_048: \[**The deferred callbacks shall enqueue the dispatch by calling DList_InsertTailList**\]**
        AsyncDispatchCallbackListAdd(ehStruct, dispatch);
    }
}

static EVENTHUBRECEIVER_RESULT ExecuteCommonReceiveFromStartTimestampAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    unsigned long long startTimestampInSec,
    unsigned int waitTimeoutInMs
)
{
    EVENTHUBRECEIVER_RESULT result;
    THREADAPI_RESULT threadResult;
    EVENTHUBRECEIVER_STRUCT* ehStruct = (EVENTHUBRECEIVER_STRUCT*)eventHubReceiverHandle;

    //**SRS_EVENTHUBRECEIVER_29_030: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
    if (ehStruct == NULL || onEventReceiveCallback == NULL || onEventReceiveErrorCallback == NULL)
    {
        LogError("Invalid arguments\r\n");
        result = EVENTHUBRECEIVER_INVALID_ARG;
    }
    else if (Lock(ehStruct->lockInfo) == LOCK_OK)
    {
        //**SRS_EVENTHUBRECEIVER_29_039: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall return an error code of EVENTHUBRECEIVER_NOT_ALLOWED if a user called EventHubReceiver_Receive* more than once on the same handle.**\]**
        if (ehStruct->state != RECEIVER_STATE_INACTIVE)
        {
            result = EVENTHUBRECEIVER_NOT_ALLOWED;
            LogError("Operation not permitted as there is an exiting Receiver instance.\r\n");
        }
        else
        {
            //**SRS_EVENTHUBRECEIVER_29_033: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall pass `EHR_OnAsyncReceiveCB`, `EHR_OnAsyncReceiveErrorCB` along with eventHubReceiverHandle as corresponding context arguments to `EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` so as to defer execution of these callbacks to the workloop thread.**\]**
			if (waitTimeoutInMs == 0)
            {
                //**SRS_EVENTHUBRECEIVER_29_034: \[**`EventHubReceiver_ReceiveFromStartTimestampAsync` shall invoke `EventHubReceiver_ReceiveFromStartTimestampAsync_LL`.**\]**
                //**SRS_EVENTHUBRECEIVER_29_035: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` invoke `EventHubReceiver_ReceiveFromStartTimestampAsync_LL` if waitTimeout is zero.**\]**
                result = EventHubReceiver_LL_ReceiveFromStartTimestampAsync(ehStruct->eventHubReceiverLLHandle,
                    EHR_OnAsyncReceiveCB,
                    ehStruct,
                    EHR_OnAsyncReceiveErrorCB,
                    ehStruct,
                    startTimestampInSec);
            }
            else
            {
                //**SRS_EVENTHUBRECEIVER_29_036: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` invoke `EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_LL` if waitTimeout is non zero.**\]**
                result = EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(ehStruct->eventHubReceiverLLHandle,
                    EHR_OnAsyncReceiveCB,
                    ehStruct,
                    EHR_OnAsyncReceiveErrorCB,
                    ehStruct,
                    startTimestampInSec,
                    waitTimeoutInMs);
            }
            if (result != EVENTHUBRECEIVER_OK)
            {
                //**SRS_EVENTHUBRECEIVER_29_037: \[**If `EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` returns an error, any allocated memory is freed and the error code is returned to the user and a message will logged.**\]**
                LogError("EventHubReceiver_LL_ReceiveFromStartTimestamp*Async Error Code:%u \r\n", result);
            }
            //**SRS_EVENTHUBRECEIVER_29_038: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall create workloop thread which performs the requirements under "EHR_AsyncWorkLoopThreadEntry"**\]**
            else if (ehStruct->threadHandle == NULL)
            {
                ehStruct->threadState = THREAD_RUN;
                if ((threadResult = ThreadAPI_Create(&ehStruct->threadHandle, EHR_AsyncWorkLoopThreadEntry, (void*)ehStruct)) != THREADAPI_OK)
                {
                    //**SRS_EVENTHUBRECEIVER_29_042: \[**Upon failures, EVENTHUBRECEIVER_ERROR shall be returned.**\]**
                    LogError("Could not create workloop thread %u.\r\n", threadResult);
                    ehStruct->threadState = THREAD_END;
                    result = EVENTHUBRECEIVER_ERROR;
                }
                else
                {
                    //**SRS_EVENTHUBRECEIVER_29_041: \[**Upon Success EVENTHUBRECEIVER_OK shall be returned.**\]**
                    result = EVENTHUBRECEIVER_OK;
                }
            }

            if (result == EVENTHUBRECEIVER_OK)
            {
                ehStruct->state = RECEIVER_STATE_ACTIVE;
                //**SRS_EVENTHUBRECEIVER_29_031: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall save off the user supplied callbacks and contexts.**\]**
                ehStruct->onEventReceiveCallback = onEventReceiveCallback;
                ehStruct->onEventReceiveUserContext = onEventReceiveUserContext;
                ehStruct->onEventReceiveErrorCallback = onEventReceiveErrorCallback;
                ehStruct->onEventReceiveErrorUserContext = onEventReceiveErrorUserContext;
                ehStruct->onEventReceiveEndCallback = NULL;
                ehStruct->onEventReceiveEndUserContext = NULL;
            }
        }
        (void)Unlock(ehStruct->lockInfo);
    }
    else
    {
        //**SRS_EVENTHUBRECEIVER_29_042: \[**Upon failures, EVENTHUBRECEIVER_ERROR shall be returned.**\]**
        result = EVENTHUBRECEIVER_ERROR;
        LogError("Could not acquire lock.\r\n");
    }

    return result;
}

EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveFromStartTimestampAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    uint64_t startTimestampInSec
)
{
    return ExecuteCommonReceiveFromStartTimestampAsync(eventHubReceiverHandle, onEventReceiveCallback,
                                            onEventReceiveUserContext, onEventReceiveErrorCallback,
                                            onEventReceiveErrorUserContext, startTimestampInSec, 0);
}

EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    uint64_t startTimestampInSec,
    unsigned int waitTimeoutInMs
)
{
    return ExecuteCommonReceiveFromStartTimestampAsync(eventHubReceiverHandle, onEventReceiveCallback,
                                            onEventReceiveUserContext, onEventReceiveErrorCallback,
                                            onEventReceiveErrorUserContext, startTimestampInSec, waitTimeoutInMs);
}

EVENTHUBRECEIVER_RESULT EventHubReceiver_SetConnectionTracing
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    bool traceEnabled
)
{
    EVENTHUBRECEIVER_RESULT result;
    EVENTHUBRECEIVER_STRUCT* ehStruct = (EVENTHUBRECEIVER_STRUCT*)eventHubReceiverHandle;
    //**SRS_EVENTHUBRECEIVER_29_020: \[**`EventHubReceiver_Set_ConnectionTracing` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
    if (ehStruct == NULL)
    {
        LogError("Invalid arguments\r\n");
        result = EVENTHUBRECEIVER_INVALID_ARG;
    }
    else
    {
        if (Lock(ehStruct->lockInfo) == LOCK_OK)
        {
            //**SRS_EVENTHUBRECEIVER_29_021: \[**`EventHubReceiver_Set_ConnectionTracing` shall pass the arguments to `EventHubReceiver_LL_SetConnectionTracing`.**\]**
            //**SRS_EVENTHUBRECEIVER_29_023: \[**Upon Success, `EventHubReceiver_Set_ConnectionTracing` shall return EVENTHUBRECEIVER_OK.**\]**
            result = EventHubReceiver_LL_SetConnectionTracing(ehStruct->eventHubReceiverLLHandle, traceEnabled);
            (void)Unlock(ehStruct->lockInfo);
            //**SRS_EVENTHUBRECEIVER_29_022: \[**If `EventHubReceiver_LL_SetConnectionTracing` returns an error, the code is returned to the user and a message will logged.**\]**
            if (result != EVENTHUBRECEIVER_OK)
            {
                LogError("EventHubReceiver_LL_SetConnectionTracing Error Code:%u \r\n", result);
            }
        }
        else
        {
            //**SRS_EVENTHUBRECEIVER_29_024: \[**Upon failure, `EventHubReceiver_Set_ConnectionTracing` shall return EVENTHUBRECEIVER_ERROR and a message will be logged.**\]**
            result = EVENTHUBRECEIVER_ERROR;
            LogError("Could not acquire lock.\r\n");
        }
    }

    return result;
}

static void AsyncDispatchCallbackListInit(EVENTHUBRECEIVER_STRUCT* ehStruct)
{
    DList_InitializeListHead(&ehStruct->receiveCallbackList);
}

static void AsyncDispatchCallbackListDeInit(EVENTHUBRECEIVER_STRUCT* ehStruct)
{
    PDLIST_ENTRY list = &ehStruct->receiveCallbackList;
    PDLIST_ENTRY tempEntry;
    while ((tempEntry = DList_RemoveHeadList(list)) != list)
    {
        ASYNC_CALLBACK_DISPATCH_STRUCT* dispatch = containingRecord(tempEntry, ASYNC_CALLBACK_DISPATCH_STRUCT, entry);
        if (dispatch->dataHandle)
        {
            EventData_Destroy(dispatch->dataHandle);
        }
        free(dispatch);
    }
}

static void AsyncDispatchCallbackListAdd(EVENTHUBRECEIVER_STRUCT* ehStruct, ASYNC_CALLBACK_DISPATCH_STRUCT *asyncCallback)
{
    DList_InsertTailList(&ehStruct->receiveCallbackList, &(asyncCallback->entry));
}

static void AsyncDispatchCallbackListProcess(EVENTHUBRECEIVER_STRUCT* ehStruct, EVENTHUBRECEIVER_ASYNC_CALLBACKS_STRUCT* callbacks)
{
    PDLIST_ENTRY list = &ehStruct->receiveCallbackList;
    PDLIST_ENTRY tempEntry;

    while ((tempEntry = DList_RemoveHeadList(list)) != list)
    {
        ASYNC_CALLBACK_DISPATCH_STRUCT* dispatch = containingRecord(tempEntry, ASYNC_CALLBACK_DISPATCH_STRUCT, entry);
        if ((dispatch->type == DISPATCH_RECEIVEASYNC) && (callbacks->onEventReceiveCallback))
        {
            callbacks->onEventReceiveCallback(dispatch->callbackResultCode, dispatch->dataHandle, callbacks->onEventReceiveUserContext);
            if (dispatch->dataHandle)
            {
                EventData_Destroy(dispatch->dataHandle);
            }
        }
        else if ((dispatch->type == DISPATCH_RECEIVEASYNC_ERROR) && (callbacks->onEventReceiveErrorCallback))
        {
            callbacks->onEventReceiveErrorCallback(dispatch->callbackResultCode, callbacks->onEventReceiveErrorUserContext);
        }
        else if ((dispatch->type == DISPATCH_RECEIVEASYNC_END) && (callbacks->onEventReceiveEndCallback))
        {
            callbacks->onEventReceiveEndCallback(dispatch->callbackResultCode, callbacks->onEventReceiveEndUserContext);
        }
        free(dispatch);
    }
}