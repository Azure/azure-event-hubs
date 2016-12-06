# EventHubReceiver_LL Requirements
 
## Overview

The EventHubReceiver_LL (LL stands for Lower Layer) is module used for communication with an existing Event Hub. 
The EVENTHUBRECEIVER Lower Layer module makes use of uAMQP for AMQP communication with the Event Hub for the 
purposes of reading event data from a partition. 
This library is developed to provide similar usage to that provided via the EVENTHUBRECEIVER Class in .Net.
This library also doesn't use Thread or Locks, allowing it to be used in embedded applications that are 
limited in resources.

## References

[Event Hubs](http://msdn.microsoft.com/en-us/library/azure/dn789973.aspx)

[EVENTHUBRECEIVER Class for .NET](http://msdn.microsoft.com/en-us/library/microsoft.servicebus.messaging.EVENTHUBRECEIVER.aspx)

## Exposed API
```c
#define EVENTHUBRECEIVER_RESULT_VALUES                \
        EVENTHUBRECEIVER_OK,                          \
        EVENTHUBRECEIVER_INVALID_ARG,                 \
        EVENTHUBRECEIVER_ERROR,                       \
        EVENTHUBRECEIVER_TIMEOUT,                     \
        EVENTHUBRECEIVER_CONNECTION_RUNTIME_ERROR,    \
        EVENTHUBRECEIVER_NOT_ALLOWED

DEFINE_ENUM(EVENTHUBRECEIVER_RESULT, EVENTHUBRECEIVER_RESULT_VALUES);

typedef void(*EVENTHUBRECEIVER_ASYNC_CALLBACK)(EVENTHUBRECEIVER_RESULT result,
                                                EVENTDATA_HANDLE eventDataHandle,
                                                void* userContext);

typedef void(*EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK)(EVENTHUBRECEIVER_RESULT errorCode,
                                                        void* userContextCallback);

typedef struct EVENTHUBRECEIVER_LL_TAG* EVENTHUBRECEIVER_LL_HANDLE;

MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_LL_HANDLE, EventHubReceiver_LL_Create,
    const char*,  connectionString,
    const char*,  eventHubPath,
    const char*,  consumerGroup,
    const char*,  partitionId
);

MOCKABLE_FUNCTION(, void, EventHubReceiver_LL_Destroy, EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverHandle);

MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_ReceiveFromStartTimestampAsync,
	EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle, 
	EVENTHUBRECEIVER_ASYNC_CALLBACK, onEventReceiveCallback, 
	void*, onEventReceiveUserContext, 
	EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK, onEventReceiveErrorCallback,
	void*, onEventReceiveErrorUserContext,
	uint64_t, startTimestampInSec
);

MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync,
	EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle,
	EVENTHUBRECEIVER_ASYNC_CALLBACK, onEventReceiveCallback,
	void*, onEventReceiveUserContext,
	EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK, onEventReceiveErrorCallback,
	void*, onEventReceiveErrorUserContext,
        uint64_t, startTimestampInSec,
        unsigned int, waitTimeoutInMs
);

MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_ReceiveEndAsync,
					EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle,
					EVENTHUBRECEIVER_ASYNC_END_CALLBACK, onEventReceiveEndCallback,
					void*, onEventReceiveEndUserContext);

MOCKABLE_FUNCTION(, void, EventHubReceiver_LL_DoWork, EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle);

MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_SetConnectionTracing,
                    EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle, bool, traceEnabled);
                    
```																
### EventHubReceiver_LL_Create

```c
MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_LL_HANDLE, EventHubReceiver_LL_Create,
    const char*,  connectionString,
    const char*,  eventHubPath,
    const char*,  consumerGroup,
    const char*,  partitionId
);
```
**SRS_EVENTHUBRECEIVER_LL_29_111: \[**`EventHubReceiver_LL_Create` shall return NULL if any parameter connectionString, eventHubPath, consumerGroup and partitionId is NULL.**\]** 
**SRS_EVENTHUBRECEIVER_LL_29_101: \[**`EventHubReceiver_LL_Create` shall obtain the version string by a call to EVENTHUBRECEIVER_GetVersionString.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_102: \[**`EventHubReceiver_LL_Create` shall print the version string to standard output.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_103: \[**`EventHubReceiver_LL_Create` shall expect a service bus connection string in one of the following formats:
Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[keyname];SharedAccessKey=[keyvalue] 
Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[keyname];SharedAccessKey=[keyvalue]
**\]**
**SRS_EVENTHUBRECEIVER_LL_29_104: \[**The connection string shall be parsed to a map of strings by using connection_string_parser_parse.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_105: \[**If connection_string_parser_parse fails then `EventHubReceiver_LL_Create` shall fail and return NULL.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_106: \[**The endpoint shall be looked up in the resulting map and used to construct the host name to be used for connecting by removing the sb://.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_107: \[**The key name and key shall be looked up in the resulting map and they should be stored as is for later use in connecting.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_108: \[**Initialize receiver host address using the supplied connection string parameters host name, domain suffix, event hub name. **\]**
**SRS_EVENTHUBRECEIVER_LL_29_109: \[**Initialize receiver partition target address using consumer group and partition id data with format {eventHubName}/ConsumerGroups/{consumerGroup}/Partitions/{partitionID}.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_112: \[**`EventHubReceiver_LL_Create` shall return a non-NULL handle value upon success.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_114: \[**`EventHubReceiver_LL_Create` shall allocate a new event hub receiver LL instance, initialize it with the user supplied data.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_115: \[**For all other errors, `EventHubReceiver_LL_Create` shall return NULL.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_116: \[**Initialize connection tracing to false by default.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_117: \[**`EventHubReceiver_LL_Create` shall create a tickcounter.**\]**

### EventHubReceiver_LL_Destroy
```c
MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_Destroy, EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverHandle);
```
**SRS_EVENTHUBRECEIVER_LL_29_201: \[**`EventHubReceiver_LL_Destroy` shall tear down connection with the event hub.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_202: \[**`EventHubReceiver_LL_Destroy` shall terminate the usage of the EVENTHUBRECEIVER_LL_STRUCT and cleanup all associated resources.**\]**

### EventHubReceiver_LL_ReceiveFromStartTimestampAsync and EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync
```c
MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_ReceiveFromStartTimestampAsync,
	EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle, 
	EVENTHUBRECEIVER_ASYNC_CALLBACK, onEventReceiveCallback, 
	void*, onEventReceiveUserContext, 
	EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK, onEventReceiveErrorCallback,
	void*, onEventReceiveErrorUserContext,
	uint64_t, startTimestampInSec
);

MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync,
	EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle,
	EVENTHUBRECEIVER_ASYNC_CALLBACK, onEventReceiveCallback,
	void*, onEventReceiveUserContext,
	EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK, onEventReceiveErrorCallback,
	void*, onEventReceiveErrorUserContext,
        uint64_t, startTimestampInSec,
        unsigned int, waitTimeoutInMs
);
```
**SRS_EVENTHUBRECEIVER_LL_29_301: \[**`EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` shall fail and return EVENTHUBRECEIVER_INVALID_ARG if parameter eventHubReceiverHandle, onEventReceiveErrorCallback, onEventReceiveErrorCallback are NULL.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_302: \[**`EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` shall record the callbacks and contexts in the EVENTHUBRECEIVER_LL_STRUCT.**\]** 
**SRS_EVENTHUBRECEIVER_LL_29_303: \[**If cloning and/or adding the information fails for any reason, `EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` shall fail and return EVENTHUBRECEIVER_ERROR.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_304: \[**Create a filter string using format "apache.org:selector-filter:string" and "amqp.annotation.x-opt-enqueuedtimeutc > startTimestampInSec" using STRING_sprintf**\]**
**SRS_EVENTHUBRECEIVER_LL_29_305: \[**If filter string create fails then a log message will be logged and an error code of EVENTHUBRECEIVER_ERROR shall be returned.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_306: \[**Initialize timeout value (zero if no timeout) and a current timestamp of now.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_307: \[**`EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` shall return an error code of EVENTHUBRECEIVER_NOT_ALLOWED if a user called EventHubReceiver_LL_Receive* more than once on the same handle.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_308: \[**Otherwise `EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` shall succeed and return EVENTHUBRECEIVER_OK.**\]**

### EventHubReceiver_LL_DoWork
```c
MOCKABLE_FUNCTION(, void, EventHubReceiver_LL_DoWork, EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle);
```
#### Parameter Verification
**SRS_EVENTHUBRECEIVER_LL_29_450: \[**`EventHubReceiver_LL_DoWork` shall return immediately if the supplied handle is NULL**\]**

#### General
**SRS_EVENTHUBRECEIVER_LL_29_451: \[**`EventHubReceiver_LL_DoWork` shall initialize and bring up the uAMQP stack if it has not already brought up**\]** 
**SRS_EVENTHUBRECEIVER_LL_29_452: \[**`EventHubReceiver_LL_DoWork` shall create a message receiver if not aleady created**\]** 
**SRS_EVENTHUBRECEIVER_LL_29_453: \[**`EventHubReceiver_LL_DoWork` shall invoke connection_dowork **\]** 
**SRS_EVENTHUBRECEIVER_LL_29_454: \[**`EventHubReceiver_LL_DoWork` shall manage timeouts as long as the user specified timeout value is non zero **\]** 

#### AMQP Stack Initialize
**SRS_EVENTHUBRECEIVER_LL_29_501: \[**A SASL plain mechanism shall be created by calling saslmechanism_create.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_502: \[**The interface passed to saslmechanism_create shall be obtained by calling saslplain_get_interface.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_503: \[**The authcid shall be set to the key name parsed from the connection string.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_504: \[**The passwd members shall be set to the key value from the connection string.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_505: \[**If saslplain_get_interface fails then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_506: \[**The creation parameters for the SASL plain mechanism shall be in the form of a SASL_PLAIN_CONFIG structure.**\]**

**SRS_EVENTHUBRECEIVER_LL_29_520: \[**A TLS IO shall be created by calling xio_create using TLS port 5671 and host name obtained from the connection string**\]**
**SRS_EVENTHUBRECEIVER_LL_29_521: \[**The interface passed to xio_create shall be obtained by calling platform_get_default_tlsio.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_522: \[**If xio_create fails then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_523: \[**A SASL IO shall be created by calling xio_create using TLS IO interface created previously and the SASL plain mechanism created earlier. The SASL interface shall be obtained using `saslclientio_get_interface_description`**\]**
**SRS_EVENTHUBRECEIVER_LL_29_524: \[**If xio_create fails then a log message will be logged and the function returns immediately.**\]**

**SRS_EVENTHUBRECEIVER_LL_29_530: \[**An AMQP connection shall be created by calling connection_create and passing as arguments the SASL client IO handle created previously, hostname, connection name and NULL for the new session handler end point and context.**\]** 
**SRS_EVENTHUBRECEIVER_LL_29_531: \[**If connection_create fails then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_532: \[**Connection tracing shall be called with the current value of the tracing flag**\]**

**SRS_EVENTHUBRECEIVER_LL_29_540: \[**An AMQP session shall be created by calling session_create and passing as arguments the connection handle, and NULL for the new link handler and context.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_541: \[**If session_create fails then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_542: \[**Configure the session incoming window by calling session_set_incoming_window and set value to INTMAX.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_543: \[**A filter_set shall be created and initialized using key "apache.org:selector-filter:string" and value as the query filter created previously.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_544: \[**If creation of the filter_set fails, then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_545: \[**If a failure is observed during source creation and initialization, then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_546: \[**If messaging_create_target fails, then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_547: \[**The message receiver 'source' shall be created using source_create.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_550: \[**An AMQP link for the to be created message receiver shall be created by calling link_create with role as role_receiver and name as "receiver-link"**\]**
**SRS_EVENTHUBRECEIVER_LL_29_551: \[**The message receiver link 'source' shall be created using API amqpvalue_create_source**\]**
**SRS_EVENTHUBRECEIVER_LL_29_552: \[**The message receiver link 'source' filter shall be initialized by calling source_set_filter and using the filter_set created earlier.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_553: \[**The message receiver link 'source' address shall be initialized using the partition target address created earlier.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_554: \[**The message receiver link target shall be created using messaging_create_target with address obtained from the partition target address created earlier.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_555: \[**If link_create fails then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_556: \[**Configure the link settle mode by calling link_set_rcv_settle_mode and set value to receiver_settle_mode_first.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_557: \[**If link_set_rcv_settle_mode fails then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_558: \[**The message size shall be set to 256K by calling link_set_max_message_size.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_559: \[**If link_set_max_message_size fails then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_560: \[**A message receiver shall be created by calling messagereceiver_create and passing as arguments the link handle, a state changed callback and context.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_561: \[**If messagereceiver_create fails then a log message will be logged and the function returns immediately.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_562: \[**The created message receiver shall be transitioned to OPEN by calling messagereceiver_open.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_563: \[**If messagereceiver_open fails then a log message will be logged and the function returns immediately.**\]**


##### AMQP Stack DeInitialize
**SRS_EVENTHUBRECEIVER_LL_29_919: \[**`EventHubReceiver_LL_DoWork` shall do the work to tear down the AMQP stack when a user had called `EventHubReceiver_LL_ReceiveEndAsync`.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_920: \[**All pending message data not reported to the calling client shall be freed by calling messagereceiver_close and messagereceiver_destroy.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_921: \[**The link shall be freed by calling link_destroy.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_922: \[**The session shall be freed by calling session_destroy.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_923: \[**The connection shall be freed by calling connection_destroy.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_924: \[**The SASL client IO shall be freed by calling xio_destroy.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_925: \[**The TLS IO shall be freed by calling xio_destroy.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_926: \[**The SASL plain mechanism shall be freed by calling saslmechanism_destroy.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_927: \[**The filter string shall be freed by STRING_delete.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_928: \[**Upon Success, `EventHubReceiver_LL_DoWork` shall invoke the onEventReceiveEndCallback along with onEventReceiveEndUserContext with result code EVENTHUBRECEIVER_OK.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_929: \[**Upon failure, `EventHubReceiver_LL_DoWork` shall invoke the onEventReceiveEndCallback along with onEventReceiveEndUserContext with result code EVENTHUBRECEIVER_ERROR.**\]**

##### Creation of the message receiver
**SRS_EVENTHUBRECEIVER_LL_29_570: \[**`EventHubReceiver_LL_DoWork` shall call messagereceiver_create and pass link created in SRS_EVENTHUBRECEIVER_LL_29_550 `EHR_LL_OnStateChanged` as the ON_MESSAGE_RECEIVER_STATE_CHANGED parameter and the EVENTHUBRECEIVER_LL_HANDLE as the callback context**\]**
**SRS_EVENTHUBRECEIVER_LL_29_571: \[**Once the message receiver is created, `EventHubReceiver_LL_DoWork` shall call messagereceiver_open and pass in `EHR_LL_OnMessageReceived` as the ON_MESSAGE_RECEIVED parameter and the EVENTHUBRECEIVER_LL_HANDLE as the callback context**\]**

##### EHR_LL_OnStateChanged
```c
static void EHR_LL_OnStateChanged(const void* context, MESSAGE_RECEIVER_STATE newState, MESSAGE_RECEIVER_STATE previousState)
```
**SRS_EVENTHUBRECEIVER_LL_29_600: \[**When `EHR_LL_OnStateChanged` is invoked, obtain the EventHubReceiverLL handle from the context and update the message receiver state with the new state received in the callback.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_601: \[**If the new state is MESSAGE_RECEIVER_STATE_ERROR, and previous state is not MESSAGE_RECEIVER_STATE_ERROR, `EHR_LL_OnStateChanged` shall invoke the user supplied error callback along with error callback context`**\]**


##### EHR_LL_OnMessageReceived
```c
static AMQP_VALUE EHR_LL_OnMessageReceived(const void* context, MESSAGE_HANDLE message)
```
**SRS_EVENTHUBRECEIVER_LL_29_700: \[**When `EHR_LL_OnMessageReceived` is invoked, message_get_body_amqp_data shall be called to obtain the data into a BINARY_DATA buffer.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_702: \[**`EHR_LL_OnMessageReceived` shall create a EVENT_DATA handle using EventData_CreateWithNewMemory and pass in the buffer data pointer and size as arguments.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_703: \[**`EHR_LL_OnMessageReceived` shall obtain the application properties using message_get_application_properties() and populate the EVENT_DATA handle map with these key value pairs.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_704: \[**`EHR_LL_OnMessageReceived` shall obtain event data specific properties using message_get_message_annotations() and populate the EVENT_DATA handle with these properties.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_701: \[**If any errors are seen `EHR_LL_OnMessageReceived` shall reject the incoming message by calling messaging_delivery_rejected() and return.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_701: \[**`EHR_LL_OnMessageReceived` shall invoke the user registered onMessageReceive callback with status code EVENTHUBRECEIVER_OK, the EVENT_DATA handle and the context passed in by the user.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_706: \[**After `EHR_LL_OnMessageReceived` invokes the user callback, messaging_delivery_accepted shall be called.**\]**

##### Timeout Management
**SRS_EVENTHUBRECEIVER_LL_29_750: \[**`EventHubReceiver_LL_DoWork` shall check if a message was received, if so, reset the last activity time to the current time i.e. now**\]** 
**SRS_EVENTHUBRECEIVER_LL_29_751: \[**If a message was not received, check if the time now minus the last activity time is greater than or equal to the user specified timeout. If greater, the user registered callback is invoked along with the user supplied context with status code EVENTHUBRECEIVER_TIMEOUT. Last activity time shall be updated to the current time i.e. now.**\]** 
**SRS_EVENTHUBRECEIVER_LL_29_753: \[**If at any point there is an error for a specific eventHubReceiverLLHandle, the user callback should be invoked along with the error code.**\]**


### EventHubReceiver_LL_SetConnectionTracing
```c
MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_SetConnectionTracing,
                    EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle, bool, traceEnabled);
```
**SRS_EVENTHUBRECEIVER_LL_29_800: \[**`EventHubReceiver_LL_SetConnectionTracing` shall fail and return EVENTHUBRECEIVER_INVALID_ARG if parameter eventHubReceiverLLHandle.**\]** 
**SRS_EVENTHUBRECEIVER_LL_29_801: \[**`EventHubReceiver_LL_SetConnectionTracing` shall save the value of tracingOnOff in eventHubReceiverLLHandle**\]**
**SRS_EVENTHUBRECEIVER_LL_29_802: \[**If an active connection has been setup, `EventHubReceiver_LL_SetConnectionTracing` shall be called with the value of connection_set_trace tracingOnOff**\]**
**SRS_EVENTHUBRECEIVER_LL_29_803: \[**`Upon success, EventHubReceiver_LL_SetConnectionTracing` shall return EVENTHUBRECEIVER_OK**\]**

### 
```c
MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_ReceiveEndAsync,
					EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle,
					EVENTHUBRECEIVER_ASYNC_END_CALLBACK, onEventReceiveEndCallback,
					void*, onEventReceiveEndUserContext);
```
**SRS_EVENTHUBRECEIVER_LL_29_900: \[**`EventHubReceiver_LL_ReceiveEndAsync` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_901: \[**`EventHubReceiver_LL_ReceiveEndAsync` shall check if a receiver connection is currently active. If no receiver is active, EVENTHUBRECEIVER_NOT_ALLOWED shall be returned and a message will be logged.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_902: \[**`EventHubReceiver_LL_ReceiveEndAsync` save off the user callback and context and defer the UAMQP stack tear down to `EventHubReceiver_LL_DoWork`.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_903: \[**Upon Success, `EventHubReceiver_LL_ReceiveEndAsync` shall return EVENTHUBRECEIVER_OK.**\]**
**SRS_EVENTHUBRECEIVER_LL_29_904: \[**Upon failure, `EventHubReceiver_LL_ReceiveEndAsync` shall return EVENTHUBRECEIVER_ERROR and a message will be logged.**\]**