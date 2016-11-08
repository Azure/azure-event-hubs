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
    unsigned int partitionId
);

extern EVENTHUBRECEIVER_RESULT EventHubReceiver_Close(EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle);

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
    unsigned int partitionId
);
```

**SRS_EVENTHUBRECEIVER_29_001: \[**`EventHubReceiver_Create` shall pass the connectionString, eventHubPath, consumerGroup and partitionKey arguments to `EventHubReceiver_Create_LL`.**\]**
**SRS_EVENTHUBRECEIVER_29_002: \[**`EventHubReceiver_Create` shall return a NULL value if `EventHubReceiver_Create_LL` returns NULL.**\]**
**SRS_EVENTHUBRECEIVER_29_003: \[**Upon Success of `EventHubReceiver_Create_LL`, `EventHubReceiver_Create` shall allocate the internal structures as required by this module.**\]**
**SRS_EVENTHUBRECEIVER_29_004: \[**Upon Success `EventHubReceiver_Create` shall return the EVENTHUBRECEIVER_HANDLE.**\]**
**SRS_EVENTHUBRECEIVER_29_005: \[**Upon Failure `EventHubReceiver_Create` shall return NULL.**\]**

### EventHubReceiver_Close
```c
extern EVENTHUBRECEIVER_RESULT EventHubReceiver_Close(EVENTHUBRECEIVER_HANDLE eventHubReceiverHandle);
```
**SRS_EVENTHUBRECEIVER_29_006: \[**`EventHubReceiver_Close` shall pass the EventHubReceiver_LL_Handle to `EventHubReceiver_Close_LL`.**\]**
**SRS_EVENTHUBRECEIVER_29_007: \[**Upon failure of `EventHubReceiver_Close_LL` a log message will be logged and an error code will be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_008: \[**After call to `EventHubReceiver_Close_LL`, free up any allocated structures.**\]**
**SRS_EVENTHUBRECEIVER_29_009: \[**Upon Success, `EventHubReceiver_Close` shall return EVENTHUBRECEIVER_OK.**\]**
**SRS_EVENTHUBRECEIVER_29_010: \[**If any errors are seen, a message will be logged and error code EVENTHUBRECEIVER_ERROR will be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_011: \[**`EventHubReceiver_Close` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**

### EventHubReceiver_ReceiveFromStartTimestampAsync
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
```
**SRS_EVENTHUBRECEIVER_29_030: \[**`EventHubReceiver_ReceiveFromStartTimestampAsync` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_031: \[**`EventHubReceiver_ReceiveFromStartTimestampAsync` shall pass the arguments to `EventHubReceiver_ReceiveFromStartTimestampAsync_LL`.**\]**
**SRS_EVENTHUBRECEIVER_29_033: \[**Create a workloop thread which calls EventHubReceiver_LL_DoWork in a polled fashion with polling interval of 1ms**\]**
**SRS_EVENTHUBRECEIVER_29_034: \[**If `EventHubReceiver_ReceiveFromStartTimestampAsync_LL` returns an error, any allocated memory is freed and the error code is returned to the user and a message will logged.**\]**
**SRS_EVENTHUBRECEIVER_29_035: \[**Upon Success EVENTHUBRECEIVER_OK shall be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_036: \[**Upon failure, EVENTHUBRECEIVER_ERROR shall be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_037: \[**`EventHubReceiver_ReceiveFromStartTimestampAsync` shall return an error code of EVENTHUBRECEIVER_ERROR if a user called EventHubReceiver_Receive* more than once on the same handle.**\]**

### EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync
```c
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
**SRS_EVENTHUBRECEIVER_29_040: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_041: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` shall pass the arguments to `EventHubReceiver_ReceiveFromStartTimestampAsync_LL` if waitTimeout is zero.**\]**
**SRS_EVENTHUBRECEIVER_29_042: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` shall pass the arguments to `EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_LL` if waitTimeout is non zero.**\]**
**SRS_EVENTHUBRECEIVER_29_033: \[**Create a workloop thread which calls EventHubReceiver_LL_DoWork in a polled fashion with polling interval of 1ms**\]**
**SRS_EVENTHUBRECEIVER_29_043: \[**If `EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync_LL` returns an error, any allocated memory is freed and the error code is returned to the user and a message will logged.**\]**
**SRS_EVENTHUBRECEIVER_29_034: \[**If `EventHubReceiver_ReceiveFromStartTimestampAsync_LL` returns an error, any allocated memory is freed and the error code is returned to the user and a message will logged.**\]**
**SRS_EVENTHUBRECEIVER_29_035: \[**Upon Success EVENTHUBRECEIVER_OK shall be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_036: \[**Upon failures, EVENTHUBRECEIVER_ERROR shall be returned.**\]**
**SRS_EVENTHUBRECEIVER_29_044: \[**`EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync` shall return an error code of EVENTHUBRECEIVER_ERROR if a user called EventHubReceiver_Receive* more than once on the same handle.**\]**

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
