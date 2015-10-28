/*
Microsoft Azure IoT Device Libraries
Copyright (c) Microsoft Corporation
All rights reserved.
MIT License
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the Software), to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "gballoc.h"

#include <string.h>
#include "eventhubclient_ll.h"
#include "iot_logging.h"
#include "strings.h"
#include "string_tokenizer.h"
#include "urlencode.h"
#include "version.h"
#include "crt_abstractions.h"

#include "proton/message.h"
#include "proton/messenger.h"
#include "proton/codec.h"

#include "doublylinkedlist.h"

#define LOG_ERROR LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, result));
DEFINE_ENUM_STRINGS(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_RESULT_VALUES)

#define ALL_MESSAGES_IN_OUTGOING_QUEUE  -1
#define NUMBER_OF_MESSENGER_STOP_TRIES  10
#define PROTON_PROPERTY_HEADER_SIZE     12
#define AMQP_PACK_OVERHEAD              20
#define AMQP_MAX_MESSAGE_SIZE           (64*1024)

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
    pn_tracker_t tracker;
    DLIST_ENTRY entry;
} EVENTHUB_EVENT_LIST, *PEVENTHUB_EVENT_LIST;

typedef struct EVENTHUBCLIENT_LL_STRUCT_TAG
{
    STRING_HANDLE keyName;
    STRING_HANDLE keyValue;
    STRING_HANDLE namespace;
    STRING_HANDLE eventHubpath;
    STRING_HANDLE amqpAddressStringHandle;
    pn_messenger_t* messenger;
    pn_message_t* message;
    size_t currentNumberOfMessageWaitingForAck;
    DLIST_ENTRY outgoingEvents;
} EVENTHUBCLIENT_LL_STRUCT;

static const char ENDPOINT_SUBSTRING[] = "Endpoint=sb://";
static const size_t ENDPOINT_SUBSTRING_LENGTH = sizeof(ENDPOINT_SUBSTRING) / sizeof(ENDPOINT_SUBSTRING[0]) - 1;
static const char SERVICEBUS_PATH_STRING[] = "servicebus.windows.net";
static const char SERVICEBUS_PATH_STRING_ALTERNATIVE[] = "servicebus.windows.net/";

static const char* PARTITION_KEY_NAME = "x-opt-partition-key";
static const char* SHARED_ACCESS_KEY_NAME = "SharedAccessKeyName";
static const int MESSENGER_TIMEOUT_IN_MILLISECONDS = 10000;

static int SetMessageBatchProperty(EVENTDATA_HANDLE eventDataHandle, char** protonProperties, size_t* propertyLen)
{
    int result;
    size_t index;
    const char*const* keys;
    const char*const* values;
    size_t propertyCount = 0;

    MAP_HANDLE mapProperties = EventData_Properties(eventDataHandle);
    if (mapProperties == NULL)
    {
        LogError("Failure pn_data.\r\n");
        result = __LINE__;
    }
    else if (Map_GetInternals(mapProperties, &keys, &values, &propertyCount) != MAP_OK)
    {
        LogError("Failure Map_GetInternals.\r\n");
        result = __LINE__;
    }
    else if (propertyCount > 0)
    {
        pn_data_t* propertyData;
        if ( (propertyData = pn_data(0) ) == NULL)
        {
            LogError("Failure pn_data.\r\n");
            result = __LINE__;
        }
        else if (pn_data_put_map(propertyData) != 0)
        {
            LogError("Failure pn_data_put_map.\r\n");
            result = __LINE__;
        }
        else if (!pn_data_enter(propertyData))
        {
            LogError("Failure pn_data_enter.\r\n");
            result = __LINE__;
        }
        else
        {
            *propertyLen = PROTON_PROPERTY_HEADER_SIZE;
            for (index = 0; index < propertyCount; index++)
            {
                *propertyLen += strlen(keys[index])+2;
                *propertyLen += strlen(values[index])+2;

                if (pn_data_put_symbol(propertyData, pn_bytes(strlen(keys[index]), keys[index])) != 0)
                {
                    LogError("Failure pn_data_put_symbol.\r\n");
                    break;
                }
                else if (pn_data_put_string(propertyData, pn_bytes(strlen(values[index]), values[index])) != 0)
                {
                        LogError("Failure pn_data_put_string.\r\n");
                    break;
                }
            }
            
            if (index != propertyCount)
            {
                result = __LINE__;
            }
            else if (!pn_data_exit(propertyData))
            {
                LogError("Failure pn_data_exit.\r\n");
                result = __LINE__;
            }
            else
            {
                char* insertPos;
                char* tmpValue = malloc(*propertyLen);
                if (tmpValue == NULL)
                {
                    LogError("Failure pn_data_encode.\r\n");
                    result = __LINE__;
                }
                else
                {
                    tmpValue[0] = 0x00; // this is a constructor of type "/ %x00 descriptor constructor" - see AMQP specs fig 1.3
                    tmpValue[1] = 0x53; // this is a "fixed one" type, see AMQP specs fig 1.3
                    tmpValue[2] = 0x74; // this is "Data" see AMQP specs 3.2.6*/
                    insertPos = tmpValue+3;

                    ssize_t sizeOfProperty = pn_data_encode(propertyData, insertPos, *propertyLen-3);
                    if (sizeOfProperty == 0)
                    {
                        LogError("Failure pn_data_encode.\r\n");
                        free(tmpValue);
                        result = __LINE__;
                    }
                    else
                    {
                        *protonProperties = tmpValue;
                        result = 0;
                    }
                }
                pn_data_free(propertyData);
            }
        }
    }
    else
    {
        result = 0;
    }
    return result;
}

static int SetMessagePartitionKey(pn_message_t* message, EVENTDATA_HANDLE eventDataHandle)
{
    int result;
    const char* partitionKey = EventData_GetPartitionKey(eventDataHandle);
    if (partitionKey != NULL)
    {
        pn_data_t* partitionMsg = pn_message_annotations(message);
        if (partitionMsg == NULL)
        {
            result = __LINE__;
        }
        else if (pn_data_put_map(partitionMsg) != 0)
        {
            result = __LINE__;
        }
        else if (!pn_data_enter(partitionMsg))
        {
            result = __LINE__;
        }
        else if (pn_data_put_symbol(partitionMsg, pn_bytes(strlen(PARTITION_KEY_NAME), PARTITION_KEY_NAME)) != 0)
        {
            result = __LINE__;
        }
        else if (pn_data_put_string(partitionMsg, pn_bytes(strlen(partitionKey), partitionKey)) != 0)
        {
            result = __LINE__;
        }
        else
        {
            pn_data_exit(partitionMsg);
            result = 0;
        }
    }
    else
    {
        result = 0;
    }
    return result;
}

/* Codes_SRS_EVENTHUBCLIENT_LL_04_027: [EventHubClient_LL_Destroy shall retry at least 10 times to stop the messenger if it could not be stopped at the first pn_messenger_stop call.] */
static bool amqpStopMessenger(pn_messenger_t* messenger)
{
    bool succeeded;
    int result;
    
    result = pn_messenger_stop(messenger);
    if (result == 0)
    {
        succeeded = true;
    }
    else if (result != PN_INPROGRESS)
    {
        succeeded = false;
    }
    else
    {
        size_t numberOfTrys;
        succeeded = false;
        /* Codes_SRS_EVENTHUBCLIENT_LL_04_027: [EventHubClient_LL_Destroy shall retry at least 10 times to stop the messenger if it could not be stopped at the first pn_messenger_stop call.] */
        for (numberOfTrys = NUMBER_OF_MESSENGER_STOP_TRIES; numberOfTrys > 0; numberOfTrys--)
        {
            pn_messenger_work(messenger, 100);
            if (pn_messenger_stopped(messenger))
            {
                succeeded = true;
                break;
            }
        }
    }
    return succeeded;
}
EVENTHUBCLIENT_LL_HANDLE EventHubClient_LL_CreateFromConnectionString(const char* connectionString, const char* eventHubPath)
{
    EVENTHUBCLIENT_LL_STRUCT* ehLLStruct = NULL;
    STRING_HANDLE connection_STRING = NULL;
    STRING_HANDLE tokenString = NULL;
    STRING_TOKENIZER_HANDLE tokenizer = NULL;
    STRING_HANDLE namespaceStringHandle = NULL;
    STRING_HANDLE keyNameStringHandle = NULL;
    STRING_HANDLE keyValueStringHandle = NULL;
    STRING_HANDLE eventHubPathStringHandle = NULL;

    /* Codes_SRS_EVENTHUBCLIENT_LL_05_001: [EventHubClient_LL_CreateFromConnectionString shall obtain the version string by a call to EventHubClient_GetVersionString.] */
    /* Codes_SRS_EVENTHUBCLIENT_LL_05_002: [EventHubClient_LL_CreateFromConnectionString shall print the version string to standard output.]*/
    LogInfo("Event Hubs Client SDK for C, version %s\r\n", EventHubClient_GetVersionString());

    /* Codes_SRS_EVENTHUBCLIENT_LL_03_003: [EventHubClient_ CreateFromConnectionString shall return a NULL value if connectionString or path is NULL.] */
    if (connectionString == NULL || eventHubPath == NULL)
    {
        LogError("Invalid Argument. result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG));
    }
    /* Codes_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    else if (strncmp(connectionString, ENDPOINT_SUBSTRING, ENDPOINT_SUBSTRING_LENGTH) != 0) /* Making sure the connection String Starts with Endpoint=sb:// */
    {
        LogError("Invalid Connection String. result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_CONNECTION_STRING));
    }
    else if ((connection_STRING = STRING_construct(connectionString + ENDPOINT_SUBSTRING_LENGTH)) == NULL)
    {
        LogError("Error creating connection String.\r\n");
    }
    else if ((tokenizer = STRING_TOKENIZER_create(connection_STRING)) == NULL)
    {
        LogError("Error creating Tokenizer.\r\n");
    }
    else if ((tokenString = STRING_new()) == NULL)
    {
        LogError("Error creating Token String.\r\n");
    }
    /* Codes_SRS_EVENTHUBCLIENT_LL_03_017: [EventHubClient_ll expects a service bus connection string in one of the following formats:
    Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]
    Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[key name];SharedAccessKey=[key value] ]*/
    /* Codes_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    /* Codes_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    else if (STRING_TOKENIZER_get_next_token(tokenizer, tokenString, ".") != 0) /*looking for the servicebus namespace */
    {
        LogError("Couldn't find namespace name.result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_CONNECTION_STRING));
    }
    /* Codes_SRS_EVENTHUBCLIENT_LL_03_002: [EventHubClient_LL_CreateFromConnectionString shall allocate the internal structures required by this module.] */
    else if ((ehLLStruct = malloc(sizeof(EVENTHUBCLIENT_LL_STRUCT))) == NULL)
    {
        LogError("Memory Allocation Failed for ehLLStruct.\r\n");
    }
    else
    {
        bool clearehLLStruct = false;
        DList_InitializeListHead(&(ehLLStruct->outgoingEvents));
        ehLLStruct->keyName = NULL;
        ehLLStruct->keyValue = NULL;
        ehLLStruct->namespace = NULL;
        ehLLStruct->eventHubpath = NULL;
        ehLLStruct->amqpAddressStringHandle = NULL;
        ehLLStruct->currentNumberOfMessageWaitingForAck = 0;

        if ((namespaceStringHandle = STRING_clone(tokenString)) == NULL)
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR));
        }
        /* Codes_SRS_EVENTHUBCLIENT_LL_03_020: [EventHubClient_LL_CreateFromConnectionString shall ensure that the keyname, keyvalue, namespace and eventhubpath are all URL encoded.] */
        else if ((ehLLStruct->namespace = URL_Encode(namespaceStringHandle)) == NULL)
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_03_021: [EventHubClient_LL_CreateFromConnectionString shall return EVENTHUBCLIENT_URL_ENCODING_FAILURE if any failure is encountered during the URL encoding.] */
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_URL_ENCODING_FAILURE));
        }
        else if (STRING_TOKENIZER_get_next_token(tokenizer, tokenString, ";") != 0)  /* looking for "servicebus.windows.net" or "servicebus.windows.net/" */
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_CONNECTION_STRING));
        }
        else if ((strcmp(STRING_c_str(tokenString), SERVICEBUS_PATH_STRING_ALTERNATIVE) != 0) &&
            (strcmp(STRING_c_str(tokenString), SERVICEBUS_PATH_STRING) != 0)) /* Check the  different patterns of Connection String we support.*/
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_CONNECTION_STRING));
        }
        else if (STRING_TOKENIZER_get_next_token(tokenizer, tokenString, "=") != 0)  /* looking for the "SharedAccessKeyName" */
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_CONNECTION_STRING));
        }
        else if (strcmp(STRING_c_str(tokenString), SHARED_ACCESS_KEY_NAME) != 0)
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_CONNECTION_STRING));
        }
        else if (STRING_TOKENIZER_get_next_token(tokenizer, tokenString, ";") != 0)
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_CONNECTION_STRING));
        }
        else if ((keyNameStringHandle = STRING_clone(tokenString)) == NULL) /* looking for the key name*/
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR));
        }
        /* Codes_SRS_EVENTHUBCLIENT_LL_03_020: [EventHubClient_LL_CreateFromConnectionString shall ensure that the keyname, keyvalue, namespace and eventhubpath are all URL encoded.] */
        else if ((ehLLStruct->keyName = URL_Encode(keyNameStringHandle)) == NULL)
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_03_021: [EventHubClient_LL_CreateFromConnectionString shall return EVENTHUBCLIENT_URL_ENCODING_FAILURE if any failure is encountered during the URL encoding.] */
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_URL_ENCODING_FAILURE));
        }
        else if (STRING_TOKENIZER_get_next_token(tokenizer, tokenString, "=") != 0)   /* looking for the "SharedAccessKey" */
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_CONNECTION_STRING));
        }
        else if (strcmp(STRING_c_str(tokenString), "SharedAccessKey") != 0)
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_CONNECTION_STRING));
        }
        else if (STRING_TOKENIZER_get_next_token(tokenizer, tokenString, ";") != 0) /* looking for the Key value */
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_CONNECTION_STRING));
        }
        else if ((keyValueStringHandle = STRING_clone(tokenString)) == NULL)
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR));
        }
        /* Codes_SRS_EVENTHUBCLIENT_LL_03_020: [EventHubClient_LL_CreateFromConnectionString shall ensure that the keyname, keyvalue, namespace and eventhubpath are all URL encoded.] */
        else if ((ehLLStruct->keyValue = URL_Encode(keyValueStringHandle)) == NULL)
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_03_021: [EventHubClient_LL_CreateFromConnectionString shall return EVENTHUBCLIENT_URL_ENCODING_FAILURE if any failure is encountered during the URL encoding.] */
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_URL_ENCODING_FAILURE));
        }
        else if ((eventHubPathStringHandle = STRING_construct(eventHubPath)) == NULL)
        {
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR));
        }
        /* Codes_SRS_EVENTHUBCLIENT_LL_03_020: [EventHubClient_LL_CreateFromConnectionString shall ensure that the keyname, keyvalue, namespace and eventhubpath are all URL encoded.] */
        else if ((ehLLStruct->eventHubpath = URL_Encode(eventHubPathStringHandle)) == NULL)
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_03_021: [EventHubClient_LL_CreateFromConnectionString shall return EVENTHUBCLIENT_URL_ENCODING_FAILURE if any failure is encountered during the URL encoding.] */
            clearehLLStruct = true;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_URL_ENCODING_FAILURE));
        }
        /* Codes_SRS_EVENTHUBCLIENT_LL_03_029: [EventHubClient_ CreateFromConnectionString shall create a proton messenger via a call to pn_messenger.] */
        else if ((ehLLStruct->messenger = pn_messenger(NULL)) == NULL)
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_03_039: [EventHubClient_ CreateFromConnectionString shall return EVENTHUBCLIENT_ERROR if it fails to create a proton messenger.] */
            clearehLLStruct = true;
            LogError("pn_messenger failed.\r\n");
        }
        /* Codes_SRS_EVENTHUBCLIENT_LL_04_001: [EventHubClient_LL_CreateFromConnectionString shall call pn_messenger_set_outgoing_window so it can get the status of the message sent] */
        else if (pn_messenger_set_outgoing_window(ehLLStruct->messenger, OUTGOING_WINDOW_SIZE) != 0)
        {
            pn_messenger_free(ehLLStruct->messenger);
            /* Codes_SRS_EVENTHUBCLIENT_LL_04_002: [If EventHubClient_LL_CreateFromConnectionString fail is shall return  NULL;] */
            clearehLLStruct = true;
            LogError("pn_messenger_set_outgoing_window failed.\r\n");
        }
        /* Codes_SRS_EVENTHUBCLIENT_LL_04_010: [EventHubClient_LL_CreateFromConnectionString shall setup the global messenger to be blocking via a call to pn_messenger_set_blocking.] */
        else if (pn_messenger_set_blocking(ehLLStruct->messenger, false) != 0)
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_03_033: [EventHubClient_LL_Send shall return EVENTHUBCLIENT_ERROR for any error encountered while invoking messenger functionalities.] */
            pn_messenger_free(ehLLStruct->messenger);
            clearehLLStruct = true;
            LogError("pn_messenger_set_blocking failed.\r\n");
        }
        /* Codes_SRS_EVENTHUBCLIENT_LL_04_009: [EventHubClient_LL_CreateFromConnectionString shall specify a timeout value of 10 seconds to the proton messenger via a call to pn_messenger_set_timeout.]  */
        else if (pn_messenger_set_timeout(ehLLStruct->messenger, MESSENGER_TIMEOUT_IN_MILLISECONDS) != 0)
        {
            clearehLLStruct = true;
            pn_messenger_free(ehLLStruct->messenger);
            LogError("pn_messenger_set_timeout failed.\r\n");
        }
        /* Codes_SRS_EVENTHUBCLIENT_LL_03_030: [EventHubClient_LL_CreateFromConnectionString shall then start the messenger invoking pn_messenger_start.] */
        else if (pn_messenger_start(ehLLStruct->messenger) != 0)
        {
            /* Codes_SRS_EVENTHUBCLIENT_LL_04_002: [If EventHubClient_LL_CreateFromConnectionString fail is shall return  NULL;]  */
            clearehLLStruct = true;
            pn_messenger_free(ehLLStruct->messenger);
            LogError("pn_messenger_start failed.\r\n");
        }
        else if ((ehLLStruct->amqpAddressStringHandle = STRING_construct("amqps://")) == NULL)
        {
            clearehLLStruct = true;
            pn_messenger_free(ehLLStruct->messenger);
            LogError("Failed trying to build the AMQP Address String.\r\n");
        }
        else if (STRING_concat_with_STRING(ehLLStruct->amqpAddressStringHandle, ehLLStruct->keyName) != 0 ||
            STRING_concat(ehLLStruct->amqpAddressStringHandle, ":") != 0 ||
            STRING_concat_with_STRING(ehLLStruct->amqpAddressStringHandle, ehLLStruct->keyValue) != 0 ||
            STRING_concat(ehLLStruct->amqpAddressStringHandle, "@") != 0 ||
            STRING_concat_with_STRING(ehLLStruct->amqpAddressStringHandle, ehLLStruct->namespace) != 0 ||
            STRING_concat(ehLLStruct->amqpAddressStringHandle, ".servicebus.windows.net/") != 0 ||
            STRING_concat_with_STRING(ehLLStruct->amqpAddressStringHandle, ehLLStruct->eventHubpath))
        {
            clearehLLStruct = true;
            pn_messenger_free(ehLLStruct->messenger);
            LogError("Failed trying to build the AMQP Address String.\r\n");
        }
        else if ((ehLLStruct->message = pn_message()) == NULL)
        {
            clearehLLStruct = true;
            pn_messenger_free(ehLLStruct->messenger);
            LogError("Failed trying to build the AMQP Address String.\r\n");
        }

        if (clearehLLStruct == true)
        {
            STRING_delete(ehLLStruct->keyValue);
            STRING_delete(ehLLStruct->keyName);
            STRING_delete(ehLLStruct->namespace);
            STRING_delete(ehLLStruct->amqpAddressStringHandle);
            free(ehLLStruct);
            ehLLStruct = NULL;
        }
        STRING_delete(namespaceStringHandle);
        STRING_delete(keyNameStringHandle);
        STRING_delete(keyValueStringHandle);
        STRING_delete(eventHubPathStringHandle);
    }

    STRING_delete(connection_STRING);
    STRING_delete(tokenString);
    STRING_TOKENIZER_destroy(tokenizer);

    return ((EVENTHUBCLIENT_LL_HANDLE)ehLLStruct);
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

static EVENTHUBCLIENT_RESULT GenerateMessagePayload(EVENTHUBCLIENT_LL_STRUCT* eventHubClientLLStruct, EVENTDATA_HANDLE* eventDataList, size_t count, pn_tracker_t* tracker)
{
    EVENTHUBCLIENT_RESULT result;
    pn_data_t* body = NULL;

    //Fist Clear the current message;
    pn_message_clear(eventHubClientLLStruct->message);

    if (pn_message_set_address(eventHubClientLLStruct->message, STRING_c_str(eventHubClientLLStruct->amqpAddressStringHandle)) != 0)
    {
        result = EVENTHUBCLIENT_ERROR;
        LogError("pn_message_set_address failed.\r\n");
    }
    else if (pn_message_set_inferred(eventHubClientLLStruct->message, true) != 0)
    {
        result = EVENTHUBCLIENT_ERROR;
        LogError("pn_message_set_inferred failed.\r\n");
    }
    // Use the first EventData Item in the list since by spec they all have to be the same
    else if (SetMessagePartitionKey(eventHubClientLLStruct->message, eventDataList[0]) != 0)
    {
        result = EVENTHUBCLIENT_ERROR;
    }
    else if ((body = pn_message_body(eventHubClientLLStruct->message)) == NULL)
    {
        result = EVENTHUBCLIENT_ERROR;
        LogError("pn_message_body failed.\r\n");
    }
    else
    {
        int tempResult = 0;
        size_t totalSize = 0;
        size_t index;
        for (index = 0; index < count; index++)
        {
            unsigned char* data = NULL;
            char* propValues = NULL;
            size_t dataLength = 0;
            pn_data_t* innerBinary;

            size_t propertyLength = 0;

            // Get the Properties, may return NULL if not no properties
            if (SetMessageBatchProperty(eventDataList[index], &propValues, &propertyLength) != 0)
            {
                tempResult = EVENTHUBCLIENT_ERROR;
                break;
            }
            else if (EventData_GetData(eventDataList[index], (const unsigned char**)&data, &dataLength) != EVENTDATA_OK)
            {
                tempResult = EVENTHUBCLIENT_ERROR;
                free(propValues);
                LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_EVENT_DATA_FAILURE) );
                break;
            }
            else
            {
                // Check the total size to make sure we don't go over proton's limits
                totalSize += propertyLength+dataLength+AMQP_PACK_OVERHEAD;
                if (totalSize > AMQP_MAX_MESSAGE_SIZE)
                {
                    tempResult = EVENTHUBCLIENT_DATA_SIZE_EXCEEDED;
                    free(propValues);
                    LogError("The message (%lu bytes) exceeds the limit currently allowed on the link.\r\n", totalSize);
                    break;
                }
                else if ( (innerBinary = pn_data(0) ) == NULL)
                {
                    tempResult = EVENTHUBCLIENT_ERROR;
                    free(propValues);
                    LogError("pn_data failed failed.\r\n");
                    break;
                }
                else
                {
                    pn_bytes_t protonBytes = pn_bytes(dataLength, (const char*)data);
                    if (pn_data_put_binary(innerBinary, protonBytes) != 0)
                    {
                        tempResult = EVENTHUBCLIENT_ERROR;
                        free(propValues);
                        LogError("pn_data_put_binary failed.\r\n");
                        pn_data_free(innerBinary);
                        break;
                    }
                    else
                    {
                        /*5 because string = 0xA0 + 1 byte for length or 0xA1 and 4 bytes for length */
                        /*3 because a section is encoded as 0x00, 0x53, 0x75*/
                        char* iterator;
                        char* out1 = (char*)malloc(dataLength + 5 + 3 + propertyLength);
                        if (out1 == NULL)
                        {
                            tempResult = EVENTHUBCLIENT_ERROR;
                            free(propValues);
                            LogError("Memory Allocation Failure.\r\n");
                            pn_data_free(innerBinary);
                            break;
                        }
                        else
                        {
                            iterator = out1;
                            if (propertyLength > 0)
                            {
                                memcpy(iterator, propValues, propertyLength);
                                iterator += propertyLength;
                            }
                            free(propValues);
                            iterator[0] = 0x00; /*this is a constructor of type "/ %x00 descriptor constructor" - see AMQP specs fig 1.3*/
                            iterator[1] = 0x53; /*this is a "fixed one" type, see AMQP specs fig 1.3*/
                            iterator[2] = 0x75; /*this is "Data" see AMQP specs 3.2.6*/
                            ssize_t encodeLen = pn_data_encode(innerBinary, iterator + 3, dataLength + 3 + 5);
                            if (encodeLen < 0)
                            {
                                tempResult = EVENTHUBCLIENT_ERROR;
                                LogError("pn_data_encode failed.\r\n");
                                pn_data_free(innerBinary);
                                free(out1);
                                break;
                            }
                            else
                            {
                                pn_bytes_t sectionAsBinary;
                                sectionAsBinary.size = encodeLen + 3 + propertyLength;
                                sectionAsBinary.start = out1;
                                if (pn_data_put_binary(body, sectionAsBinary) != 0)
                                {
                                    tempResult = EVENTHUBCLIENT_ERROR;
                                    LogError("pn_data_put_binary failed.\r\n");
                                    pn_data_free(innerBinary);
                                    free(out1);
                                    break;
                                }
                                else
                                {
                                    /*all is fine so far*/
                                }
                            }
                            free(out1);
                        }
                    }
                    pn_data_free(innerBinary);
                }
            }
        }

        if (index < count)
        {
            result = tempResult;
        }
        else
        {
            if (pn_messenger_put(eventHubClientLLStruct->messenger, eventHubClientLLStruct->message) != 0)
            {
                result = EVENTHUBCLIENT_ERROR;
                LogError("pn_messenger_put failed.\r\n");
            }
            else
            {
                *tracker = pn_messenger_outgoing_tracker(eventHubClientLLStruct->messenger);
                result = EVENTHUBCLIENT_OK;
            }
        }
    }
    return result;
}

void EventHubClient_LL_Destroy(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle)
{
    /* Codes_SRS_EVENTHUBCLIENT_LL_03_010: [If the eventHubClientLLHandle is NULL, EventHubClient_LL_Destroy shall not do anything.] */
    if (eventHubClientLLHandle != NULL)
    {
        PDLIST_ENTRY unsend;
        /* Codes_SRS_EVENTHUBCLIENT_LL_03_009: [EventHubClient_LL_Destroy shall terminate the usage of this EventHubClient specified by the eventHubClientLLHandle and cleanup all associated resources.] */
        EVENTHUBCLIENT_LL_STRUCT* ehLLStruct = (EVENTHUBCLIENT_LL_STRUCT*)eventHubClientLLHandle;
        STRING_delete(ehLLStruct->keyName);
        STRING_delete(ehLLStruct->keyValue);
        STRING_delete(ehLLStruct->namespace);
        STRING_delete(ehLLStruct->eventHubpath);
        STRING_delete(ehLLStruct->amqpAddressStringHandle);

        /* Codes_SRS_EVENTHUBCLIENT_LL_04_017: [EventHubClient_LL_Destroy shall complete all the event notifications callbacks that are in the outgoingdestroy the outgoingEvents with the result EVENTHUBCLIENT_CONFIRMATION_DESTROY.] */
        while ((unsend = DList_RemoveHeadList(&(ehLLStruct->outgoingEvents))) != &(ehLLStruct->outgoingEvents))
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

        /* Codes_SRS_EVENTHUBCLIENT_LL_03_034: [EventHubClient_LL_Destroy shall then stop the messenger by invoking pn_messenger_stop.] */		
        if (amqpStopMessenger(ehLLStruct->messenger) != true)
        {
            LogError("Error trying to stop the messenger.\r\n");
        }

        /* SRS_EVENTHUBCLIENT_LL_03_040: [EventHubClient_LL_Destroy shall clean the messenger resource via a call to pn_messenger_free.] */
        pn_messenger_free(ehLLStruct->messenger);

        pn_message_free(ehLLStruct->message);

        free(ehLLStruct);
    }
}

EVENTHUBCLIENT_RESULT EventHubClient_LL_SendAsync(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle, EVENTDATA_HANDLE eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncConfirmationCallback, void* userContextCallback)
{
   EVENTHUBCLIENT_RESULT result;

    /* Codes_SRS_EVENTHUBCLIENT_LL_04_011: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter eventHubClientLLHandle or eventDataHandle is NULL.] */
    /* Codes_SRS_EVENTHUBCLIENT_LL_04_012: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter telemetryConfirmationCallBack is NULL and userContextCallBack is not NULL.] */
    if (eventHubClientLLHandle == NULL || eventDataHandle == NULL || (sendAsyncConfirmationCallback == NULL && userContextCallback != NULL))
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
                    EVENTHUBCLIENT_LL_STRUCT* ehClientLLData = (EVENTHUBCLIENT_LL_STRUCT*)eventHubClientLLHandle;
                    /* Codes_SRS_EVENTHUBCLIENT_LL_04_013: [EventHubClient_LL_SendAsync shall add the DLIST outgoingEvents a new record cloning the information from eventDataHandle, telemetryConfirmationCallback and userContextCallBack.] */
                    newEntry->callback = sendAsyncConfirmationCallback;
                    newEntry->context = userContextCallback;
                    DList_InsertTailList(&(ehClientLLData->outgoingEvents), &(newEntry->entry));
                    /* Codes_SRS_EVENTHUBCLIENT_LL_04_015: [Otherwise EventHubClient_LL_SendAsync shall succeed and return EVENTHUBCLIENT_OK.] */
                    result = EVENTHUBCLIENT_OK;
                }
            }
        }
    }

    return result;
}

EVENTHUBCLIENT_RESULT EventHubClient_LL_SendBatchAsync(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle, EVENTDATA_HANDLE* eventDataList, size_t count, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncConfirmationCallback, void* userContextCallback)
{
    EVENTHUBCLIENT_RESULT result;
    /* Codes_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALLID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    if (eventHubClientLLHandle == NULL || eventDataList == NULL || count == 0 || (sendAsyncConfirmationCallback == NULL && userContextCallback != NULL))
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
                    /* Codes_SRS_EVENTHUBCLIENT_LL_07_014: [EventHubClient_LL_SendBatchAsync shall clone each item in the eventDataList and by calling EVENTDATA_Clone.] */
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
                        /* Codes_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shallreturn EVENTHUBCLIENT_ERROR for any Error that is encountered.] */
                        result = EVENTHUBCLIENT_ERROR;
                        free(newEntry->eventDataList);
                        free(newEntry);
                        LOG_ERROR;
                    }
                    else
                    {
                        EVENTHUBCLIENT_LL_STRUCT* ehClientLLData = (EVENTHUBCLIENT_LL_STRUCT*)eventHubClientLLHandle;
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

void EventHubClient_LL_DoWork(EVENTHUBCLIENT_LL_HANDLE eventHubClient_LL_Handle)
{
    EVENTHUBCLIENT_LL_STRUCT* ehStruct = (EVENTHUBCLIENT_LL_STRUCT*)eventHubClient_LL_Handle;
    /* Codes_SRS_EVENTHUBCLIENT_LL_04_018: [if parameter eventHubClientLLHandle is NULL  EventHubClient_LL_DoWork shall immediately return.]   */
    if (ehStruct != NULL)
    {
        PDLIST_ENTRY currentListEntry;
        /* Codes_SRS_EVENTHUBCLIENT_LL_04_019: [If the current status of the entry is WAITING_TO_BE_SENT and there is available spots on proton, defined by OUTGOING_WINDOW_SIZE, EventHubClient_LL_DoWork shall call create pn_message and put the message into messenger by calling pn_messenger_put.]  */
        currentListEntry = ehStruct->outgoingEvents.Flink;
        while (currentListEntry != &(ehStruct->outgoingEvents))
        {
            DLIST_ENTRY savedFromCurrentListEntry;
            PEVENTHUB_EVENT_LIST currentWork = containingRecord(currentListEntry, EVENTHUB_EVENT_LIST, entry);
            savedFromCurrentListEntry.Flink = currentListEntry->Flink; // We need to save this because it will be stomped on when we remove it from the list.

            if (currentWork->currentStatus == WAITING_FOR_ACK)
            {
                /* Codes_SRS_EVENTHUBCLIENT_LL_04_022: [If the current status of the entry is WAITING_FOR_ACK, than EventHubClient_LL_DoWork shall check status of this entry by calling pn_messenger_status.] */
                pn_state_t result;
                int settleReturnCode;
                EVENTHUBCLIENT_CONFIRMATION_RESULT confirmResult = EVENTHUBCLIENT_CONFIRMATION_OK;
                result = pn_messenger_status(ehStruct->messenger, currentWork->tracker);
                switch (result)
                {
                    case PN_STATUS_PENDING:
                        /* Codes_SRS_EVENTHUBCLIENT_LL_04_023: [If the status returned is PN_STATUS_PENDING EventHubClient_LL_DoWork shall do nothing.] */
                        break;
                    case PN_STATUS_UNKNOWN:
                        confirmResult = EVENTHUBCLIENT_CONFIRMATION_UNKNOWN;
                        break;
                    case PN_STATUS_ACCEPTED:
                        confirmResult = EVENTHUBCLIENT_CONFIRMATION_OK;
                        break;
                    default:
                        LogError("pn_messenger_status return an unexpected status of %d.\r\n", result);
                        confirmResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;
                        break;
                }

                if (result != PN_STATUS_PENDING)
                {
                    /* Codes_SRS_EVENTHUBCLIENT_LL_04_025: [If the status returned is any other EventHubClient_LL_DoWork shall call the callback (if exist) with CONFIRMATION_RESULT value of EVENTHUBCLIENT_CONFIRMATION_ERROR and remove the event from the list.]*/
                    if (currentWork->callback != NULL)
                    {
                        currentWork->callback(confirmResult, currentWork->context);
                    }
                    /* Codes_SRS_EVENTHUBCLIENT_LL_04_026: [EventHubClient_LL_DoWork shall free the tracker before removing the event from the list by calling pn_messenger_settle] */
                    if ( (settleReturnCode = pn_messenger_settle(ehStruct->messenger, currentWork->tracker, 0)) != 0)
                    {
                        LogError("pn_messenger_settle failed. Error Code: %d\r\n", settleReturnCode);
                    }
                    DList_RemoveEntryList(currentListEntry);
                    ehStruct->currentNumberOfMessageWaitingForAck--;
                    for (size_t index = 0; index < currentWork->dataCount; index++)
                    {
                        EventData_Destroy(currentWork->eventDataList[index]);
                    }
                    free(currentWork->eventDataList);
                    free(currentWork);
                    // Move the current entry to the saved entry
                    currentListEntry = savedFromCurrentListEntry.Flink;
                }
                else
                {
                    // Move the current entry to the next entry
                    currentListEntry = currentListEntry->Flink;
                }
            }
            else if (ehStruct->currentNumberOfMessageWaitingForAck >= OUTGOING_WINDOW_BUFFER)
            {
                // Move the current entry to the next entry
                currentListEntry = currentListEntry->Flink;
                break;
            }
            else if (currentWork->currentStatus == WAITING_TO_BE_SENT)
            {
                /* Codes_SRS_EVENTHUBCLIENT_LL_04_020: [If EventHubClient_LL_DoWork fails to create/put and get tracker of a proton message, it shall fail the payload and send the callback EVENTHUBCLIENT_CONFIRMATION_ERROR.] */
                EVENTHUBCLIENT_RESULT payloadResult = GenerateMessagePayload(ehStruct, currentWork->eventDataList, currentWork->dataCount, &currentWork->tracker);
                if (payloadResult != EVENTHUBCLIENT_OK)
                {
                    /* Codes_SRS_EVENTHUBCLIENT_LL_07_016: [If the total data size of the message exceeds the AMQP_MAX_MESSAGE_SIZE, then the message shall fail and the callback will be called with EVENTHUBCLIENT_CONFIRMATION_EXCEED_MAX_SIZE.] */
                    if (currentWork->callback != NULL)
                    {
                        currentWork->callback( (payloadResult == EVENTHUBCLIENT_ERROR) ? EVENTHUBCLIENT_CONFIRMATION_ERROR : EVENTHUBCLIENT_CONFIRMATION_EXCEED_MAX_SIZE, currentWork->context);
                    }

                    DList_RemoveEntryList(currentListEntry);
                    for (size_t index = 0; index < currentWork->dataCount; index++)
                    {
                        EventData_Destroy(currentWork->eventDataList[index]);
                    }
                    free(currentWork->eventDataList);
                    free(currentWork);
                    // Move the current entry to the saved entry
                    currentListEntry = savedFromCurrentListEntry.Flink;
                }
                else
                {
                    ehStruct->currentNumberOfMessageWaitingForAck++;
                    currentWork->currentStatus = WAITING_FOR_ACK;
                    // Move the current entry to the next entry
                    currentListEntry = currentListEntry->Flink;
                }
            }
        }

        /* Codes_SRS_EVENTHUBCLIENT_LL_04_021: [If there are message to be sent, EventHubClient_LL_DoWork shall call pn_messenger_send with parameter -1.] */
        if (ehStruct->currentNumberOfMessageWaitingForAck > 0)
        {
            int tempReturnCode = pn_messenger_send(ehStruct->messenger, ALL_MESSAGES_IN_OUTGOING_QUEUE);
            if (tempReturnCode != PN_INPROGRESS && tempReturnCode != 0 )
            {
                LogError("pn_messenger_send failed with Error Code: %d.\r\n", tempReturnCode);
            }
        }
    }
}