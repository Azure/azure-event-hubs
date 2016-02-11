#EventHubClient_LL Requirements
 
##Overview

The EventHubClient_LL (LL stands for Lower Layer) is module used for communication with an existing Event Hub. The EventHubClient Lower Layer module makes use of uAMQP for AMQP communication with the Event Hub. This library is developed to provide similar usage to that provided via the EventHubClient Class in .Net. This library also doesn’t use Thread or Lock, allowing it to be used in embedded applications that are limited as resources. 

##References

Event Hubs [http://msdn.microsoft.com/en-us/library/azure/dn789973.aspx]
EventHubClient Class for .net [http://msdn.microsoft.com/en-us/library/microsoft.servicebus.messaging.eventhubclient.aspx]

##Exposed API

```c
#define EVENTHUBCLIENT_RESULT_VALUES            \
    EVENTHUBCLIENT_OK,                          \
    EVENTHUBCLIENT_INVALID_ARG,                 \
    EVENTHUBCLIENT_INVALID_CONNECTION_STRING,   \
    EVENTHUBCLIENT_URL_ENCODING_FAILURE,        \
    EVENTHUBCLIENT_EVENT_DATA_FAILURE,          \
    EVENTHUBCLIENT_PARTITION_KEY_MISMATCH,      \
    EVENTHUBCLIENT_DATA_SIZE_EXCEEDED,          \
    EVENTHUBCLIENT_ERROR                        \

DEFINE_ENUM(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_RESULT_VALUES);

#define EVENTHUBCLIENT_CONFIRMATION_RESULT_VALUES     \
    EVENTHUBCLIENT_CONFIRMATION_OK,                   \
    EVENTHUBCLIENT_CONFIRMATION_DESTROY,              \
    EVENTHUBCLIENT_CONFIRMATION_EXCEED_MAX_SIZE,      \
    EVENTHUBCLIENT_CONFIRMATION_UNKNOWN,              \
    EVENTHUBCLIENT_CONFIRMATION_ERROR                 \

DEFINE_ENUM(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_RESULT_VALUES);

typedef struct EVENTHUBCLIENT_LL_TAG* EVENTHUBCLIENT_LL_HANDLE;
typedef void(*EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK)(EVENTHUBCLIENT_CONFIRMATION_RESULT result, void* userContextCallback);

extern EVENTHUBCLIENT_LL_HANDLE EventHubClient_LL_CreateFromConnectionString(const char* connectionString, const char* eventHubPath);

extern EVENTHUBCLIENT_RESULT EventHubClient_LL_SendAsync(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle, EVENTDATA_HANDLE eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncConfirmationCallback, void* userContextCallback);
extern EVENTHUBCLIENT_RESULT EventHubClient_LL_SendBatchAsync(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle, EVENTDATA_HANDLE* eventDataList, size_t count, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncConfirmationCallback, void* userContextCallback);

extern void EventHubClient_LL_DoWork(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle);

extern void EventHubClient_LL_Destroy(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle);
```

###EventHubClient_LL_CreateFromConnectionString

```c
extern EVENTHUBCLIENT_LL_HANDLE EventHubClient_LL_CreateFromConnectionString(const char* connectionString, const char* eventHubPath);
```
**SRS_EVENTHUBCLIENT_LL_03_002: \[**EventHubClient_LL_CreateFromConnectionString shall allocate a new event hub client LL instance.**\]**
**SRS_EVENTHUBCLIENT_LL_05_001: \[**EventHubClient_LL_CreateFromConnectionString shall obtain the version string by a call to EventHubClient_GetVersionString.**\]**
**SRS_EVENTHUBCLIENT_LL_05_002: \[**EventHubClient_LL_CreateFromConnectionString shall print the version string to standard output.**\]**
**SRS_EVENTHUBCLIENT_LL_03_017: \[**EventHubClient_LL expects a service bus connection string in one of the following formats:
Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[keyname];SharedAccessKey=[keyvalue] 
Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[keyname];SharedAccessKey=[keyvalue]
**\]**
**SRS_EVENTHUBCLIENT_LL_03_018: \[**EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.**\]**
**SRS_EVENTHUBCLIENT_LL_01_065: \[**The connection string shall be parsed to a map of strings by using connection_string_parser_parse.**\]**
**SRS_EVENTHUBCLIENT_LL_01_066: \[**If connection_string_parser_parse fails then EventHubClient_LL_CreateFromConnectionString shall fail and return NULL.**\]**
**SRS_EVENTHUBCLIENT_LL_01_067: \[**The endpoint shall be looked up in the resulting map and used to construct the host name to be used for connecting by removing the sb://.**\]**
**SRS_EVENTHUBCLIENT_LL_01_068: \[**The key name and key shall be looked up in the resulting map and they should be stored as is for later use in connecting.**\]** 
**SRS_EVENTHUBCLIENT_LL_03_016: \[**EventHubClient_LL_CreateFromConnectionString shall return a non-NULL handle value upon success.**\]**
**SRS_EVENTHUBCLIENT_LL_03_003: \[**EventHubClient_LL_CreateFromConnectionString shall return a NULL value if connectionString or eventHubPath is NULL.**\]**
**SRS_EVENTHUBCLIENT_LL_04_016: \[**EventHubClient_LL_CreateFromConnectionString shall initialize the pending list that will be used to send Events.**\]** 
**SRS_EVENTHUBCLIENT_LL_03_004: \[**For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.**\]**

###EventHubClient_LL_SendAsync

```c
extern EVENTHUBCLIENT_RESULT EventHubClient_LL_SendAsync(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle , EVENTDATA_HANDLE eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncConfirmationCallback, void* userContextCallback);
```

**SRS_EVENTHUBCLIENT_LL_04_011: \[**EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_LL_INVALID_ARG if parameter eventHubClientLLHandle or eventDataHandle is NULL.**\]** 
**SRS_EVENTHUBCLIENT_LL_04_012: \[**EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_LL_INVALID_ARG if parameter telemetryConfirmationCallBack is NULL and userContextCallBack is not NULL.**\]** 
**SRS_EVENTHUBCLIENT_LL_04_013: \[**EventHubClient_LL_SendAsync shall add to the pending events DLIST outgoingEvents a new record cloning the information from eventDataHandle, telemetryConfirmationCallback and userContextCallBack.**\]** 
**SRS_EVENTHUBCLIENT_LL_04_014: \[**If cloning and/or adding the information fails for any reason, EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_LL_ERROR.**\]**
**SRS_EVENTHUBCLIENT_LL_04_015: \[**Otherwise EventHubClient_LL_SendAsync shall succeed and return EVENTHUBCLIENT_LL_OK.**\]** 

###EventHubClient_LL_SendBatchAsync

```c
extern EVENTHUBCLIENT_RESULT EventHubClient_LL_SendBatchAsync(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle, EVENTDATA_HANDLE* eventDataList, size_t count,  EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncConfirmationCallback, void* userContextCallback);
```

**SRS_EVENTHUBCLIENT_LL_07_012: \[**EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.**\]**
**SRS_EVENTHUBCLIENT_LL_01_095: \[**EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if the count argument is zero.**\]** 
**SRS_EVENTHUBCLIENT_LL_07_013: \[**EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_096: \[**If the partitionKey properties on the events in the batch are not the same then EventHubClient_LL_SendBatchAsync shall fail and return EVENTHUBCLIENT_ERROR.**\]**
**SRS_EVENTHUBCLIENT_LL_01_097: \[**The partition key for each event shall be obtained by calling EventData_getPartitionKey.**\]** 
**SRS_EVENTHUBCLIENT_LL_07_014: \[**EventHubClient_LL_SendBatchAsync shall clone each item in the eventDataList by calling EventData_Clone.**\]** 
**SRS_EVENTHUBCLIENT_LL_07_015: \[**On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.**\]** 

###EventHubClient_LL_DoWork

```c
extern void EventHubClient_LL_DoWork(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle);
```

**SRS_EVENTHUBCLIENT_LL_01_079: \[**EventHubClient_LL_DoWork shall bring up the uAMQP stack if it has not already brought up:**\]** 
**SRS_EVENTHUBCLIENT_LL_01_004: \[**A SASL plain mechanism shall be created by calling saslmechanism_create.**\]**
**SRS_EVENTHUBCLIENT_LL_01_005: \[**The interface passed to saslmechanism_create shall be obtained by calling saslplain_get_interface.**\]**
**SRS_EVENTHUBCLIENT_LL_01_006: \[**If saslplain_get_interface fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.**\]**
**SRS_EVENTHUBCLIENT_LL_01_007: \[**The creation parameters for the SASL plain mechanism shall be in the form of a SASL_PLAIN_CONFIG structure.**\]**
**SRS_EVENTHUBCLIENT_LL_01_008: \[**The authcid shall be set to the key name parsed earlier from the connection string.**\]**
**SRS_EVENTHUBCLIENT_LL_01_009: \[**The passwd members shall be set to the key value parsed earlier from the connection string.**\]**
**SRS_EVENTHUBCLIENT_LL_01_010: \[**The authzid shall be NULL.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_011: \[**If sasl_mechanism_create fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_03_030: \[**A TLS IO shall be created by calling xio_create.**\]** **SRS_EVENTHUBCLIENT_LL_01_002: \[**The TLS IO interface description passed to xio_create shall be obtained by calling platform_get_default_tlsio_interface.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_001: \[**If platform_get_default_tlsio_interface fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages. **\]**
**SRS_EVENTHUBCLIENT_LL_01_003: \[**If xio_create fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_012: \[**A SASL client IO shall be created by calling xio_create.**\]**
**SRS_EVENTHUBCLIENT_LL_01_013: \[**The IO interface description for the SASL client IO shall be obtained by calling saslclientio_get_interface_description.**\]**
**SRS_EVENTHUBCLIENT_LL_01_014: \[**If saslclientio_get_interface_description fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.**\]**
**SRS_EVENTHUBCLIENT_LL_01_015: \[**The IO creation parameters passed to xio_create shall be in the form of a SASLCLIENTIO_CONFIG.**\]**
**SRS_EVENTHUBCLIENT_LL_01_016: \[**The underlying_io members shall be set to the previously created TLS IO.**\]**
**SRS_EVENTHUBCLIENT_LL_01_017: \[**The sasl_mechanism shall be set to the previously created SASL PLAIN mechanism.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_018: \[**If xio_create fails creating the SASL client IO then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_019: \[**An AMQP connection shall be created by calling connection_create and passing as arguments the SASL client IO handle, eventhub hostname, “eh_client_connection” as container name and NULL for the new session handler and context.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_020: \[**If connection_create fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_028: \[**An AMQP session shall be created by calling session_create and passing as arguments the connection handle, and NULL for the new link handler and context.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_029: \[**If session_create fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_030: \[**The outgoing window for the session shall be set to 10 by calling session_set_outgoing_window.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_031: \[**If setting the outgoing window fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_021: \[**A source AMQP value shall be created by calling messaging_create_source.**\]**
**SRS_EVENTHUBCLIENT_LL_01_022: \[**The source address shall be “ingress”.**\]** **SRS_EVENTHUBCLIENT_LL_01_023: \[**A target AMQP value shall be created by calling messaging_create_target.**\]**
**SRS_EVENTHUBCLIENT_LL_01_024: \[**The target address shall be “amqps://” {eventhub hostname} / {eventhub name}.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_025: \[**If creating the source or target values fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_026: \[**An AMQP link shall be created by calling link_create and passing as arguments the session handle, “sender-link” as link name, role_sender and the previously created source and target values.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_027: \[**If creating the link fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_032: \[**The link sender settle mode shall be set to unsettled by calling link_set_snd_settle_mode.**\]**
**SRS_EVENTHUBCLIENT_LL_01_033: \[**If link_set_snd_settle_mode fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_034: \[**The message size shall be set to 256K by calling link_set_max_message_size.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_035: \[**If link_set_max_message_size fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_036: \[**A message sender shall be created by calling messagesender_create and passing as arguments the link handle, a state changed callback, a context and NULL for the logging function.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_037: \[**If creating the message sender fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_080: \[**If any other error happens while bringing up the uAMQP stack, EventHubClient_LL_DoWork shall not attempt to open the message_sender and return without sending any messages.**\]** 

**SRS_EVENTHUBCLIENT_LL_04_018: \[**if parameter eventHubClientLLHandle is NULL EventHubClient_LL_DoWork shall immediately return.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_038: \[**EventHubClient_LL_DoWork shall perform a messagesender_open if the state of the message_sender is not OPEN.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_039: \[**If messagesender_open fails, no further actions shall be carried out.**\]** 
For each message that is pending:
**SRS_EVENTHUBCLIENT_LL_01_049: \[**If the message has not yet been given to uAMQP then a new message shall be created by calling message_create.**\]** **SRS_EVENTHUBCLIENT_LL_01_070: \[**If creating the message fails, then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_050: \[**If the number of event data entries for the message is 1 (not batched) then the message body shall be set to the event data payload by calling message_add_body_amqp_data.**\]** **SRS_EVENTHUBCLIENT_LL_01_071: \[**If message_add_body_amqp_data fails then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.**\]** **SRS_EVENTHUBCLIENT_LL_01_051: \[**The pointer to the payload and its length shall be obtained by calling EventData_GetData.**\]** **SRS_EVENTHUBCLIENT_LL_01_052: \[**If EventData_GetData fails then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_054: \[**If the number of event data entries for the message is 1 (not batched) the event data properties shall be added as application properties to the message.**\]** **SRS_EVENTHUBCLIENT_LL_01_055: \[**A map shall be created to hold the application properties by calling amqpvalue_create_map.**\]** **SRS_EVENTHUBCLIENT_LL_01_056: \[**For each property a key and value AMQP value shall be created by calling amqpvalue_create_string.**\]** **SRS_EVENTHUBCLIENT_LL_01_057: \[**Then each property shall be added to the application properties map by calling amqpvalue_set_map_value.**\]** **SRS_EVENTHUBCLIENT_LL_01_058: \[**The resulting map shall be set as the message application properties by calling message_set_application_properties.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_059: \[**If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_082: \[**If the number of event data entries for the message is greater than 1 (batched) then the message format shall be set to 0x80013700 by calling message_set_message_format.**\]** **SRS_EVENTHUBCLIENT_LL_01_083: \[**If message_set_message_format fails, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_084: \[**For each event in the batch:**\]** 
**SRS_EVENTHUBCLIENT_LL_01_085: \[**The event shall be added to the message by into a separate data section by calling message_add_body_amqp_data.**\]** **SRS_EVENTHUBCLIENT_LL_01_086: \[**The buffer passed to message_add_body_amqp_data shall contain the properties and the binary event payload serialized as AMQP values.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_087: \[**The properties shall be serialized as AMQP application_properties.**\]** **SRS_EVENTHUBCLIENT_LL_01_088: \[**The event payload shall be serialized as an AMQP message data section.**\]** **SRS_EVENTHUBCLIENT_LL_01_089: \[**If message_add_body_amqp_data fails, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_090: \[**Enough memory shall be allocated to hold the properties and binary payload for each event part of the batch.**\]** **SRS_EVENTHUBCLIENT_LL_01_091: \[**The size needed for the properties and data section shall be obtained by calling amqpvalue_get_encoded_size.**\]** **SRS_EVENTHUBCLIENT_LL_01_092: \[**The properties and binary data shall be encoded by calling amqpvalue_encode and passing an encoding function that places the encoded data into the memory allocated for the event.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_093: \[**If the property count is 0 for an event part of the batch, then no property map shall be serialized for that event.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_094: \[**If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_069: \[**The AMQP message shall be given to uAMQP by calling messagesender_send, while passing as arguments the message sender handle, the message handle, a callback function and its context.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_053: \[**If messagesender_send failed then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_064: \[**EventHubClient_LL_DoWork shall call connection_dowork while passing as argument the connection handle obtained in EventHubClient_LL_Create.**\]** 

###on_messagesender_state_changed

**SRS_EVENTHUBCLIENT_LL_01_060: \[**When on_messagesender_state_changed is called with MESSAGE_SENDER_STATE_ERROR, the uAMQP stack shall be brough down so that it can be created again if needed in dowork:**\]** 
-	**SRS_EVENTHUBCLIENT_LL_01_072: \[**The message sender shall be destroyed by calling messagesender_destroy.**\]** 
-	**SRS_EVENTHUBCLIENT_LL_01_073: \[**The link shall be destroyed by calling link_destroy.**\]** 
-	**SRS_EVENTHUBCLIENT_LL_01_074: \[**The session shall be destroyed by calling session_destroy.**\]** 
-	**SRS_EVENTHUBCLIENT_LL_01_075: \[**The connection shall be destroyed by calling connection_destroy.**\]** 
-	**SRS_EVENTHUBCLIENT_LL_01_076: \[**The SASL IO shall be destroyed by calling xio_destroy.**\]** 
-	**SRS_EVENTHUBCLIENT_LL_01_077: \[**The TLS IO shall be destroyed by calling xio_destroy.**\]** 
-	**SRS_EVENTHUBCLIENT_LL_01_078: \[**The SASL mechanism shall be destroyed by calling saslmechanism_destroy.**\]** 

###on_message_send_complete

**SRS_EVENTHUBCLIENT_LL_01_061: \[**When on_message_send_complete is called with MESSAGE_SEND_OK the pending message shall be indicated as sent correctly by calling the callback associated with the pending message with EVENTHUBCLIENT_CONFIRMATION_OK.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_063: \[**When on_message_send_complete is called with a result code different than MESSAGE_SEND_OK the pending message shall be indicated as having an error by calling the callback associated with the pending message with EVENTHUBCLIENT_CONFIRMATION_ERROR.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_062: \[**The pending message shall be removed from the pending list.**\]** 

###EventHubClient_LL_Destroy

```c
extern void EventHubClient_LL_Destroy(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle);
```

**SRS_EVENTHUBCLIENT_LL_03_009: \[**EventHubClient_LL_Destroy shall terminate the usage of this EventHubClient_LL specified by the eventHubLLHandle and cleanup all associated resources.**\]** **SRS_EVENTHUBCLIENT_LL_01_081: \[**The key host name, key name and key allocated in EventHubClient_LL_CreateFromConnectionString shall be freed.**\]** 
**SRS_EVENTHUBCLIENT_LL_03_010: \[**If the eventHubLLHandle is NULL, EventHubClient_Destroy shall not do anything.**\]**
**SRS_EVENTHUBCLIENT_LL_01_040: \[**All the pending messages shall be indicated as error by calling the associated callback with EVENTHUBCLIENT_CONFIRMATION_DESTROY.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_041: \[**All pending message data shall be freed.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_042: \[**The message sender shall be freed by calling messagesender_destroy.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_043: \[**The link shall be freed by calling link_destroy.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_044: \[**The session shall be freed by calling session_destroy.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_045: \[**The connection shall be freed by calling connection_destroy.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_046: \[**The SASL client IO shall be freed by calling xio_destroy.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_047: \[**The TLS IO shall be freed by calling xio_destroy.**\]** 
**SRS_EVENTHUBCLIENT_LL_01_048: \[**The SASL plain mechanism shall be freed by calling saslmechanism_destroy.**\]** 
