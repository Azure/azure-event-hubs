// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdarg.h>

#include "eventhubclient.h"
#include "eventdata.h"
#include "send.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"

static const char* connectionString = "Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]";
static const char* eventHubPath = "[event hub name]";

static bool g_bSendProperties = false;
static bool g_bSendPartitionKey = false;

static const char PARTITION_KEY_INFO[] = "PartitionKeyInfo";
static const char TEST_STRING_VALUE_1[] = "Property_String_Value_1";
static const char TEST_STRING_VALUE_2[] = "Property_String_Value_2";

#define SLEEP_TIME		1000
#define BUFFER_SIZE     128
static size_t g_id = 1000;

DEFINE_ENUM_STRINGS(EVENTHUBCLIENT_STATE, EVENTHUBCLIENT_STATE_VALUES);
DEFINE_ENUM_STRINGS(EVENTHUBCLIENT_ERROR_RESULT, EVENTHUBCLIENT_ERROR_RESULT_VALUES);

void custom_logging_function(LOG_CATEGORY log_category, const char* file, const char* func, const int line, unsigned int options, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    switch (log_category)
    {
        case LOG_INFO:
            (void)printf("Custom Info: ");
            break;
        case LOG_ERROR:
            (void)printf("Custom Error: File:%s Func:%s Line:%d ", file, func, line);
            break;
        default:
            break;
    }
    (void)vprintf(format, args);
    va_end(args);

    if (options & LOG_LINE)
    {
        (void)printf("\r\n");
    }
}

void eventhub_state_change_callback(EVENTHUBCLIENT_STATE eventhub_state, void* userContextCallback)
{
    printf("eventhub_state_change_callback %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_STATE, eventhub_state) );
}

void eventhub_error_callback(EVENTHUBCLIENT_ERROR_RESULT eventhub_failure, void* userContextCallback)
{
    printf("eventhub_error_callback %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_ERROR_RESULT, eventhub_failure) );
}

int Send_Sample(void)
{
    xlogging_set_log_function(custom_logging_function);

    int result;
    // Increment the id
    g_id++;

    char msgContent[BUFFER_SIZE];
    size_t msgLength = sprintf_s(msgContent, BUFFER_SIZE, "{\"messageId\":%d, \"name\":\"Send_Sample\"}", g_id);

    (void)printf("Starting the EventHub Client Send Sample (%s)...\r\n", EventHubClient_GetVersionString());

    if (platform_init() != 0)
    {
        (void)printf("ERROR: Failed initializing platform!\r\n");
        result = 1;
    }
    else
    {
        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(connectionString, eventHubPath);
        if (eventHubClientHandle == NULL)
        {
            (void)printf("ERROR: EventHubClient_CreateFromConnectionString returned NULL!\r\n");
            result = 1;
        }
        else
        {
            EventHubClient_SetStateChangeCallback(eventHubClientHandle, eventhub_state_change_callback, NULL);
            EventHubClient_SetErrorCallback(eventHubClientHandle, eventhub_error_callback, NULL);
            EventHubClient_SetLogTrace(eventHubClientHandle, true);

            EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory((const unsigned char*)msgContent, msgLength);
            if (eventDataHandle == NULL)
            {
                (void)printf("ERROR: eventDataHandle is NULL!\r\n");
                result = 1;
            }
            else
            {
                if (EventData_SetPartitionKey(eventDataHandle, PARTITION_KEY_INFO) != EVENTDATA_OK)
                {
                    (void)printf("ERROR: EventData_SetPartitionKey failed!\r\n");
                    result = 1;
                }
                else
                {
                    // Add the properties to the Event Data
                    MAP_HANDLE mapProperties = EventData_Properties(eventDataHandle);
                    if (mapProperties != NULL)
                    {
                        if (Map_Add(mapProperties, "SendHL_1", TEST_STRING_VALUE_1) != MAP_OK)
                        {
                            (void)printf("ERROR: Map_AddOrUpdate failed!\r\n");
                        }
                    }

                    if (EventHubClient_Send(eventHubClientHandle, eventDataHandle) != EVENTHUBCLIENT_OK)
                    {
                        (void)printf("ERROR: EventHubClient_Send failed!\r\n");
                        result = 1;
                    }
                    else
                    {
                        (void)printf("EventHubClient_Send.......Successful\r\n");
                        result = 0;
                    }
                }
                EventData_Destroy(eventDataHandle);
            }
            EventHubClient_Destroy(eventHubClientHandle);
        }

        platform_deinit();
    }

    (void)printf("Press any key to continue.");
    (void)getchar();
    return result; 
}
