// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include "eventhubreceiver.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"

static const char* connectionString = "Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]";
static const char* eventHubPath     = "[event hub name]";
static const char* consumerGroup    = "[consumer group name]"; //example "$Default"
static const char* partitionId      = "[partition id]";        //example "0"

#define SLEEP_TIME_MS   2000        // in milliseconds

static volatile int timeoutCounter   = 0;
static volatile int errorCBTriggered = 0;

static void PrintData(const unsigned char* buffer, size_t size)
{
    size_t index = 0;
    if (buffer && size)
    {
        (void)printf("Data Received:[");
        while (index < size)
        {
            (void)printf("%c", buffer[index++]);
        }
        (void)printf("]\r\n");
    }
}

static void PrintProperties(MAP_HANDLE map)
{
    STRING_HANDLE jsonKVP = Map_ToJSON(map);
    if (jsonKVP != NULL)
    {
        (void)printf("   >>>Key Value Pairs Received:[%s]\r\n", STRING_c_str(jsonKVP));
        STRING_delete(jsonKVP);
    }
    else
    {
        (void)printf("   >>>Key Value Pairs Received:[]\r\n");
    }
}

static void PrintEventDataReceiveParams(EVENTDATA_HANDLE eventDataHandle)
{
    uint64_t timestamp = EventData_GetEnqueuedTimestampUTCInMs(eventDataHandle);
    (void)printf("   >>>Event Data Enqueue Timestamp Ms Raw:[%" PRIu64 "]\r\n", timestamp);
}

static void OnReceiveCB(EVENTHUBRECEIVER_RESULT result, EVENTDATA_HANDLE eventDataHandle, void* userContext)
{
    (void)userContext;

    switch (result)
    {
        case EVENTHUBRECEIVER_TIMEOUT:
        {
            timeoutCounter++;
            (void)printf("INFO: Timeout Seen# %d\r\n", timeoutCounter);
            break;
        }
        case EVENTHUBRECEIVER_OK:
        {
            EVENTDATA_RESULT eventDataResult;
            MAP_HANDLE map;
            size_t dataSize;
            const unsigned char *dataBuffer;

            if ((eventDataResult = EventData_GetData(eventDataHandle, &dataBuffer, &dataSize)) == EVENTDATA_OK)
            {
                PrintData(dataBuffer, dataSize);
            }
            if ((map = EventData_Properties(eventDataHandle)) != NULL)
            {
                PrintProperties(map);
            }
            PrintEventDataReceiveParams(eventDataHandle);
            break;
        }
        default:
            (void)printf("ERROR: Result code %u.\r\n", result);
    };
}

static void OnErrorCB(EVENTHUBRECEIVER_RESULT errorCode, void* userContext)
{
    (void)userContext;
    (void)printf("ERROR: Unexpected Error Callback Triggered! %u\r\n", errorCode);
    errorCBTriggered = 1;
}

int Receive_Sample(void)
{
    int result;
    time_t now;

    if (platform_init() != 0)
    {
        (void)printf("ERROR: Failed initializing platform!\r\n");
        result = 1;
    }
    else if ((now = time(NULL)) == (time_t)(-1))
    {
        (void)printf("ERROR: In obtaining current UTC time in seconds!\r\n");
        result = 1;
        platform_deinit();
    }
    else
    {
        EVENTHUBRECEIVER_RESULT errorCode;
        EVENTHUBRECEIVER_HANDLE eventHubReceiveHandle = EventHubReceiver_Create(connectionString, eventHubPath, consumerGroup, partitionId);
        if (eventHubReceiveHandle == NULL)
        {
            (void)printf("ERROR: EventHubClient_CreateFromConnectionString returned NULL!\r\n");
            result = 1;
        }
        else
        {
            // set back an hour ago
            uint64_t nowTimestamp = (now > 3600) ? (now - 3600) : 0;
            // enable connection tracing
            EventHubReceiver_SetConnectionTracing(eventHubReceiveHandle, true);
            // register callback functions to read event payload from the hub
            errorCode = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(eventHubReceiveHandle, OnReceiveCB, NULL, OnErrorCB, NULL, nowTimestamp, 1000);
            if (errorCode != EVENTHUBRECEIVER_OK)
            {
                (void)printf("ERROR: EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync returned NULL!\r\n");
                result = 1;
            }
            else
            {
                int exitLoop = 0;
                while (!exitLoop)
                {
                    if ((timeoutCounter >= 10) || (errorCBTriggered))
                    {
                        exitLoop = 1;
                    }
                    else
                    {
                        ThreadAPI_Sleep(SLEEP_TIME_MS);
                    }
                }
                result = 0;
            }
            EventHubReceiver_Destroy(eventHubReceiveHandle);
        }
        platform_deinit();
    }

    return result; 
}

int main(void)
{
    return Receive_Sample();
}
