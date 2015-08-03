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

#include <stdio.h>

#include "eventhubclient.h"
#include "eventdata.h"
#include "send.h"
#include "threadapi.h"
#include "crt_abstractions.h"

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

    (void)printf("Starting the EventHub Client Send Sample (%s)...\r\n", EventHubClient_GetVersionString() ); 

    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory( (const unsigned char*)msgContent, msgLength);
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
    return result; 
}
