# EventHubReceiver Requirements


## Overview

The EventHubReceiver is a native library used for communication with an existing Event Hub with the sole purpose of reading 
event payload from a specific partition.
The EventHubReceiver module is simply a convenience wrapper on top of the EventHubReceiver_LL module.

## References

[Event Hubs](http://msdn.microsoft.com/en-us/library/azure/dn789973.aspx)

[EVENTHUBRECEIVER Class for .NET](http://msdn.microsoft.com/en-us/library/microsoft.servicebus.messaging.EVENTHUBRECEIVER.aspx)

## Exposed API
```c
typedef void* EVENTHUBRECEIVER_HANDLE;

extern EVENTHUBRECEIVER_HANDLE EventHubReceiver_Create
(
    const char* connectionString,
    const char* eventHubPath,
    const char* consumerGroup,
    const char* partitionId
);

extern void EventHubReceiver_Destroy(EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle);

extern EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveFromStartTimestampAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    uint64_t startTimestampInSec
);

extern EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    uint64_t startTimestampInSec,
    unsigned int waitTimeoutInMs
);

extern EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveEndAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_END_CALLBACK onEventReceiveEndCallback,
    void* onEventReceiveEndUserContext
);

extern EVENTHUBRECEIVER_RESULT EventHubReceiver_Set_ConnectionTracing
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    bool traceEnabled
);
```
												
### EventHubReceiver_Create
```c
extern EVENTHUBRECEIVER_HANDLE EventHubReceiver_Create
(
    const char* connectionString,
    const char* eventHubPath,
    const char* consumerGroup,
    const char* partitionId
);
```

**SRS_EVENTHUBRECEIVER_29_001: \[**`EventHubReceiver_Create` shall pass the connectionString, eventHubPath, consumerGroup and partitionId arguments to `EventHubReceiver_LL_Create`.**\]**
**SRS_EVENTHUBRECEIVER_29_002: \[**`EventHubReceiver_Create` shall return a NULL value if `EventHubReceiver_LL_Create` returns NULL.**\]**
**SRS_EVENTHUBRECEIVER_29_003: \[**Upon Success of `EventHubReceiver_LL_Create`, `EventHubReceiver_Create` shall allocate the internal structures as required by this module.**\]**
**SRS_EVENTHUBRECEIVER_29_006: \[**`EventHubReceiver_Create` shall initialize a DLIST by calling DList_InitializeListHead for queuing callbacks resulting from the invocation of `EventHubReceiver_LL_DoWork` from the `EHR_AsyncWorkLoopThreadEntry` workloop thread. **\]**
**SRS_EVENTHUBRECEIVER_29_004: \[**Upon Success `EventHubReceiver_Create` shall return the EVENTHUBRECEIVER_HANDLE.**\]**
**SRS_EVENTHUBRECEIVER_29_005: \[**Upon Failure `EventHubReceiver_Create` shall return NULL.**\]**

### EventHubReceiver_Destroy
```c
extern void EventHubReceiver_Destroy(EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle);
```
**SRS_EVENTHUBRECEIVER_29_050: \[**`EventHubReceiver_Destroy` shall call ThreadAPI_Join if required to wait for the workloop thread.**\]**
**SRS_EVENTHUBRECEIVER_29_051: \[**`EventHubReceiver_Destroy` shall pass the EventHubReceiver_LL_Handle to `EventHubReceiver_Destroy_LL`.**\]**
**SRS_EVENTHUBRECEIVER_29_052: \[**After call to `EventHubReceiver_Destroy_LL`, free up any allocated structures.**\]**
**SRS_EVENTHUBRECEIVER_29_053: \[**If any errors are seen, a message will be logged.**\]**

### EventHubReceiver_ReceiveFromStartTimestampAsync and EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync
```c
extern EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveFromStartTimestampAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    uint64_t startTimestampInSec
);

extern EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    uint64_t startTimestampInSec,
    unsigned int waitTimeoutInMs
);
```
**SRS_EVENTHUBRECEIVER_29_030: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_031: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall save off the user supplied callbacks and contexts.**\]**
**SRS_EVENTHUBRECEIVER_29_033: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall pass `EHR_OnAsyncReceiveCB`, `EHR_OnAsyncReceiveErrorCB` along with eventHubReceiverHandle as corresponding context arguments to `EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` so as to defer execution of these callbacks to the workloop thread.**\]**
**SRS_EVENTHUBRECEIVER_29_034: \[**`EventHubReceiver_ReceiveFromStartTimestampAsync` shall invoke `EventHubReceiver_LL_ReceiveFromStartTimestampAsync`.**\]**
**SRS_EVENTHUBRECEIVER_29_035: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` invoke `EventHubReceiver_LL_ReceiveFromStartTimestampAsync` if waitTimeout is zero.**\]**
**SRS_EVENTHUBRECEIVER_29_036: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` invoke `EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync` if waitTimeout is non zero.**\]**
**SRS_EVENTHUBRECEIVER_29_037: \[**If `EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` returns an error, any allocated memory is freed and the error code is returned to the user and a message will logged.**\]**
**SRS_EVENTHUBRECEIVER_29_038: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall create workloop thread which performs the requirements under "EHR_AsyncWorkLoopThreadEntry"**\]**
**SRS_EVENTHUBRECEIVER_29_039: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall return an error code of EVENTHUBRECEIVER_NOT_ALLOWED if a user called EventHubReceiver_Receive* more than once on the same handle.**\]**
**SRS_EVENTHUBRECEIVER_29_041: \[**Upon Success EVENTHUBRECEIVER_OK shall be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_042: \[**Upon failures, EVENTHUBRECEIVER_ERROR shall be returned.**\]**

#### Receiver Do Work
```c
static int EHR_AsyncWorkLoopThreadEntry(void* userContextCallback)
```
**SRS_EVENTHUBRECEIVER_29_080: \[**`EHR_AsyncWorkLoopThreadEntry` shall synchronously call EventHubReceiver_LL_DoWork.**\]**
**SRS_EVENTHUBRECEIVER_29_081: \[**`EHR_AsyncWorkLoopThreadEntry` shall invoke any queued user callbacks as a result of `EventHubReceiver_LL_DoWork` calling after the lock has been released.**\]**
**SRS_EVENTHUBRECEIVER_29_082: \[**`EHR_AsyncWorkLoopThreadEntry` shall poll using ThreadAPI_Sleep with an interval of 1ms.**\]**

#### Callback Deferring
```c
static void EHR_OnAsyncReceiveCB(EVENTHUBRECEIVER_RESULT result, EVENTDATA_HANDLE eventDataHandle, void* context);
static void EHR_OnAsyncReceiveErrorCB(EVENTHUBRECEIVER_RESULT errorCode, void* context);
static void EHR_OnAsyncEndCB(EVENTHUBRECEIVER_RESULT result, void* context);
```
**SRS_EVENTHUBRECEIVER_29_045: \[**The deferred callbacks shall allocate memory to dispatch the resulting callbacks after calling EventHubReceiver_LL_DoWork. **\]**
**SRS_EVENTHUBRECEIVER_29_046: \[**The deferred callbacks shall save off the callback result. **\]**
**SRS_EVENTHUBRECEIVER_29_047: \[**`EHR_OnAsyncReceiveCB` shall clone the event data using API EventData_Clone. **\]**
**SRS_EVENTHUBRECEIVER_29_048: \[**The deferred callbacks shall enqueue the dispatch by calling DList_InsertTailList**\]**
**SRS_EVENTHUBRECEIVER_29_042: \[**Upon failures, error messages shall be logged and any allocated memory or data structures shall be deallocated.**\]**

### EventHubReceiver_Set_ConnectionTracing
```c
extern EVENTHUBRECEIVER_RESULT EventHubReceiver_Set_ConnectionTracing
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    bool traceEnabled
);
```
**SRS_EVENTHUBRECEIVER_29_020: \[**`EventHubReceiver_Set_ConnectionTracing` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_021: \[**`EventHubReceiver_Set_ConnectionTracing` shall pass the arguments to `EventHubReceiver_LL_SetConnectionTracing`.**\]**
**SRS_EVENTHUBRECEIVER_29_022: \[**If `EventHubReceiver_LL_SetConnectionTracing` returns an error, the code is returned to the user and a message will logged.**\]**
**SRS_EVENTHUBRECEIVER_29_023: \[**Upon Success, `EventHubReceiver_Set_ConnectionTracing` shall return EVENTHUBRECEIVER_OK.**\]**
**SRS_EVENTHUBRECEIVER_29_024: \[**Upon failure, `EventHubReceiver_Set_ConnectionTracing` shall return EVENTHUBRECEIVER_ERROR and a message will be logged.**\]**
 
### 
```c
extern EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveEndAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_END_CALLBACK onEventReceiveEndCallback,
    void* onEventReceiveEndUserContext
);
```
**SRS_EVENTHUBRECEIVER_29_060: \[**`EventHubReceiver_ReceiveEndAsync` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_061: \[**`EventHubReceiver_ReceiveEndAsync` shall check if a receiver connection is currently active. If no receiver is active, EVENTHUBRECEIVER_NOT_ALLOWED shall be returned and a message will be logged.**\]**
**SRS_EVENTHUBRECEIVER_29_062: \[**Upon Success, `EventHubReceiver_ReceiveEndAsync` shall return EVENTHUBRECEIVER_OK.**\]**
**SRS_EVENTHUBRECEIVER_29_063: \[**Upon failure, `EventHubReceiver_ReceiveEndAsync` shall return EVENTHUBRECEIVER_ERROR and a message will be logged.**\]**
**SRS_EVENTHUBRECEIVER_29_064: \[**`EventHubReceiver_ReceiveEndAsync` shall call EventHubReceiver_ReceiveEndAsync_LL.**\]**
**SRS_EVENTHUBRECEIVER_29_065: \[**`EventHubReceiver_ReceiveEndAsync` shall pass `EHR_OnAsyncEndCB` along with eventHubReceiverHandle as corresponding context arguments to `EventHubReceiver_ReceiveEndAsync_LL` so as to defer execution of the user provided callbacks to the `EHR_AsyncWorkLoopThreadEntry` workloop thread. This step will only be required if the user has passed in a callback.**\]**