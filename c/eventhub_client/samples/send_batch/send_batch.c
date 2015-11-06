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
#include <time.h>

#include "eventhubclient.h"
#include "eventdata.h"
#include "send_batch.h"
#include "threadapi.h"
#include "crt_abstractions.h"

static const char* connectionString = "Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]";
static const char* eventHubPath = "[event hub name]";

static bool g_bSendProperties = false;
static bool g_bSendPartitionKey = false;

static const char PARTITION_KEY_INFO[] = "PartitionKeyInfo";
static const char TEST_STRING_VALUE_1[] = "Property_String_Value_1";
static const char TEST_STRING_VALUE_A[] = "Property_String_Value_A";
static const char TEST_STRING_VALUE_2[] = "Property_String_Value_2";
static const char TEST_STRING_VALUE_B[] = "Property_String_Value_B";

#define NUM_OF_MSG_TO_SEND 2
#define SLEEP_TIME		1000
#define BUFFER_SIZE     128

int SendBatch_Sample(void)
{
    int result;
    time_t t = time(&t);
    (void)printf("Starting the EventHub Client SendBatch Sample (%s)...\r\n", EventHubClient_GetVersionString() ); 

    //Create an Array of EventData so we can send these data batched.
    EVENTDATA_HANDLE eventDataList[NUM_OF_MSG_TO_SEND];

    size_t index;
    for (index = 0; index < NUM_OF_MSG_TO_SEND; index++)
    {
        char msgContent[BUFFER_SIZE];
        size_t msgLength = sprintf_s(msgContent, BUFFER_SIZE, "{\"messageId\":%ul, \"timeValue\":%I64d, \"name\":\"SendBatch_Sample\"}", index, t);

        if ( (eventDataList[index] = EventData_CreateWithNewMemory((const unsigned char*)msgContent, msgLength) ) == NULL)
        {
            (void)printf("ERROR: EventData_CreateWithNewMemory returned NULL!\r\n");
            result = 1;
            break;
        }
    }

    if (index == NUM_OF_MSG_TO_SEND)
    {
        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(connectionString, eventHubPath);
        if (eventHubClientHandle == NULL)
        {
            (void)printf("ERROR: EventHubClient_CreateFromConnectionString returned NULL!\r\n");
            result = 1;
        }
        else
        {
            for (size_t i = 0; i < NUM_OF_MSG_TO_SEND; i++)
            {
                EventData_SetPartitionKey(eventDataList[i], PARTITION_KEY_INFO);
            }


            MAP_HANDLE mapProperties = EventData_Properties(eventDataList[0]);
            if (mapProperties != NULL)
            {
                if ( (Map_AddOrUpdate(eventDataList[0], "SendBatch_HL_1", TEST_STRING_VALUE_1) != MAP_OK) ||
                    (Map_AddOrUpdate(eventDataList[0], "SendBatch_HL_A", TEST_STRING_VALUE_A) != MAP_OK) ||
                    (Map_AddOrUpdate(eventDataList[1], "SendBatch_HL_2", TEST_STRING_VALUE_2) != MAP_OK)
                    )
                {
                    (void)printf("ERROR: Map_AddOrUpdate failed!\r\n");
                }
            }

            //Send all 3 data created above on the same call (Batched)
            if (EventHubClient_SendBatch(eventHubClientHandle, eventDataList, NUM_OF_MSG_TO_SEND) != EVENTHUBCLIENT_OK)
            {
                (void)printf("ERROR: EventHubClient_SendBatch failed!\r\n");
                result = 1;
            }
            else
            {
                (void)printf("EventHubClient_SendBatch.......Successful\r\n");
                result = 0;
            }
            EventHubClient_Destroy(eventHubClientHandle);
        }
    }
    for (size_t i = 0; i < index; i++)
    {
        EventData_Destroy(eventDataList[i]);
    }
    return result;
}
