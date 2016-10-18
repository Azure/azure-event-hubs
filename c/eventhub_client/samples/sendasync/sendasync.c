// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <signal.h>

#include "eventhubclient.h"
#include "eventdata.h"
#include "sendasync.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/crt_abstractions.h"

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

DEFINE_ENUM_STRINGS(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_RESULT_VALUES);

static void EventhubClientCallback(EVENTHUBCLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    sig_atomic_t* callbackNotified = (sig_atomic_t*)userContextCallback;
    *callbackNotified = 1;
    (void)printf("EventhubClientCallback result (%s)...\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_CONFIRMATION_RESULT, result) ); 
}

int SendAsync_Sample(void)
{
    int result;
    sig_atomic_t callbackNotified = 0;
    // Increment the id
    g_id++;

    if (platform_init() != 0)
    {
        (void)printf("ERROR: Failed initializing platform!\r\n");
        result = 1;
    }
    else
    {
        char msgContent[BUFFER_SIZE];
        size_t msgLength = sprintf_s(msgContent, BUFFER_SIZE, "{\"messageId\":%d, \"name\":\"SendAsync_Sample\"}", g_id);

        (void)printf("Starting the EventHub Client SendAsync Sample (%s)...\r\n", EventHubClient_GetVersionString());

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
                (void)printf("ERROR: eventHubClientHandle is NULL!\r\n");
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
                    MAP_HANDLE mapProperties = EventData_Properties(eventDataHandle);
                    if (mapProperties != NULL)
                    {
                        if (Map_AddOrUpdate(mapProperties, "SendAsyncHL_1", TEST_STRING_VALUE_1) != MAP_OK)
                        {
                            (void)printf("ERROR: Map_AddOrUpdate failed!\r\n");
                        }
                    }

                    if (EventHubClient_SendAsync(eventHubClientHandle, eventDataHandle, EventhubClientCallback, &callbackNotified) != EVENTHUBCLIENT_OK)
                    {
                        (void)printf("ERROR: EventHubClient_SendAsync failed!\r\n");
                        result = 1;
                    }
                    else
                    {
                        while (callbackNotified == 0)
                        {
                            ThreadAPI_Sleep(SLEEP_TIME);
                        }

                        (void)printf("EventHubClient_SendAsync.......Successful\r\n");
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
