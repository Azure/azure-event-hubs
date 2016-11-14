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
    unsigned long long startTimestampInSec
);

extern EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    unsigned long long startTimestampInSec,
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

**SRS_EVENTHUBRECEIVER_29_001: \[**`EventHubReceiver_Create` shall pass the connectionString, eventHubPath, consumerGroup and partitionId arguments to `EventHubReceiver_Create_LL`.**\]**
**SRS_EVENTHUBRECEIVER_29_002: \[**`EventHubReceiver_Create` shall return a NULL value if `EventHubReceiver_Create_LL` returns NULL.**\]**
**SRS_EVENTHUBRECEIVER_29_003: \[**Upon Success of `EventHubReceiver_Create_LL`, `EventHubReceiver_Create` shall allocate the internal structures as required by this module.**\]**
**SRS_EVENTHUBRECEIVER_29_004: \[**Upon Success `EventHubReceiver_Create` shall return the EVENTHUBRECEIVER_HANDLE.**\]**
**SRS_EVENTHUBRECEIVER_29_005: \[**Upon Failure `EventHubReceiver_Create` shall return NULL.**\]**

### EventHubReceiver_Destroy
```c
extern void EventHubReceiver_Destroy(EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle);
```
**SRS_EVENTHUBRECEIVER_29_050: \[**`EventHubReceiver_Destroy` shall end any receiver communication if a connection is active by calling `EventHubReceiver_ReceiveEndAsync_LL`.**\]**
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
    unsigned long long startTimestampInSec
);

extern EVENTHUBRECEIVER_RESULT EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    unsigned long long startTimestampInSec,
    unsigned int waitTimeoutInMs
);
```
**SRS_EVENTHUBRECEIVER_29_030: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_031: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall allocate memory to save off the user supplied callbacks and contexts.**\]**
**SRS_EVENTHUBRECEIVER_29_032: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall initialize a DList by calling DList_InitializeListHead for queuing callbacks resulting from the invocation of `EventHubReceiver_LL_DoWork` from the workloop thread.**\]**
**SRS_EVENTHUBRECEIVER_29_033: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall pass `EventHubReceiver_OnReceiveCB`, `EventHubReceiver_OnReceiveErrorCB` along with eventHubReceiverHandle as corresponding context arguments to `EventHubReceiver_ReceiveFromStartTimestamp*Async_LL` so as to defer execution of these callbacks to the workloop thread. Requirements for these are listed in Callback Deferring section**\]**
**SRS_EVENTHUBRECEIVER_29_034: \[**`EventHubReceiver_ReceiveFromStartTimestampAsync` shall invoke `EventHubReceiver_ReceiveFromStartTimestampAsync_LL`.**\]**
**SRS_EVENTHUBRECEIVER_29_035: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` invoke `EventHubReceiver_ReceiveFromStartTimestampAsync_LL` if waitTimeout is zero.**\]**
**SRS_EVENTHUBRECEIVER_29_036: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` invoke `EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_LL` if waitTimeout is non zero.**\]**
**SRS_EVENTHUBRECEIVER_29_037: \[**If `EventHubReceiver_ReceiveFromStartTimestamp*Async_LL` returns an error, any allocated memory is freed and the error code is returned to the user and a message will logged.**\]**
**SRS_EVENTHUBRECEIVER_29_038: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall create workloop thread which performs the "Receiver Do Work"**\]**
**SRS_EVENTHUBRECEIVER_29_039: \[**`EventHubReceiver_ReceiveFromStartTimestamp*Async` shall return an error code of EVENTHUBRECEIVER_ERROR if a user called EventHubReceiver_Receive* more than once on the same handle.**\]**
**SRS_EVENTHUBRECEIVER_29_041: \[**Upon Success EVENTHUBRECEIVER_OK shall be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_042: \[**Upon failures, EVENTHUBRECEIVER_ERROR shall be returned.**\]**

#### Receiver Do Work
```c
static int EventHubReceiverAsyncThreadEntry(void* userContextCallback)
```
**SRS_EVENTHUBRECEIVER_29_080: \[**`EventHubReceiverAsyncThreadEntry` shall call EventHubReceiver_LL_DoWork.**\]**
**SRS_EVENTHUBRECEIVER_29_081: \[**`EventHubReceiverAsyncThreadEntry` shall invoke any queued user callbacks as a result of `EventHubReceiver_LL_DoWork` calling after the lock has been released.**\]**
**SRS_EVENTHUBRECEIVER_29_082: \[**`EventHubReceiverAsyncThreadEntry` shall poll using ThreadAPI_Sleep with an interval of 1ms.**\]**

#### Callback Deferring
```c
static void EventHubReceiver_OnReceiveCB(EVENTHUBRECEIVER_RESULT result, EVENTDATA_HANDLE eventDataHandle, void* context);
static void EventHubReceiver_OnReceiveErrorCB(EVENTHUBRECEIVER_RESULT errorCode, void* context);
```
**SRS_EVENTHUBRECEIVER_29_045: \[**`EventHubReceiver_OnReceiveCB` and `EventHubReceiver_OnReceiveErrorCB` shall allocate memory to dispatch the resulting callbacks after calling EventHubReceiver_LL_DoWork. **\]**
**SRS_EVENTHUBRECEIVER_29_046: \[**`EventHubReceiver_OnReceiveCB` and `EventHubReceiver_OnReceiveErrorCB` shall save off the callback result. **\]**
**SRS_EVENTHUBRECEIVER_29_047: \[**`EventHubReceiver_OnReceiveCB` shall clone the event data using API EventData_Clone. **\]**
**SRS_EVENTHUBRECEIVER_29_048: \[**Both callbacks enqueue the dispatch by calling DList_InsertTailList**\]**
**SRS_EVENTHUBRECEIVER_29_042: \[**Upon failures, error messages shall be logged.**\]**

### EventHubReceiver_Set_ConnectionTracing
```c
extern EVENTHUBRECEIVER_RESULT EventHubReceiver_Set_ConnectionTracing
(
    EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle,
    bool traceEnabled
);
```
**SRS_EVENTHUBRECEIVER_29_020: \[**`EventHubReceiver_Set_ConnectionTracing` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_021: \[**`EventHubReceiver_Set_ConnectionTracing` shall pass the arguments to `EventHubReceiver_Set_ConnectionTracing_LL`.**\]**
**SRS_EVENTHUBRECEIVER_29_022: \[**If `EventHubReceiver_Set_ConnectionTracing_LL` returns an error, the code is returned to the user and a message will logged.**\]**
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
**SRS_EVENTHUBRECEIVER_29_061: \[**`EventHubReceiver_ReceiveEndAsync` shall check if a receiver connection is currently active and if so it will create a worker thread and have it perform functionality in Receiver End Communication.**\]**
**SRS_EVENTHUBRECEIVER_29_061: \[**If no receiver connection is active, `EventHubReceiver_ReceiveEndAsync` shall not invoke the user callback and supply error code EVENTHUBRECEIVER_ERROR.**\]**
**SRS_EVENTHUBRECEIVER_29_062: \[**Upon Success, `EventHubReceiver_ReceiveEndAsync` shall return EVENTHUBRECEIVER_OK.**\]**
**SRS_EVENTHUBRECEIVER_29_063: \[**Upon failure, `EventHubReceiver_ReceiveEndAsync` shall return EVENTHUBRECEIVER_ERROR and a message will be logged.**\]**

#### Receiver End Communication
```c
static int EventHubReceiverEndAsyncThreadEntry(void* userContextCallback)
```
**SRS_EVENTHUBRECEIVER_29_070: \[**`EventHubReceiverEndAsyncThreadEntry` shall call EventHubReceiver_ReceiveEndAsync_LL.**\]**
**SRS_EVENTHUBRECEIVER_29_071: \[**`EventHubReceiverEndAsyncThreadEntry` shall invoke the user registered callback and supply EVENTHUBRECEIVER_OK on success or EVENTHUBRECEIVER_ERROR on failure as the result code along with the user supplied context.**\]**
**SRS_EVENTHUBRECEIVER_29_072: \[**`EventHubReceiverEndAsyncThreadEntry` shall return 0 on success and -1 if any errors are observed.**\]**
**SRS_EVENTHUBRECEIVER_29_073: \[**`EventHubReceiverEndAsyncThreadEntry` shall join to wait for the receiver work loop thread.**\]**
**SRS_EVENTHUBRECEIVER_29_074: \[**`EventHubReceiverEndAsyncThreadEntry` shall free up any prior active receiver specific data structures.**\]**

