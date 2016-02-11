#EventHubClient Requirements
 
##Overview

The EventHubClient is a native library used for communication with an existing Event Hub.
The EventHubClient module is simply a convenience wrapper on top of the EventHubClient_LL module.

##References

Event Hubs [http://msdn.microsoft.com/en-us/library/azure/dn789973.aspx]
EventHubClient Class for .net [http://msdn.microsoft.com/en-us/library/microsoft.servicebus.messaging.eventhubclient.aspx]

##Exposed API

```C
typedef void* EVENTHUBCLIENT_HANDLE;
 
extern const char* EventHubClient_GetVersionString(void);
extern EVENTHUBCLIENT_HANDLE EventHubClient_CreateFromConnectionString(const char* connectionString, const char* eventHubPath);
extern EVENTHUBCLIENT_RESULT EventHubClient_Send(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE eventDataHandle);
extern EVENTHUBCLIENT_RESULT EventHubClient_SendAsync(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE eventDataHandle, EVENTDATA_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK notificatoinCallback, void* userContextCallback);
extern EVENTHUBCLIENT_RESULT EventHubClient_SendBatch(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE *eventDataHandle, size_t count);
extern EVENTHUBCLIENT_RESULT EventHubClient_SendBatchAsync(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE *eventDataHandle, size_t count, EVENTDATA_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncCallback, void* userContextCallback);
extern void EventHubClient_Destroy(EVENTHUBCLIENT_HANDLE eventHubHandle);
```

###EventHubClient_GetVersionString

```C
extern const char* EventHubClient_GetVersionString(void);
```

**SRS_EVENTHUBCLIENT_05_003: \[**EventHubClient_GetVersionString shall return a pointer to a constant string which indicates the version of EventHubClient API.**\]** 

###EventHubClient_CreateFromConnectionString

```C
extern EVENTHUBCLIENT_HANDLE EventHubClient_CreateFromConnectionString(const char* connectionString, const char* eventHubPath);
```

**SRS_EVENTHUBCLIENT_03_002: \[**Upon Success of EventHubClient_CreateFromConnectionString_LL, EventHubClient_CreateFromConnectionString shall allocate the internal structures required by this module.**\]**
**SRS_EVENTHUBCLIENT_03_004: \[**EventHubClient_CreateFromConnectionString shall pass the connectionString and eventHubPath variables to EventHubClient_CreateFromConnectionString_LL.**\]** 
**SRS_EVENTHUBCLIENT_03_005: \[**Upon Success EventHubClient_CreateFromConnectionString shall return the EVENTHUBCLIENT_HANDLE.**\]** 
**SRS_EVENTHUBCLIENT_03_006: \[**EventHubClient_CreateFromConnectionString shall return a NULL value if EventHubClient_CreateFromConnectionString_LL returns NULL.**\]**

###Execute_LowerLayerSendAsync

```C
extern int Execute_LowerLayerSendAsync(EVENTHUBCLIENT_STRUCT* eventHubClientInfo, EVENTDATA_HANDLE eventDataHandle, EVENTDATA_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK notificiationCallback, void* userContextCallback);
```

**SRS_EVENTHUBCLIENT_07_029: \[**Execute_LowerLayerSendAsync shall Lock on the EVENTHUBCLIENT_STRUCT lockInfo to protect calls to Lower Layer and Thread function calls.**\]** 
**SRS_EVENTHUBCLIENT_07_030: \[**Execute_LowerLayerSendAsync shall return a nonzero value if it is unable to obtain the lock with the Lock function.**\]** 
**SRS_EVENTHUBCLIENT_07_031: \[**Execute_LowerLayerSendAsync shall call into the Create_DoWorkThreadIfNeccesary function to create the DoWork thread.**\]** 
**SRS_EVENTHUBCLIENT_07_032: \[**If Create_DoWorkThreadIfNeccesary does not return 0 then Execute_LowerLayerSendAsync shall return a nonzero value.**\]** 
**SRS_EVENTHUBCLIENT_07_028: \[**If Execute_LowerLayerSendAsync is successful then it shall return 0.**\]**
**SRS_EVENTHUBCLIENT_07_038: \[**Execute_LowerLayerSendAsync shall call EventHubClient_LL_SendAsync to send data to the Eventhub Endpoint.**\]** **SRS_EVENTHUBCLIENT_07_039: \[**If the EventHubClient_LL_SendAsync call fails then Execute_LowerLayerSendAsync shall return a nonzero value.**\]** 

###Execute_LowerLayerSendBatchAsync

```C
extern int Execute_LowerLayerSendAsync(EVENTHUBCLIENT_STRUCT* eventHubClientInfo, EVENTDATA_HANDLE* eventDataList, size_t count, EVENTDATA_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncCallback, void* userContextCallback);
```

**SRS_EVENTHUBCLIENT_07_043: \[**Execute_LowerLayerSendBatchAsync shall Lock on the EVENTHUBCLIENT_STRUCT lockInfo to protect calls to Lower Layer and Thread function calls.**\]** 
**SRS_EVENTHUBCLIENT_07_044: \[**Execute_LowerLayerSendBatchAsync shall return a nonzero value if it is unable to obtain the lock with the Lock function.**\]** 
**SRS_EVENTHUBCLIENT_07_045: \[**Execute_LowerLayerSendAsync shall call into the Create_DoWorkThreadIfNeccesary function to create the DoWork thread.**\]** 
**SRS_EVENTHUBCLIENT_07_046: \[**If Create_DoWorkThreadIfNeccesary does not return 0 then Execute_LowerLayerSendAsync shall return a nonzero value.**\]** 
**SRS_EVENTHUBCLIENT_07_047: \[**If Execute_LowerLayerSendAsync is successful then it shall return 0.**\]** 
**SRS_EVENTHUBCLIENT_07_048: \[**Execute_LowerLayerSendAsync shall call EventHubClient_LL_SendAsync to send data to the Eventhub Endpoint.**\]**
**SRS_EVENTHUBCLIENT_07_049: \[**If the EventHubClient_LL_SendAsync call fails then Execute_LowerLayerSendAsync shall return a nonzero value.**\]** 

###Create_DoWorkThreadIfNeccesary
```C
extern int Create_DoWorkThreadIfNeccesary (EVENTHUBCLIENT_STRUCT* eventHubClientInfo);
```

**SRS_EVENTHUBCLIENT_07_033: \[**Create_DoWorkThreadIfNeccesary shall set return 0 if threadHandle parameter is not a NULL value.**\]** 
**SRS_EVENTHUBCLIENT_07_034: \[**Create_DoWorkThreadIfNeccesary shall use the ThreadAPI_Create API to create a thread and execute EventhubClientThread function.**\]** 
**SRS_EVENTHUBCLIENT_07_035: \[**Create_DoWorkThreadIfNeccesary shall return a nonzero value if any failure is encountered.**\]** 

###EventHubClient_Send
```C
extern EVENTHUBCLIENT_RESULT EventHubClient_Send(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE eventDataHandle);
```

EventHubClient_Send shall call into EventHubClient_LL_SendAsync to send content to EventHub by means of the following steps: 
**SRS_EVENTHUBCLIENT_03_007: \[**EventHubClient_Send shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL.**\]**
**SRS_EVENTHUBCLIENT_03_008: \[**EventHubClient_Send shall call into the Execute_LowerLayerSendAsync function to send the eventDataHandle parameter to the EventHub.**\]**
**SRS_EVENTHUBCLIENT_03_009: \[**EventHubClient_Send shall return EVENTHUBCLIENT_ERROR on any failure that is encountered.**\]** 
**SRS_EVENTHUBCLIENT_03_010: \[**Upon success of Execute_LowerLayerSendAsync, then EventHubClient_Send wait until the EVENTHUB_CALLBACK_STRUCT callbackStatus variable is set to CALLBACK_NOTIFIED.**\]** 
**SRS_EVENTHUBCLIENT_07_012: \[**EventHubClient_Send shall return EVENTHUBCLIENT_ERROR if the EVENTHUB_CALLBACK_STRUCT confirmationResult variable does not equal EVENTHUBCLIENT_CONFIMRATION_OK.**\]**
**SRS_EVENTHUBCLIENT_03_013: \[**EventHubClient_Send shall return EVENTHUBCLIENT_OK upon successful completion of the Execute_LowerLayerSendAsync and the callback function.**\]** 

###EventHubClient_SendAsync
```C
extern EVENTHUBCLIENT_RESULT EventHubClient_SendAsync(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE eventDataHandle, EVENTDATA_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncCallback, void* userContextCallback);
```
**SRS_EVENTHUBCLIENT_03_021: \[**EventHubClient_SendAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL.**\]**
**SRS_EVENTHUBCLIENT_07_022: \[**EventHubClient_SendAsync shall call into Execute_LowerLayerSendAsync and return EVENTHUBCLIENT_ERROR on a nonzero return value.**\]** 
**SRS_EVENTHUBCLIENT_07_037: \[**On Success EventHubClient_SendAsync shall return EVENTHUBCLIENT_OK.**\]** 

###EventHubClient_SendBatch
```C
extern EVENTHUBCLIENT_RESULT EventHubClient_SendBatch(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE* eventDataList);
```

EventHubClient_SendBatch shall call into EventHubClient_LL_SendBatchAsync to send content to EventHub by means of the following steps: 
**SRS_EVENTHUBCLIENT_07_050: \[**EventHubClient_SendBatch shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL.**\]** 
**SRS_EVENTHUBCLIENT_07_051: \[**EventHubClient_SendBatch shall call into the Execute_LowerLayerSendBatchAsync function to send the eventDataHandle parameter to the EventHub.**\]** 
**SRS_EVENTHUBCLIENT_07_052: \[**EventHubClient_SendBatch shall return EVENTHUBCLIENT_ERROR on any failure that is encountered.**\]** 
**SRS_EVENTHUBCLIENT_07_053: \[**Upon success of Execute_LowerLayerSendBatchAsync, then EventHubClient_SendBatch shall wait until the EVENTHUB_CALLBACK_STRUCT callbackStatus variable is set to CALLBACK_NOTIFIED.**\]** 
**SRS_EVENTHUBCLIENT_07_054: \[**EventHubClient_SendBatch shall return EVENTHUBCLIENT_OK upon successful completion of the Execute_LowerLayerSendBatchAsync and the callback function.**\]** 

###EventHubClient_SendBatchAsync
```C
extern EVENTHUBCLIENT_RESULT EventHubClient_SendBatchAsync(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE* eventDataList, size_t count, EVENTDATA_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncCallback, void* userContextCallback);
```

**SRS_EVENTHUBCLIENT_07_040: \[**EventHubClient_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL or count is zero.**\]** 
**SRS_EVENTHUBCLIENT_07_041: \[**EventHubClient_SendBatchAsync shall call into Execute_LowerLayerSendBatchAsync and return EVENTHUBCLIENT_ERROR on a nonzero return value.**\]** 
**SRS_EVENTHUBCLIENT_07_042: \[**On Success EventHubClient_SendBatchAsync shall return EVENTHUBCLIENT_OK.**\]** 

###EventHubClient_Destroy
```C
extern void EventHubClient_Destroy(EVENTHUBCLIENT_HANDLE eventHubHandle);
```

**SRS_EVENTHUBCLIENT_03_019: \[**EventHubClient_Destroy shall terminate the usage of this EventHubClient specified by the eventHubHandle and cleanup all associated resources.**\]** 
**SRS_EVENTHUBCLIENT_03_018: \[**If the eventHubHandle is NULL, EventHubClient_Destroy shall not do anything.**\]**
**SRS_EVENTHUBCLIENT_03_020: \[**EventHubClient_Destroy shall call EventHubClient_LL_Destroy with the lower level handle.**\]**

