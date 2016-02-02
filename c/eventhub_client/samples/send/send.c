// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "eventhubclient.h"
#include "eventdata.h"
#include "send.h"
#include "threadapi.h"
#include "crt_abstractions.h"
#include "platform.h"

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

int Send_Sample(void)
{
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
        EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory((const unsigned char*)msgContent, msgLength);
        if (eventDataHandle == NULL)
        {
            (void)printf("ERROR: eventDataHandle is NULL!\r\n");
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
                EventHubClient_Destroy(eventHubClientHandle);
            }
            EventData_Destroy(eventDataHandle);
        }

        platform_deinit();
    }

    return result; 
}
