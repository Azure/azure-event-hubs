/*
Microsoft Azure IoT Device Libraries

Copyright (c) Microsoft Corporation
All rights reserved.

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the Software), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "eventhubclient.h"
#include "eventdata.h"
#include "send.h"
#include "crt_abstractions.h"
#include "threadapi.h"

#include <stdio.h>
#include <time.h>
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Serialization library information can be found http://ccodearchive.net/info/json.html

static const char* connectionString = "Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]";
static const char* eventHubPath = "[event hub name]";

#define SLEEP_TIME		3000

static char *serialize(int device_id, float reading)
{
    char* result;

    // Create a serialization creation object (JSON)
    JsonNode *object = json_mkobject();
    if (object == NULL)
    {
        result = NULL;
    }
    else
    {
        // Temporary string for the float value (conversion to string)
        char tstring[64];
        (void)sprintf_s(tstring, 64, "%.2f", reading);

        json_append_member(object, "device_id", json_mknumber(device_id));
        json_append_member(object, "reading", json_mkstring(tstring));

        // Create and return the JSON object.
        result = json_encode(object);
        free(object);
    }
    return result;
}

static int sendData(const char *msgContent)
{
    // Send result.  0 indicates success.
    int result;

    // Create a new handle with the message to be sent.
    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory( (const unsigned char*)msgContent, strlen(msgContent));

    // Make sure the handle was created successfully.
    if (eventDataHandle == NULL)
    {
        (void)printf("ERROR: eventDataHandle is NULL!\r\n");
        result = 1;
    }
    else
    {
        // Create a client.
        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(connectionString, eventHubPath);

        // Make sure the client was created successfully.
        if (eventHubClientHandle == NULL)
        {
            (void)printf("ERROR: eventHubClientHandle is NULL!\r\n");
            result = 1;
        }
        else
        {
            // Send the message.  There is also a batch send function available, 
            // EventHubClient_SendBatch(eventHubClientHandle, <array>, <array_size>);
            if (EventHubClient_Send(eventHubClientHandle, eventDataHandle) != EVENTHUBCLIENT_OK)
            {
                (void)printf("ERROR: EventHubClient_Send failed!\r\n");
                result = 1;
            }
            else
            {
                result = 0;
            }
            // Clean up the client.
            EventHubClient_Destroy(eventHubClientHandle);
        }
        // Clean up the handle.
        EventData_Destroy(eventDataHandle);
    }
    return result;
}

int sendsample_run(void)
{
    // Generated reading
    float reading = 0.0;

    int randNo = 0; 
    int retVal = 0;

    // Initialize the random number generator
    srand((int)time(NULL));

    for (size_t index = 0; index < 12; ++index)
    {
        // Create a new (random) reading.  rand() returns an int, so rand()/RANDMAX normalizes to 0-1.
        randNo = rand();
        reading = 100.0f + 30.0f * ((float)(randNo) / (float)(RAND_MAX));

        // Serialize the result.
        char* msgContent = serialize(500, reading);

        // Check for failed serialization.
        if (msgContent == NULL)
        {
            (void)printf("ERROR: Serialization failed!\r\n");
            retVal = 1;
            break;
        }
        else
        {
            // Send the data.  Set retVal to 1 if any send fails.
            if (sendData(msgContent) > 0)
            {
                retVal = 1;
            }
            free(msgContent);
        }

        ThreadAPI_Sleep(SLEEP_TIME);
    }
    return retVal;
}
