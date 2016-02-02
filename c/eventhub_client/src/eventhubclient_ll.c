// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "gballoc.h"

#include <string.h>
#include "eventhubclient_ll.h"
#include "iot_logging.h"
#include "map.h"
#include "connection_string_parser.h"
#include "strings.h"
#include "version.h"
#include "crt_abstractions.h"
#include "connection.h"
#include "session.h"
#include "link.h"
#include "xio.h"
#include "messaging.h"
#include "message_sender.h"
#include "saslclientio.h"
#include "sasl_plain.h"
#include "tlsio.h"
#include "platform.h"
#include "doublylinkedlist.h"

#define LOG_ERROR LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, result));
DEFINE_ENUM_STRINGS(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_RESULT_VALUES)

#define ALL_MESSAGES_IN_OUTGOING_QUEUE  -1
#define PROTON_PROPERTY_HEADER_SIZE     12
#define AMQP_PACK_OVERHEAD              20
#define AMQP_MAX_MESSAGE_SIZE           (64*1024)
#define SB_STRING_LENGTH                5   /* length for sb:// */

static const size_t OUTGOING_WINDOW_SIZE = 10;
static const size_t OUTGOING_WINDOW_BUFFER = 5;

typedef enum EVENTHUB_EVENT_STATUS_TAG
{
    WAITING_TO_BE_SENT = 0,
    WAITING_FOR_ACK
} EVENTHUB_EVENT_STATUS;

typedef struct EVENTHUB_EVENT_LIST_TAG
{
    EVENTDATA_HANDLE* eventDataList;
    size_t dataCount;
    EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK callback;
    void* context;
    EVENTHUB_EVENT_STATUS currentStatus;
    DLIST_ENTRY entry;
} EVENTHUB_EVENT_LIST, *PEVENTHUB_EVENT_LIST;

typedef struct EVENTHUBCLIENT_LL_TAG
{
    STRING_HANDLE keyName;
    STRING_HANDLE keyValue;
    STRING_HANDLE eventHubpath;
    STRING_HANDLE host_name;
    DLIST_ENTRY outgoingEvents;
    CONNECTION_HANDLE connection;
    SESSION_HANDLE session;
    LINK_HANDLE link;
    SASL_MECHANISM_HANDLE sasl_mechanism_handle;
    XIO_HANDLE sasl_io;
    XIO_HANDLE tls_io;
    MESSAGE_SENDER_STATE message_sender_state;
    MESSAGE_SENDER_HANDLE message_sender;
} EVENTHUBCLIENT_LL;

static const char ENDPOINT_SUBSTRING[] = "Endpoint=sb://";
static const size_t ENDPOINT_SUBSTRING_LENGTH = sizeof(ENDPOINT_SUBSTRING) / sizeof(ENDPOINT_SUBSTRING[0]) - 1;
static const char SERVICEBUS_PATH_STRING[] = "servicebus.windows.net";
static const char SERVICEBUS_PATH_STRING_ALTERNATIVE[] = "servicebus.windows.net/";

static const char* PARTITION_KEY_NAME = "x-opt-partition-key";
static const char* SHARED_ACCESS_KEY_NAME = "SharedAccessKeyName";

static void on_message_sender_state_changed(const void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
{
    EVENTHUBCLIENT_LL* eventhub_client_ll = (EVENTHUBCLIENT_LL*)context;
    eventhub_client_ll->message_sender_state = new_state;
}

EVENTHUBCLIENT_LL_HANDLE EventHubClient_LL_CreateFromConnectionString(const char* connectionString, const char* eventHubPath)
{
    EVENTHUBCLIENT_LL* eventhub_client_ll;
    STRING_HANDLE connection_string;

    /* Codes_SRS_EVENTHUBCLIENT_LL_05_001: [EventHubClient_LL_CreateFromConnectionString shall obtain the version string by a call to EventHubClient_GetVersionString.] */
    /* Codes_SRS_EVENTHUBCLIENT_LL_05_002: [EventHubClient_LL_CreateFromConnectionString shall print the version string to standard output.] */
    LogInfo("Event Hubs Client SDK for C, version %s\r\n", EventHubClient_GetVersionString());

    if (connectionString == NULL || eventHubPath == NULL)
    {
        LogError("Invalid Argument. result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG));
        eventhub_client_ll = NULL;
    }
    else if ((connection_string = STRING_construct(connectionString)) == NULL)
    {
        LogError("Error creating connection String.\r\n");
        eventhub_client_ll = NULL;
    }
    else
    {
        MAP_HANDLE connection_string_values_map;

        if ((connection_string_values_map = connectionstringparser_parse(connection_string)) == NULL)
        {
            LogError("Error parsing connection string.\r\n");
            eventhub_client_ll = NULL;
        }
        else
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_03_002: [EventHubClient_LL_CreateFromConnectionString shall allocate a new event hub client LL instance.] */
            /* Codes_SRS_EVENTHUBCLIENT_LL_03_016: [EventHubClient_LL_CreateFromConnectionString shall return a non-NULL handle value upon success.] */
            if ((eventhub_client_ll = malloc(sizeof(EVENTHUBCLIENT_LL))) == NULL)
            {
                LogError("Memory Allocation Failed for eventhub_client_ll.\r\n");
            }
            else
            {
                bool error = false;
                STRING_HANDLE target_address = NULL;
                const char* value;
                const char* endpoint;

                eventhub_client_ll->keyName = NULL;
                eventhub_client_ll->keyValue = NULL;
                eventhub_client_ll->host_name = NULL;
                eventhub_client_ll->sasl_io = NULL;
                eventhub_client_ll->tls_io = NULL;
                eventhub_client_ll->sasl_mechanism_handle = NULL;
                eventhub_client_ll->message_sender = NULL;
                eventhub_client_ll->connection = NULL;
                eventhub_client_ll->session = NULL;
                eventhub_client_ll->link = NULL;

                /* Codes_SRS_EVENTHUBCLIENT_LL_03_017: [EventHubClient_ll expects a service bus connection string in one of the following formats:
                Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]
                Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[key name];SharedAccessKey=[key value] ]*/
                /* Codes_SRS_EVENTHUBCLIENT_LL_01_065: [The connection string shall be parsed to a map of strings by using connection_string_parser_parse.] */
                if ((endpoint = Map_GetValueFromKey(connection_string_values_map, "Endpoint")) == NULL)
                {
                    error = true;
                    LogError("Couldn't find endpoint in connection string\r\n");
                }
                /* Codes_SRS_EVENTHUBCLIENT_LL_01_067: [The endpoint shall be looked up in the resulting map and used to construct the host name to be used for connecting by removing the sb://.] */
                else if ((eventhub_client_ll->host_name = STRING_construct(endpoint + SB_STRING_LENGTH)) == NULL)
                {
                    error = true;
                    LogError("Couldn't create host name string\r\n");
                }
                else if ((value = Map_GetValueFromKey(connection_string_values_map, "SharedAccessKeyName")) == NULL)
                {
                    error = true;
                    LogError("Couldn't find key name in connection string\r\n");
                }
                /* Codes_SRS_EVENTHUBCLIENT_LL_01_068: [The key name and key shall be looked up in the resulting map and they should be stored as is for later use in connecting.] */
                else if ((eventhub_client_ll->keyName = STRING_construct(value)) == NULL)
                {
                    error = true;
                    LogError("Couldn't create key name string\r\n");
                }
                else if ((value = Map_GetValueFromKey(connection_string_values_map, "SharedAccessKey")) == NULL)
                {
                    error = true;
                    LogError("Couldn't find key in connection string\r\n");
                }
                else if ((eventhub_client_ll->keyValue = STRING_construct(value)) == NULL)
                {
                    error = true;
                    LogError("Couldn't create key string\r\n");
                }
                /* Codes_SRS_EVENTHUBCLIENT_LL_01_024: [The target address shall be "amqps://" {eventhub hostname} / {eventhub name}.] */
                else if (((target_address = STRING_construct("amqps://")) == NULL) ||
                    (STRING_concat(target_address, endpoint + SB_STRING_LENGTH) != 0) ||
                    (STRING_concat(target_address, "/") != 0) ||
                    (STRING_concat(target_address, eventHubPath) != 0))
                {
                    error = true;
                    LogError("Couldn't assemble target address\r\n");
                }
                else
                {
                    AMQP_VALUE source;
                    AMQP_VALUE target;

                    DList_InitializeListHead(&(eventhub_client_ll->outgoingEvents));
                    eventhub_client_ll->message_sender_state = MESSAGE_SENDER_STATE_IDLE;

                    /* create SASL PLAIN handler */
                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_007: [The creation parameters for the SASL plain mechanism shall be in the form of a SASL_PLAIN_CONFIG structure.] */
                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_008: [The authcid shall be set to the key name parsed earlier from the connection string.] */
                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_009: [The passwd members shall be set to the key value parsed earlier from the connection string.] */
                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_010: [The authzid shall be NULL.] */
                    SASL_PLAIN_CONFIG sasl_plain_config = { STRING_c_str(eventhub_client_ll->keyName), STRING_c_str(eventhub_client_ll->keyValue), NULL };
                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_004: [A SASL plain mechanism shall be created by calling saslmechanism_create.] */
                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_005: [The interface passed to saslmechanism_create shall be obtained by calling saslplain_get_interface.] */
                    eventhub_client_ll->sasl_mechanism_handle = saslmechanism_create(saslplain_get_interface(), &sasl_plain_config);
                    if (eventhub_client_ll->sasl_mechanism_handle == NULL)
                    {
                        error = true;
                        LogError("saslmechanism_create failed.\r\n");
                    }
                    else
                    {
                        TLSIO_CONFIG tls_io_config = { endpoint + SB_STRING_LENGTH, 5671 };

                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_002: [The TLS IO interface description passed to xio_create shall be obtained by calling platform_get_default_tlsio_interface.] */
                        const IO_INTERFACE_DESCRIPTION* tlsio_interface = platform_get_default_tlsio();

                        /* Codes_SRS_EVENTHUBCLIENT_LL_03_030: [EventHubClient_LL_CreateFromConnectionString shall create a TLS IO by calling xio_create.] */
                        if ((eventhub_client_ll->tls_io = xio_create(tlsio_interface, &tls_io_config, NULL)) == NULL)
                        {
                            error = true;
                            LogError("TLS IO creation failed.\r\n");
                        }
                        else
                        {
                            /* create the SASL client IO using the TLS IO */
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_015: [The IO creation parameters passed to xio_create shall be in the form of a SASLCLIENTIO_CONFIG.] */
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_016: [The underlying_io members shall be set to the previously created TLS IO.] */
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_017: [The sasl_mechanism shall be set to the previously created SASL PLAIN mechanism.] */
                            SASLCLIENTIO_CONFIG sasl_io_config = { eventhub_client_ll->tls_io, eventhub_client_ll->sasl_mechanism_handle };

                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_012: [A SASL client IO shall be created by calling xio_create.] */
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_013: [The IO interface description for the SASL client IO shall be obtained by calling saslclientio_get_interface_description.] */
                            if ((eventhub_client_ll->sasl_io = xio_create(saslclientio_get_interface_description(), &sasl_io_config, NULL)) == NULL)
                            {
                                error = true;
                                LogError("SASL client IO creation failed.\r\n");
                            }
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_019: [An AMQP connection shall be created by calling connection_create and passing as arguments the SASL client IO handle, eventhub hostname, "eh_client_connection" as container name and NULL for the new session handler and context.] */
                            else if ((eventhub_client_ll->connection = connection_create(eventhub_client_ll->sasl_io, endpoint + SB_STRING_LENGTH, "eh_client_connection", NULL, NULL)) == NULL)
                            {
                                error = true;
                                LogError("connection_create failed.\r\n");
                            }
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_028: [An AMQP session shall be created by calling session_create and passing as arguments the connection handle, and NULL for the new link handler and context.] */
                            else if ((eventhub_client_ll->session = session_create(eventhub_client_ll->connection, NULL, NULL)) == NULL)
                            {
                                error = true;
                                LogError("session_create failed.\r\n");
                            }
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_030: [The outgoing window for the session shall be set to 10 by calling session_set_outgoing_window.] */
                            else if (session_set_outgoing_window(eventhub_client_ll->session, 10) != 0)
                            {
                                error = true;
                                LogError("session_set_outgoing_window failed.\r\n");
                            }
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_021: [A source AMQP value shall be created by calling messaging_create_source.] */
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_022: [The source address shall be "ingress".] */
                            else if ((source = messaging_create_source("ingress")) == NULL)
                            {
                                error = true;
                                LogError("messaging_create_source failed.\r\n");
                            }
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_023: [A target AMQP value shall be created by calling messaging_create_target.] */
                            else if ((target = messaging_create_target(STRING_c_str(target_address))) == NULL)
                            {
                                error = true;
                                LogError("messaging_create_target failed.\r\n");
                            }
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_026: [An AMQP link shall be created by calling link_create and passing as arguments the session handle, "sender-link" as link name, role_sender and the previously created source and target values.] */
                            else if ((eventhub_client_ll->link = link_create(eventhub_client_ll->session, "sender-link", role_sender, source, target)) == NULL)
                            {
                                error = true;
                                LogError("link_create failed.\r\n");
                            }
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_032: [The link sender settle mode shall be set to unsettled by calling link_set_snd_settle_mode.] */
                            if (link_set_snd_settle_mode(eventhub_client_ll->link, sender_settle_mode_unsettled) != 0)
                            {
                                error = true;
                                LogError("link_set_snd_settle_mode failed.\r\n");
                            }
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_034: [The message size shall be set to 256K by calling link_set_max_message_size.] */
                            else if (link_set_max_message_size(eventhub_client_ll->link, 256 * 1024) != 0)
                            {
                                error = true;
                                LogError("link_set_max_message_size failed.\r\n");
                            }
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_036: [A message sender shall be created by calling messagesender_create and passing as arguments the link handle, a state changed callback, a context and NULL for the logging function.] */
                            else if ((eventhub_client_ll->message_sender = messagesender_create(eventhub_client_ll->link, on_message_sender_state_changed, eventhub_client_ll, NULL)) == NULL)
                            {
                                error = true;
                                LogError("messagesender_create failed.\r\n");
                            }
                        }
                    }
                }

                if (target_address != NULL)
                {
                    STRING_delete(target_address);
                }

                if (error == true)
                {
                    if (eventhub_client_ll->keyName != NULL)
                    {
                        STRING_delete(eventhub_client_ll->keyName);
                    }
                    if (eventhub_client_ll->keyValue != NULL)
                    {
                        STRING_delete(eventhub_client_ll->keyValue);
                    }
                    if (eventhub_client_ll->host_name != NULL)
                    {
                        STRING_delete(eventhub_client_ll->host_name);
                    }
                    if (eventhub_client_ll->link != NULL)
                    {
                        link_destroy(eventhub_client_ll->link);
                    }
                    if (eventhub_client_ll->session != NULL)
                    {
                        session_destroy(eventhub_client_ll->session);
                    }
                    if (eventhub_client_ll->connection != NULL)
                    {
                        connection_destroy(eventhub_client_ll->connection);
                    }
                    if (eventhub_client_ll->sasl_io != NULL)
                    {
                        xio_destroy(eventhub_client_ll->sasl_io);
                    }
                    if (eventhub_client_ll->tls_io != NULL)
                    {
                        xio_destroy(eventhub_client_ll->tls_io);
                    }
                    if (eventhub_client_ll->sasl_mechanism_handle != NULL)
                    {
                        saslmechanism_destroy(eventhub_client_ll->sasl_mechanism_handle);
                    }

                    free(eventhub_client_ll);
                    eventhub_client_ll = NULL;
                }
            }
        }

        STRING_delete(connection_string);
    }

    return ((EVENTHUBCLIENT_LL_HANDLE)eventhub_client_ll);
}

static int ValidateEventDataList(EVENTDATA_HANDLE *eventDataList, size_t count)
{
    int result = 0;
    const char* partitionKey = NULL;
    size_t index;

    for (index = 0; index < count; index++)
    {
        if (eventDataList[index] == NULL)
        {
            result = __LINE__;
            LogError("handle index %d NULL result = %s\r\n", (int)index, ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG));
            break;
        }
        else
        {
            const char* currPartKey = EventData_GetPartitionKey(eventDataList[index]);
            if (index == 0)
            {
                partitionKey = currPartKey;
            }
            else
            {
                if ( (currPartKey == NULL && partitionKey != NULL) || (currPartKey != NULL && partitionKey == NULL) )
                {
                    result = __LINE__;
                    LogError("All event data in a SendBatch operation must have the same partition key result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_PARTITION_KEY_MISMATCH) );
                    break;
                }
                else
                {
                    if (currPartKey != NULL && partitionKey != NULL)
                    {
                        if (strcmp(partitionKey, currPartKey) != 0)
                        {
                            /*Codes_SRS_EVENTHUBCLIENT_07_045: [If all of the eventDataHandle objects contain differing partitionKey values then EventHubClient_SendBatch shall fail and return EVENTHUBCLIENT_PARTITION_KEY_MISMATCH.]*/
                            result = __LINE__;
                            LogError("All event data in a SendBatch operation must have the same partition key result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_PARTITION_KEY_MISMATCH));
                            break;
                        }
                    }
                }
            }
        }
    }
    return result;
}

void EventHubClient_LL_Destroy(EVENTHUBCLIENT_LL_HANDLE eventhub_client_ll)
{
    /* Codes_SRS_EVENTHUBCLIENT_LL_03_010: [If the eventhub_client_ll is NULL, EventHubClient_LL_Destroy shall not do anything.] */
    if (eventhub_client_ll != NULL)
    {
		PDLIST_ENTRY unsend;
        /* Codes_SRS_EVENTHUBCLIENT_LL_03_009: [EventHubClient_LL_Destroy shall terminate the usage of this EventHubClient specified by the eventhub_client_ll and cleanup all associated resources.] */
       
        link_destroy(eventhub_client_ll->link);
        session_destroy(eventhub_client_ll->session);
        connection_destroy(eventhub_client_ll->connection);

        /* Codes_SRS_EVENTHUBCLIENT_LL_04_017: [EventHubClient_LL_Destroy shall complete all the event notifications callbacks that are in the outgoingdestroy the outgoingEvents with the result EVENTHUBCLIENT_CONFIRMATION_DESTROY.] */
        while ((unsend = DList_RemoveHeadList(&(eventhub_client_ll->outgoingEvents))) != &(eventhub_client_ll->outgoingEvents))
        {
            EVENTHUB_EVENT_LIST* temp = containingRecord(unsend, EVENTHUB_EVENT_LIST, entry);
            if (temp->callback != NULL)
            {
                temp->callback(EVENTHUBCLIENT_CONFIRMATION_DESTROY, temp->context);
            }
            // Destroy all items in the list
            for (size_t index = 0; index < temp->dataCount; index++)
            {
                EventData_Destroy(temp->eventDataList[index]);
            }

            free(temp->eventDataList);
            free(temp);
        }

        free(eventhub_client_ll);
    }
}

EVENTHUBCLIENT_RESULT EventHubClient_LL_SendAsync(EVENTHUBCLIENT_LL_HANDLE eventhub_client_ll, EVENTDATA_HANDLE eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncConfirmationCallback, void* userContextCallback)
{
   EVENTHUBCLIENT_RESULT result;

    /* Codes_SRS_EVENTHUBCLIENT_LL_04_011: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter eventhub_client_ll or eventDataHandle is NULL.] */
    /* Codes_SRS_EVENTHUBCLIENT_LL_04_012: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter telemetryConfirmationCallBack is NULL and userContextCallBack is not NULL.] */
    if (eventhub_client_ll == NULL || eventDataHandle == NULL || (sendAsyncConfirmationCallback == NULL && userContextCallback != NULL))
    {
        result = EVENTHUBCLIENT_INVALID_ARG;
        LOG_ERROR;
    }
    else
    {
        EVENTHUB_EVENT_LIST *newEntry = (EVENTHUB_EVENT_LIST*)malloc(sizeof(EVENTHUB_EVENT_LIST));
        if (newEntry == NULL)
        {
            result = EVENTHUBCLIENT_ERROR;
            LOG_ERROR;
        }
        else
        {
            newEntry->currentStatus = WAITING_TO_BE_SENT;
            newEntry->dataCount = 1;
            newEntry->eventDataList = malloc(sizeof(EVENTDATA_HANDLE) );
            if (newEntry->eventDataList == NULL)
            {
                result = EVENTHUBCLIENT_ERROR;
                free(newEntry);
                LOG_ERROR;
            }
            else
            {
                /* Codes_SRS_EVENTHUBCLIENT_LL_04_013: [EventHubClient_LL_SendAsync shall add the DLIST outgoingEvents a new record cloning the information from eventDataHandle, telemetryConfirmationCallback and userContextCallBack.] */
                if ((newEntry->eventDataList[0] = EventData_Clone(eventDataHandle)) == NULL)
                {
                    /* Codes_SRS_EVENTHUBCLIENT_LL_04_014: [If cloning and/or adding the information fails for any reason, EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_ERROR.] */
                    result = EVENTHUBCLIENT_ERROR;
                    free(newEntry->eventDataList);
                    free(newEntry);
                    LOG_ERROR;
                }
                else
                {
                    /* Codes_SRS_EVENTHUBCLIENT_LL_04_013: [EventHubClient_LL_SendAsync shall add the DLIST outgoingEvents a new record cloning the information from eventDataHandle, telemetryConfirmationCallback and userContextCallBack.] */
                    newEntry->callback = sendAsyncConfirmationCallback;
                    newEntry->context = userContextCallback;
                    DList_InsertTailList(&(eventhub_client_ll->outgoingEvents), &(newEntry->entry));
                    /* Codes_SRS_EVENTHUBCLIENT_LL_04_015: [Otherwise EventHubClient_LL_SendAsync shall succeed and return EVENTHUBCLIENT_OK.] */
                    result = EVENTHUBCLIENT_OK;
                }
            }
        }
    }

    return result;
}

EVENTHUBCLIENT_RESULT EventHubClient_LL_SendBatchAsync(EVENTHUBCLIENT_LL_HANDLE eventhub_client_ll, EVENTDATA_HANDLE* eventDataList, size_t count, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncConfirmationCallback, void* userContextCallback)
{
    EVENTHUBCLIENT_RESULT result;
    /* Codes_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALLID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    if (eventhub_client_ll == NULL || eventDataList == NULL || count == 0 || (sendAsyncConfirmationCallback == NULL && userContextCallback != NULL))
    {
        result = EVENTHUBCLIENT_INVALID_ARG;
        LOG_ERROR;
    }
    else
    {
        size_t index;
        if (ValidateEventDataList(eventDataList, count) != 0)
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shallreturn EVENTHUBCLIENT_ERROR for any Error that is encountered.] */
            result = EVENTHUBCLIENT_ERROR;
            LOG_ERROR;
        }
        else
        {
            EVENTHUB_EVENT_LIST *newEntry = (EVENTHUB_EVENT_LIST*)malloc(sizeof(EVENTHUB_EVENT_LIST));
            if (newEntry == NULL)
            {
                result = EVENTHUBCLIENT_ERROR;
                LOG_ERROR;
            }
            else
            {
                newEntry->currentStatus = WAITING_TO_BE_SENT;
                newEntry->dataCount = count;
                newEntry->eventDataList = malloc(sizeof(EVENTDATA_HANDLE)*count);
                if (newEntry->eventDataList == NULL)
                {
                    free(newEntry);
                    result = EVENTHUBCLIENT_ERROR;
                    LOG_ERROR;
                }
                else
                {
                    /* Codes_SRS_EVENTHUBCLIENT_LL_07_014: [EventHubClient_LL_SendBatchAsync shall clone each item in the eventDataList by calling EventData_Clone.] */
                    for (index = 0; index < newEntry->dataCount; index++)
                    {
                        if ( (newEntry->eventDataList[index] = EventData_Clone(eventDataList[index])) == NULL)
                        {
                            break;
                        }
                    }

                    if (index < newEntry->dataCount)
                    {
                        for (size_t i = 0; i < index; i++)
                        {
                            EventData_Destroy(newEntry->eventDataList[i]);
                        }
                        /* Codes_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.] */
                        result = EVENTHUBCLIENT_ERROR;
                        free(newEntry->eventDataList);
                        free(newEntry);
                        LOG_ERROR;
                    }
                    else
                    {
                        EVENTHUBCLIENT_LL* ehClientLLData = (EVENTHUBCLIENT_LL*)eventhub_client_ll;
                        newEntry->callback = sendAsyncConfirmationCallback;
                        newEntry->context = userContextCallback;
                        DList_InsertTailList(&(ehClientLLData->outgoingEvents), &(newEntry->entry));
                        /* Codes_SRS_EVENTHUBCLIENT_LL_07_015: [On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
                        result = EVENTHUBCLIENT_OK;
                    }
                }
            }
        }
    }
    return result;
}

static void on_message_send_complete(const void* context, MESSAGE_SEND_RESULT send_result)
{
    PEVENTHUB_EVENT_LIST event_list_item = (PEVENTHUB_EVENT_LIST)context;
    if (send_result == MESSAGE_SEND_OK)
    {
        event_list_item->callback(EVENTHUBCLIENT_CONFIRMATION_OK, event_list_item->context);
    }
    else
    {
        event_list_item->callback(EVENTHUBCLIENT_CONFIRMATION_ERROR, event_list_item->context);
    }

    DList_RemoveEntryList(&event_list_item->entry);
}

void EventHubClient_LL_DoWork(EVENTHUBCLIENT_LL_HANDLE eventhub_client_ll)
{
    /* Codes_SRS_EVENTHUBCLIENT_LL_04_018: [if parameter eventhub_client_ll is NULL  EventHubClient_LL_DoWork shall immediately return.]   */
    if (eventhub_client_ll != NULL)
    {
        PDLIST_ENTRY currentListEntry;

        if (messagesender_open(eventhub_client_ll->message_sender) != 0)
        {
            LogError("Error opening message sender.");
        }
        else
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_04_019: [If the current status of the entry is WAITING_TO_BE_SENT and there is available spots on proton, defined by OUTGOING_WINDOW_SIZE, EventHubClient_LL_DoWork shall call create pn_message and put the message into messenger by calling pn_messenger_put.]  */
            currentListEntry = eventhub_client_ll->outgoingEvents.Flink;
            while (currentListEntry != &(eventhub_client_ll->outgoingEvents))
            {
                PEVENTHUB_EVENT_LIST currentWork = containingRecord(currentListEntry, EVENTHUB_EVENT_LIST, entry);
                if (currentWork->currentStatus == WAITING_TO_BE_SENT)
                {
                    MESSAGE_HANDLE message = message_create();
                    if (message == NULL)
                    {
                        currentWork->callback(EVENTHUBCLIENT_CONFIRMATION_ERROR, currentWork->context);
                    }
                    else
                    {
                        BINARY_DATA body;
                        EventData_GetData(currentWork->eventDataList[0], &body.bytes, &body.length);
                        message_add_body_amqp_data(message, body);

                        if (messagesender_send(eventhub_client_ll->message_sender, message, on_message_send_complete, currentWork) != 0)
                        {
                            currentWork->callback(EVENTHUBCLIENT_CONFIRMATION_ERROR, currentWork->context);
                        }
                        else
                        {
                            currentWork->currentStatus = WAITING_FOR_ACK;
                        }
                    }
                }

                currentListEntry = currentListEntry->Flink;
            }
        }

        connection_dowork(eventhub_client_ll->connection);
    }
}
