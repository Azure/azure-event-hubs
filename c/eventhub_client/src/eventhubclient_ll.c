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
#include "message.h"
#include "messaging.h"
#include "message_sender.h"
#include "saslclientio.h"
#include "sasl_plain.h"
#include "tlsio.h"
#include "platform.h"
#include "doublylinkedlist.h"

#define LOG_ERROR LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, result));
DEFINE_ENUM_STRINGS(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_RESULT_VALUES)

#define AMQP_MAX_MESSAGE_SIZE           (256*1024)

static const char SB_STRING[] = "sb://";
#define SB_STRING_LENGTH                ((sizeof(SB_STRING) / sizeof(SB_STRING[0])) - 1)   /* length for sb:// */

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
    size_t eventCount;
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
    STRING_HANDLE target_address;
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

static const char* PARTITION_KEY_NAME = "x-opt-partition-key";

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
                if ((currPartKey == NULL && partitionKey != NULL) || (currPartKey != NULL && partitionKey == NULL))
                {
                    result = __LINE__;
                    LogError("All event data in a SendBatch operation must have the same partition key result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_PARTITION_KEY_MISMATCH));
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

static void destroy_uamqp_stack(EVENTHUBCLIENT_LL_HANDLE eventhub_client_ll)
{
    if (eventhub_client_ll->message_sender != NULL)
    {
        messagesender_destroy(eventhub_client_ll->message_sender);
        eventhub_client_ll->message_sender = NULL;
    }
    if (eventhub_client_ll->link != NULL)
    {
        link_destroy(eventhub_client_ll->link);
        eventhub_client_ll->link = NULL;
    }
    if (eventhub_client_ll->session != NULL)
    {
        session_destroy(eventhub_client_ll->session);
        eventhub_client_ll->session = NULL;
    }
    if (eventhub_client_ll->connection != NULL)
    {
        connection_destroy(eventhub_client_ll->connection);
        eventhub_client_ll->connection = NULL;
    }
    if (eventhub_client_ll->sasl_io != NULL)
    {
        xio_destroy(eventhub_client_ll->sasl_io);
        eventhub_client_ll->sasl_io = NULL;
    }
    if (eventhub_client_ll->tls_io != NULL)
    {
        xio_destroy(eventhub_client_ll->tls_io);
        eventhub_client_ll->tls_io = NULL;
    }
    if (eventhub_client_ll->sasl_mechanism_handle != NULL)
    {
        saslmechanism_destroy(eventhub_client_ll->sasl_mechanism_handle);
        eventhub_client_ll->sasl_mechanism_handle = NULL;
    }

    eventhub_client_ll->message_sender_state = MESSAGE_SENDER_STATE_IDLE;
}

static void on_message_sender_state_changed(const void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
{
    EVENTHUBCLIENT_LL* eventhub_client_ll = (EVENTHUBCLIENT_LL*)context;
    (void)previous_state;
    eventhub_client_ll->message_sender_state = new_state;

    /* Codes_SRS_EVENTHUBCLIENT_LL_01_060: [When on_messagesender_state_changed is called with MESSAGE_SENDER_STATE_ERROR, the uAMQP stack shall be brough down so that it can be created again if needed in dowork:] */
    if (new_state == MESSAGE_SENDER_STATE_ERROR)
    {
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_072: [The message sender shall be destroyed by calling messagesender_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_073: [The link shall be destroyed by calling link_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_074: [The session shall be destroyed by calling session_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_075: [The connection shall be destroyed by calling connection_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_076: [The SASL IO shall be destroyed by calling xio_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_077: [The TLS IO shall be destroyed by calling xio_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_078: [The SASL mechanism shall be destroyed by calling saslmechanism_destroy.] */
        destroy_uamqp_stack(eventhub_client_ll);
    }
}

static int initialize_uamqp_stack(EVENTHUBCLIENT_LL_HANDLE eventhub_client_ll)
{
    int result;

    const char* host_name_temp = STRING_c_str(eventhub_client_ll->host_name);
    if (host_name_temp == NULL)
    {
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_080: [If any other error happens while bringing up the uAMQP stack, EventHubClient_LL_DoWork shall not attempt to open the message_sender and return without sending any messages.] */
        result = __LINE__;
        LogError("Couldn't assemble target address\r\n");
    }
    else
    {
        const char* target_address_str = STRING_c_str(eventhub_client_ll->target_address);
        if (target_address_str == NULL)
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_01_080: [If any other error happens while bringing up the uAMQP stack, EventHubClient_LL_DoWork shall not attempt to open the message_sender and return without sending any messages.] */
            result = __LINE__;
            LogError("cannot get the previously constructed target address.\r\n");
        }
        else
        {
            const char* authcid;
            authcid = STRING_c_str(eventhub_client_ll->keyName);
            if (authcid == NULL)
            {
                /* Codes_SRS_EVENTHUBCLIENT_LL_01_080: [If any other error happens while bringing up the uAMQP stack, EventHubClient_LL_DoWork shall not attempt to open the message_sender and return without sending any messages.] */
                result = __LINE__;
                LogError("Cannot get key name.\r\n");
            }
            else
            {
                const char* passwd = STRING_c_str(eventhub_client_ll->keyValue);
                if (passwd == NULL)
                {
                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_080: [If any other error happens while bringing up the uAMQP stack, EventHubClient_LL_DoWork shall not attempt to open the message_sender and return without sending any messages.] */
                    result = __LINE__;
                    LogError("Cannot get key.\r\n");
                }
                else
                {
                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_005: [The interface passed to saslmechanism_create shall be obtained by calling saslplain_get_interface.] */
                    const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_mechanism_interface = saslplain_get_interface();
                    if (sasl_mechanism_interface == NULL)
                    {
                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_006: [If saslplain_get_interface fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.] */
                        result = __LINE__;
                        LogError("Cannot obtain SASL PLAIN interface.\r\n");
                    }
                    else
                    {
                        eventhub_client_ll->message_sender_state = MESSAGE_SENDER_STATE_IDLE;

                        /* create SASL PLAIN handler */
                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_007: [The creation parameters for the SASL plain mechanism shall be in the form of a SASL_PLAIN_CONFIG structure.] */
                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_008: [The authcid shall be set to the key name parsed earlier from the connection string.] */
                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_009: [The passwd members shall be set to the key value parsed earlier from the connection string.] */
                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_010: [The authzid shall be NULL.] */
                        SASL_PLAIN_CONFIG sasl_plain_config = { authcid, passwd, NULL };
                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_004: [A SASL plain mechanism shall be created by calling saslmechanism_create.] */
                        eventhub_client_ll->sasl_mechanism_handle = saslmechanism_create(sasl_mechanism_interface, &sasl_plain_config);
                        if (eventhub_client_ll->sasl_mechanism_handle == NULL)
                        {
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_011: [If sasl_mechanism_create fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.] */
                            result = __LINE__;
                            LogError("saslmechanism_create failed.\r\n");
                        }
                        else
                        {
                            TLSIO_CONFIG tls_io_config = { host_name_temp, 5671 };

                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_002: [The TLS IO interface description passed to xio_create shall be obtained by calling platform_get_default_tlsio_interface.] */
                            const IO_INTERFACE_DESCRIPTION* tlsio_interface = platform_get_default_tlsio();

                            if (tlsio_interface == NULL)
                            {
                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_001: [If platform_get_default_tlsio_interface fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages. ] */
                                result = __LINE__;
                                LogError("Could not obtain default TLS IO.\r\n");
                            }
                            else
                            {
                                /* Codes_SRS_EVENTHUBCLIENT_LL_03_030: [A TLS IO shall be created by calling xio_create.] */
                                if ((eventhub_client_ll->tls_io = xio_create(tlsio_interface, &tls_io_config, NULL)) == NULL)
                                {
                                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_003: [If xio_create fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
                                    result = __LINE__;
                                    LogError("TLS IO creation failed.\r\n");
                                }
                                else
                                {
                                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_013: [The IO interface description for the SASL client IO shall be obtained by calling saslclientio_get_interface_description.] */
                                    const IO_INTERFACE_DESCRIPTION* saslclientio_interface = saslclientio_get_interface_description();
                                    if (saslclientio_interface == NULL)
                                    {
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_014: [If saslclientio_get_interface_description fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
                                        result = __LINE__;
                                        LogError("TLS IO creation failed.\r\n");
                                    }
                                    else
                                    {
                                        AMQP_VALUE source = NULL;
                                        AMQP_VALUE target = NULL;

                                        /* create the SASL client IO using the TLS IO */
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_015: [The IO creation parameters passed to xio_create shall be in the form of a SASLCLIENTIO_CONFIG.] */
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_016: [The underlying_io members shall be set to the previously created TLS IO.] */
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_017: [The sasl_mechanism shall be set to the previously created SASL PLAIN mechanism.] */
                                        SASLCLIENTIO_CONFIG sasl_io_config = { eventhub_client_ll->tls_io, eventhub_client_ll->sasl_mechanism_handle };

                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_012: [A SASL client IO shall be created by calling xio_create.] */
                                        if ((eventhub_client_ll->sasl_io = xio_create(saslclientio_interface, &sasl_io_config, NULL)) == NULL)
                                        {
                                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_018: [If xio_create fails creating the SASL client IO then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
                                            result = __LINE__;
                                            LogError("SASL client IO creation failed.\r\n");
                                        }
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_019: [An AMQP connection shall be created by calling connection_create and passing as arguments the SASL client IO handle, eventhub hostname, "eh_client_connection" as container name and NULL for the new session handler and context.] */
                                        else if ((eventhub_client_ll->connection = connection_create(eventhub_client_ll->sasl_io, host_name_temp, "eh_client_connection", NULL, NULL)) == NULL)
                                        {
                                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_020: [If connection_create fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
                                            result = __LINE__;
                                            LogError("connection_create failed.\r\n");
                                        }
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_028: [An AMQP session shall be created by calling session_create and passing as arguments the connection handle, and NULL for the new link handler and context.] */
                                        else if ((eventhub_client_ll->session = session_create(eventhub_client_ll->connection, NULL, NULL)) == NULL)
                                        {
                                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_029: [If session_create fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
                                            result = __LINE__;
                                            LogError("session_create failed.\r\n");
                                        }
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_030: [The outgoing window for the session shall be set to 10 by calling session_set_outgoing_window.] */
                                        else if (session_set_outgoing_window(eventhub_client_ll->session, 10) != 0)
                                        {
                                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_031: [If setting the outgoing window fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
                                            result = __LINE__;
                                            LogError("session_set_outgoing_window failed.\r\n");
                                        }
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_021: [A source AMQP value shall be created by calling messaging_create_source.] */
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_022: [The source address shall be "ingress".] */
                                        else if ((source = messaging_create_source("ingress")) == NULL)
                                        {
                                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_025: [If creating the source or target values fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
                                            result = __LINE__;
                                            LogError("messaging_create_source failed.\r\n");
                                        }
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_023: [A target AMQP value shall be created by calling messaging_create_target.] */
                                        else
                                        {
                                            if ((target = messaging_create_target(target_address_str)) == NULL)
                                            {
                                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_025: [If creating the source or target values fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
                                                result = __LINE__;
                                                LogError("messaging_create_target failed.\r\n");
                                            }
                                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_026: [An AMQP link shall be created by calling link_create and passing as arguments the session handle, "sender-link" as link name, role_sender and the previously created source and target values.] */
                                            else if ((eventhub_client_ll->link = link_create(eventhub_client_ll->session, "sender-link", role_sender, source, target)) == NULL)
                                            {
                                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_027: [If creating the link fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
                                                result = __LINE__;
                                                LogError("link_create failed.\r\n");
                                            }
                                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_032: [The link sender settle mode shall be set to unsettled by calling link_set_snd_settle_mode.] */
                                            else if (link_set_snd_settle_mode(eventhub_client_ll->link, sender_settle_mode_unsettled) != 0)
                                            {
                                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_033: [If link_set_snd_settle_mode fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.] */
                                                result = __LINE__;
                                                LogError("link_set_snd_settle_mode failed.\r\n");
                                            }
                                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_034: [The message size shall be set to 256K by calling link_set_max_message_size.] */
                                            else if (link_set_max_message_size(eventhub_client_ll->link, AMQP_MAX_MESSAGE_SIZE) != 0)
                                            {
                                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_035: [If link_set_max_message_size fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.] */
                                                result = __LINE__;
                                                LogError("link_set_max_message_size failed.\r\n");
                                            }
                                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_036: [A message sender shall be created by calling messagesender_create and passing as arguments the link handle, a state changed callback, a context and NULL for the logging function.] */
                                            else if ((eventhub_client_ll->message_sender = messagesender_create(eventhub_client_ll->link, on_message_sender_state_changed, eventhub_client_ll, NULL)) == NULL)
                                            {
                                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_037: [If creating the message sender fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.] */
                                                result = __LINE__;
                                                LogError("messagesender_create failed.\r\n");
                                            }
                                            else
                                            {
                                                result = 0;
                                            }
                                        }

                                        if (source != NULL)
                                        {
                                            amqpvalue_destroy(source);
                                        }

                                        if (target != NULL)
                                        {
                                            amqpvalue_destroy(target);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (result != 0)
    {
        destroy_uamqp_stack(eventhub_client_ll);
    }

    return result;
}

static void on_message_send_complete(const void* context, MESSAGE_SEND_RESULT send_result)
{
    PDLIST_ENTRY currentListEntry = (PDLIST_ENTRY)context;
    EVENTHUBCLIENT_CONFIRMATION_RESULT callback_confirmation_result;
    PEVENTHUB_EVENT_LIST currentEvent = containingRecord(currentListEntry, EVENTHUB_EVENT_LIST, entry);
    size_t index;

    if (send_result == MESSAGE_SEND_OK)
    {
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_061: [When on_message_send_complete is called with MESSAGE_SEND_OK the pending message shall be indicated as sent correctly by calling the callback associated with the pending message with EVENTHUBCLIENT_CONFIRMATION_OK.] */
        callback_confirmation_result = EVENTHUBCLIENT_CONFIRMATION_OK;
    }
    else
    {
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_063: [When on_message_send_complete is called with a result code different than MESSAGE_SEND_OK the pending message shall be indicated as having an error by calling the callback associated with the pending message with EVENTHUBCLIENT_CONFIRMATION_ERROR.]  */
        callback_confirmation_result = EVENTHUBCLIENT_CONFIRMATION_ERROR;
    }

    currentEvent->callback(callback_confirmation_result, currentEvent->context);

    for (index = 0; index < currentEvent->eventCount; index++)
    {
        EventData_Destroy(currentEvent->eventDataList[index]);
    }

    /* Codes_SRS_EVENTHUBCLIENT_LL_01_062: [The pending message shall be removed from the pending list.] */
    DList_RemoveEntryList(currentListEntry);
    free(currentEvent->eventDataList);
    free(currentEvent);
}

EVENTHUBCLIENT_LL_HANDLE EventHubClient_LL_CreateFromConnectionString(const char* connectionString, const char* eventHubPath)
{
    EVENTHUBCLIENT_LL* eventhub_client_ll;
    STRING_HANDLE connection_string;

    /* Codes_SRS_EVENTHUBCLIENT_LL_05_001: [EventHubClient_LL_CreateFromConnectionString shall obtain the version string by a call to EventHubClient_GetVersionString.] */
    /* Codes_SRS_EVENTHUBCLIENT_LL_05_002: [EventHubClient_LL_CreateFromConnectionString shall print the version string to standard output.] */
    LogInfo("Event Hubs Client SDK for C, version %s\r\n", EventHubClient_GetVersionString());

    /* Codes_SRS_EVENTHUBCLIENT_LL_03_003: [EventHubClient_LL_CreateFromConnectionString shall return a NULL value if connectionString or eventHubPath is NULL.] */
    if (connectionString == NULL || eventHubPath == NULL)
    {
        LogError("Invalid arguments. connectionString=%p, eventHubPath=%p\r\n", connectionString, eventHubPath);
        eventhub_client_ll = NULL;
    }
    else if ((connection_string = STRING_construct(connectionString)) == NULL)
    {
        LogError("Error creating connection string handle.\r\n");
        eventhub_client_ll = NULL;
    }
    else
    {
        MAP_HANDLE connection_string_values_map;

        /* Codes_SRS_EVENTHUBCLIENT_LL_01_065: [The connection string shall be parsed to a map of strings by using connection_string_parser_parse.] */
        if ((connection_string_values_map = connectionstringparser_parse(connection_string)) == NULL)
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_01_066: [If connection_string_parser_parse fails then EventHubClient_LL_CreateFromConnectionString shall fail and return NULL.] */
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
                const char* value;
                const char* endpoint;

                /* Codes_SRS_EVENTHUBCLIENT_LL_04_016: [EventHubClient_LL_CreateFromConnectionString shall initialize the pending list that will be used to send Events.] */
                DList_InitializeListHead(&(eventhub_client_ll->outgoingEvents));

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
                eventhub_client_ll->message_sender_state = MESSAGE_SENDER_STATE_IDLE;

                /* Codes_SRS_EVENTHUBCLIENT_LL_03_017: [EventHubClient_ll expects a service bus connection string in one of the following formats:
                Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]
                Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[key name];SharedAccessKey=[key value] ]*/
                /* Codes_SRS_EVENTHUBCLIENT_LL_01_065: [The connection string shall be parsed to a map of strings by using connection_string_parser_parse.] */
                if ((endpoint = Map_GetValueFromKey(connection_string_values_map, "Endpoint")) == NULL)
                {
                    /* Codes_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
                    error = true;
                    LogError("Couldn't find endpoint in connection string\r\n");
                }
                /* Codes_SRS_EVENTHUBCLIENT_LL_01_067: [The endpoint shall be looked up in the resulting map and used to construct the host name to be used for connecting by removing the sb://.] */
                else
                {
                    size_t hostname_length = strlen(endpoint);

                    if ((hostname_length > SB_STRING_LENGTH) && (endpoint[hostname_length - 1] == '/'))
                    {
                        hostname_length--;
                    }

                    if ((hostname_length <= SB_STRING_LENGTH) ||
                        (strncmp(endpoint, SB_STRING, SB_STRING_LENGTH) != 0))
                    {
                        /* Codes_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
                        error = true;
                        LogError("Couldn't create host name string\r\n");
                    }
                    else if ((eventhub_client_ll->host_name = STRING_construct_n(endpoint + SB_STRING_LENGTH, hostname_length - SB_STRING_LENGTH)) == NULL)
                    {
                        /* Codes_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
                        error = true;
                        LogError("Couldn't create host name string\r\n");
                    }
                    else if (((value = Map_GetValueFromKey(connection_string_values_map, "SharedAccessKeyName")) == NULL) ||
                        (strlen(value) == 0))
                    {
                        /* Codes_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
                        error = true;
                        LogError("Couldn't find key name in connection string\r\n");
                    }
                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_068: [The key name and key shall be looked up in the resulting map and they should be stored as is for later use in connecting.] */
                    else if ((eventhub_client_ll->keyName = STRING_construct(value)) == NULL)
                    {
                        /* Codes_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
                        error = true;
                        LogError("Couldn't create key name string\r\n");
                    }
                    else if (((value = Map_GetValueFromKey(connection_string_values_map, "SharedAccessKey")) == NULL) ||
                        (strlen(value) == 0))
                    {
                        /* Codes_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
                        error = true;
                        LogError("Couldn't find key in connection string\r\n");
                    }
                    else if ((eventhub_client_ll->keyValue = STRING_construct(value)) == NULL)
                    {
                        /* Codes_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
                        error = true;
                        LogError("Couldn't create key string\r\n");
                    }
                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_024: [The target address shall be "amqps://" {eventhub hostname} / {eventhub name}.] */
                    else if (((eventhub_client_ll->target_address = STRING_construct("amqps://")) == NULL) ||
                        (STRING_concat_with_STRING(eventhub_client_ll->target_address, eventhub_client_ll->host_name) != 0) ||
                        (STRING_concat(eventhub_client_ll->target_address, "/") != 0) ||
                        (STRING_concat(eventhub_client_ll->target_address, eventHubPath) != 0))
                    {
                        /* Codes_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
                        error = true;
                        LogError("Couldn't assemble target address\r\n");
                    }
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

                    free(eventhub_client_ll);
                    eventhub_client_ll = NULL;
                }
            }

            Map_Destroy(connection_string_values_map);
        }

        STRING_delete(connection_string);
    }

    return ((EVENTHUBCLIENT_LL_HANDLE)eventhub_client_ll);
}

void EventHubClient_LL_Destroy(EVENTHUBCLIENT_LL_HANDLE eventhub_client_ll)
{
    /* Codes_SRS_EVENTHUBCLIENT_LL_03_010: [If the eventhub_client_ll is NULL, EventHubClient_LL_Destroy shall not do anything.] */
    if (eventhub_client_ll != NULL)
    {
		PDLIST_ENTRY unsend;

        /* Codes_SRS_EVENTHUBCLIENT_LL_03_009: [EventHubClient_LL_Destroy shall terminate the usage of this EventHubClient_LL specified by the eventHubLLHandle and cleanup all associated resources.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_042: [The message sender shall be freed by calling messagesender_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_043: [The link shall be freed by calling link_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_044: [The session shall be freed by calling session_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_045: [The connection shall be freed by calling connection_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_046: [The SASL client IO shall be freed by calling xio_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_047: [The TLS IO shall be freed by calling xio_destroy.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_048: [The SASL plain mechanism shall be freed by calling saslmechanism_destroy.] */
        destroy_uamqp_stack(eventhub_client_ll);

        /* Codes_SRS_EVENTHUBCLIENT_LL_01_081: [The key host name, key name and key allocated in EventHubClient_LL_CreateFromConnectionString shall be freed.] */
        STRING_delete(eventhub_client_ll->target_address);
        STRING_delete(eventhub_client_ll->host_name);
        STRING_delete(eventhub_client_ll->keyName);
        STRING_delete(eventhub_client_ll->keyValue);

        /* Codes_SRS_EVENTHUBCLIENT_LL_04_017: [EventHubClient_LL_Destroy shall complete all the event notifications callbacks that are in the outgoingdestroy the outgoingEvents with the result EVENTHUBCLIENT_CONFIRMATION_DESTROY.] */
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_041: [All pending message data shall be freed.] */
        while ((unsend = DList_RemoveHeadList(&(eventhub_client_ll->outgoingEvents))) != &(eventhub_client_ll->outgoingEvents))
        {
            EVENTHUB_EVENT_LIST* temp = containingRecord(unsend, EVENTHUB_EVENT_LIST, entry);
            /* Codes_SRS_EVENTHUBCLIENT_LL_01_040: [All the pending messages shall be indicated as error by calling the associated callback with EVENTHUBCLIENT_CONFIRMATION_DESTROY.] */
            if (temp->callback != NULL)
            {
                temp->callback(EVENTHUBCLIENT_CONFIRMATION_DESTROY, temp->context);
            }

            /* Codes_SRS_EVENTHUBCLIENT_LL_01_041: [All pending message data shall be freed.] */
            for (size_t index = 0; index < temp->eventCount; index++)
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
        EVENTHUB_EVENT_LIST* newEntry = (EVENTHUB_EVENT_LIST*)malloc(sizeof(EVENTHUB_EVENT_LIST));
        if (newEntry == NULL)
        {
            result = EVENTHUBCLIENT_ERROR;
            LOG_ERROR;
        }
        else
        {
            newEntry->currentStatus = WAITING_TO_BE_SENT;
            newEntry->eventCount = 1;
            newEntry->eventDataList = malloc(sizeof(EVENTDATA_HANDLE));
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
            /* Codes_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.] */
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
                newEntry->eventCount = count;
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
                    for (index = 0; index < newEntry->eventCount; index++)
                    {
                        if ( (newEntry->eventDataList[index] = EventData_Clone(eventDataList[index])) == NULL)
                        {
                            break;
                        }
                    }

                    if (index < newEntry->eventCount)
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

int encode_callback(void* context, const unsigned char* bytes, size_t length)
{
    BINARY_DATA* message_body_binary = (BINARY_DATA*)context;
    (void)memcpy((unsigned char*)message_body_binary->bytes + message_body_binary->length, bytes, length);
    message_body_binary->length += length;
    return 0;
}

int create_batch_message(MESSAGE_HANDLE message, EVENTDATA_HANDLE* event_data_list, size_t event_count)
{
    int result = 0;
    size_t index;
    size_t length = 0;

    if (message_set_message_format(message, 0x80013700) != 0)
    {
        result = __LINE__;
    }
    else
    {
        for (index = 0; index < event_count; index++)
        {
            BINARY_DATA payload;
            BINARY_DATA body_data_binary;

            if (EventData_GetData(event_data_list[index], &payload.bytes, &payload.length) != EVENTDATA_OK)
            {
                break;
            }
            else
            {
                data data = { payload.bytes, payload.length };
                AMQP_VALUE data_value = amqpvalue_create_data(data);
                if (data_value == NULL)
                {
                    break;
                }
                else
                {
                    size_t payload_length;
                    bool is_error = false;

                    if (amqpvalue_get_encoded_size(data_value, &payload_length) != 0)
                    {
                        is_error = true;
                    }
                    else
                    {
                        body_data_binary.length = 0;
                        body_data_binary.bytes = (unsigned char*)malloc(payload_length);
                        if (body_data_binary.bytes == NULL)
                        {
                            is_error = true;
                        }
                        else
                        {
                            if (amqpvalue_encode(data_value, &encode_callback, &body_data_binary) != 0)
                            {
                                is_error = true;
                            }
                            else
                            {
                                if (message_add_body_amqp_data(message, body_data_binary) != 0)
                                {
                                    is_error = true;
                                }
                            }
                        }
                    }

                    amqpvalue_destroy(data_value);

                    if (is_error)
                    {
                        break;
                    }
                }
            }
        }

        if (index < event_count)
        {
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
    }

    return result;
}

void EventHubClient_LL_DoWork(EVENTHUBCLIENT_LL_HANDLE eventhub_client_ll)
{
    /* Codes_SRS_EVENTHUBCLIENT_LL_04_018: [if parameter eventhub_client_ll is NULL EventHubClient_LL_DoWork shall immediately return.]   */
    if (eventhub_client_ll != NULL)
    {
        /* Codes_SRS_EVENTHUBCLIENT_LL_01_079: [EventHubClient_LL_DoWork shall bring up the uAMQP stack if it has not already brought up:] */
        if ((eventhub_client_ll->message_sender == NULL) && (initialize_uamqp_stack(eventhub_client_ll) != 0))
        {
            LogError("Error initializing uamqp stack.\r\n");
        }
        else
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_01_038: [EventHubClient_LL_DoWork shall perform a messagesender_open if the state of the message_sender is not OPEN.] */
            if ((eventhub_client_ll->message_sender_state == MESSAGE_SENDER_STATE_IDLE) && (messagesender_open(eventhub_client_ll->message_sender) != 0))
            {
                /* Codes_SRS_EVENTHUBCLIENT_LL_01_039: [If messagesender_open fails, no further actions shall be carried out.] */
                LogError("Error opening message sender.\r\n");
            }
            else
            {
                PDLIST_ENTRY currentListEntry;

                currentListEntry = eventhub_client_ll->outgoingEvents.Flink;
                while (currentListEntry != &(eventhub_client_ll->outgoingEvents))
                {
                    PEVENTHUB_EVENT_LIST currentEvent = containingRecord(currentListEntry, EVENTHUB_EVENT_LIST, entry);
                    PDLIST_ENTRY next_list_entry = currentListEntry->Flink;

                    if (currentEvent->currentStatus == WAITING_TO_BE_SENT)
                    {
                        bool is_error = false;

                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_049: [If the message has not yet been given to uAMQP then a new message shall be created by calling message_create.] */
                        MESSAGE_HANDLE message = message_create();
                        if (message == NULL)
                        {
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_070: [If creating the message fails, then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
                            LogError("Error creating the uAMQP message.\r\n");
                            is_error = true;
                        }
                        else
                        {
                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_050: [If the number of event data entries for the message is 1 (not batched) then the message body shall be set to the event data payload by calling message_add_body_amqp_data.] */
                            if (currentEvent->eventCount == 1)
                            {
                                BINARY_DATA body;
                                const char* const* property_keys;
                                const char* const* property_values;
                                size_t property_count;
                                MAP_HANDLE properties_map;

                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_051: [The pointer to the payload and its length shall be obtained by calling EventData_GetData.] */
                                if (EventData_GetData(currentEvent->eventDataList[0], &body.bytes, &body.length) != EVENTDATA_OK)
                                {
                                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_052: [If EventData_GetData fails then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
                                    LogError("Error getting event data.\r\n");
                                    is_error = true;
                                }
                                else if (message_add_body_amqp_data(message, body) != 0)
                                {
                                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_071: [If message_add_body_amqp_data fails then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
                                    LogError("Cannot get the properties map.\r\n");
                                    is_error = true;
                                }
                                else if ((properties_map = EventData_Properties(currentEvent->eventDataList[0])) == NULL)
                                {
                                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
                                    LogError("Cannot get the properties map.\r\n");
                                    is_error = true;
                                }
                                else if (Map_GetInternals(properties_map, &property_keys, &property_values, &property_count) != MAP_OK)
                                {
                                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
                                    LogError("Cannot get the properties map.\r\n");
                                    is_error = true;
                                }
                                else
                                {
                                    if (property_count > 0)
                                    {
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_054: [If the number of event data entries for the message is 1 (not batched) the event data properties shall be added as application properties to the message.] */
                                        /* Codes_SRS_EVENTHUBCLIENT_LL_01_055: [A map shall be created to hold the application properties by calling amqpvalue_create_map.] */
                                        AMQP_VALUE properties_uamqp_map = amqpvalue_create_map();
                                        if (properties_uamqp_map == NULL)
                                        {
                                            LogError("Cannot build uAMQP properties map.\r\n");
                                            is_error = true;
                                        }
                                        else
                                        {
                                            size_t i;

                                            for (i = 0; i < property_count; i++)
                                            {
                                                AMQP_VALUE property_key;
                                                AMQP_VALUE property_value;

                                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_056: [For each property a key and value AMQP value shall be created by calling amqpvalue_create_string.] */
                                                if ((property_key = amqpvalue_create_string(property_keys[i])) == NULL)
                                                {
                                                    break;
                                                }

                                                if ((property_value = amqpvalue_create_string(property_values[i])) == NULL)
                                                {
                                                    amqpvalue_destroy(property_key);
                                                    break;
                                                }

                                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_057: [Then each property shall be added to the application properties map by calling amqpvalue_set_map_value.] */
                                                if (amqpvalue_set_map_value(properties_uamqp_map, property_key, property_value) != 0)
                                                {
                                                    amqpvalue_destroy(property_key);
                                                    amqpvalue_destroy(property_value);
                                                    break;
                                                }

                                                amqpvalue_destroy(property_key);
                                                amqpvalue_destroy(property_value);
                                            }

                                            if (i < property_count)
                                            {
                                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
                                                LogError("Could not fill all properties in the uAMQP properties map.\r\n");
                                                is_error = true;
                                            }
                                            /* Codes_SRS_EVENTHUBCLIENT_LL_01_058: [The resulting map shall be set as the message application properties by calling message_set_application_properties.] */
                                            else
                                            {
                                                if (message_set_application_properties(message, properties_uamqp_map) != 0)
                                                {
                                                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
                                                    LogError("Could not set message application properties on the message.\r\n");
                                                    is_error = true;
                                                }
                                            }

                                            amqpvalue_destroy(properties_uamqp_map);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (create_batch_message(message, currentEvent->eventDataList, currentEvent->eventCount) != 0)
                                {

                                }
                            }

                            if (!is_error)
                            {
                                currentEvent->currentStatus = WAITING_FOR_ACK;

                                /* Codes_SRS_EVENTHUBCLIENT_LL_01_069: [The AMQP message shall be given to uAMQP by calling messagesender_send, while passing as arguments the message sender handle, the message handle, a callback function and its context.] */
                                if (messagesender_send(eventhub_client_ll->message_sender, message, on_message_send_complete, currentListEntry) != 0)
                                {
                                    /* Codes_SRS_EVENTHUBCLIENT_LL_01_053: [If messagesender_send failed then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
                                    is_error = true;
                                    LogError("messagesender_send failed.\r\n");
                                }
                            }

                            message_destroy(message);
                        }

                        if (is_error)
                        {
                            size_t index;

                            currentEvent->callback(EVENTHUBCLIENT_CONFIRMATION_ERROR, currentEvent->context);

                            for (index = 0; index < currentEvent->eventCount; index++)
                            {
                                EventData_Destroy(currentEvent->eventDataList[index]);
                            }

                            DList_RemoveEntryList(currentListEntry);
                            free(currentEvent->eventDataList);
                            free(currentEvent);
                        }
                    }

                    currentListEntry = next_list_entry;
                }

                /* Codes_SRS_EVENTHUBCLIENT_LL_01_064: [EventHubClient_LL_DoWork shall call connection_dowork while passing as argument the connection handle obtained in EventHubClient_LL_Create.] */
                connection_dowork(eventhub_client_ll->connection);
            }
        }
    }
}
