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

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "eventhubclient.h"
#include "eventhubclient_ll.h"

#include "eventhub_account.h"
#include "eventhub_testclient.h"

#include "buffer_.h"
#include "threadapi.h"
#include "crt_abstractions.h"

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

const char* TEST_NOTIFICATION_DATA_FMT = "{\"notifyData\":\"%.24s\",\"id\":%d,\"index\":%d}";

static size_t g_eventHubTestId = 0;

#define MAX_CLOUD_TRAVEL_TIME       60.0

/*the following time expressed in seconds denotes the maximum time to read all the events available in an event hub*/ 
#define MAX_EXECUTE_TIME            60.0 
#define DATA_MAX_SIZE               256
#define MAX_NUM_OF_MESSAGES         3

DEFINE_MICROMOCK_ENUM_TO_STRING(EVENTHUB_TEST_CLIENT_RESULT, EVENTHUB_TEST_CLIENT_RESULT_VALUES);
DEFINE_MICROMOCK_ENUM_TO_STRING(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_RESULT_VALUES);

typedef struct EXPECTED_RECEIVE_DATA_TAG
{
    const char* data[MAX_NUM_OF_MESSAGES];
    size_t dataSize[MAX_NUM_OF_MESSAGES];
    size_t MessagesFound;
    bool dataWasSent;
    size_t numOfMsg;
    EVENTHUBCLIENT_CONFIRMATION_RESULT eventhubConfirmResult;
} EXPECTED_RECEIVE_DATA;

static int EventhubTestClientCallback(void* context, const char* data, size_t size)
{
    int result = 0; /* 0 means keep processing data */
    EXPECTED_RECEIVE_DATA* expectedData = (EXPECTED_RECEIVE_DATA*)context;

    for (size_t index = 0; index < expectedData->numOfMsg; index++)
    {
        if (expectedData->dataSize[index] == size)
        {
            if (memcmp(data, expectedData->data[index], size) == 0)
            {
                expectedData->MessagesFound++;
                if (expectedData->MessagesFound >= expectedData->numOfMsg)
                {
                    result = 1;
                }
                break;
            }
        }
    }
    return result;
}

static void EventhubClientCallback(EVENTHUBCLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    EXPECTED_RECEIVE_DATA* expectedData = (EXPECTED_RECEIVE_DATA*)userContextCallback;
    expectedData->eventhubConfirmResult = result;
    expectedData->dataWasSent = true;
}

BEGIN_TEST_SUITE(eventhubclient_e2etests)

    static EXPECTED_RECEIVE_DATA* MessageData_Create(size_t numOfMessages)
    {
        EXPECTED_RECEIVE_DATA* result = (EXPECTED_RECEIVE_DATA*)malloc(sizeof(EXPECTED_RECEIVE_DATA));
        if (result != NULL)
        {
            for (size_t index = 0; index < numOfMessages; index++)
            {
                char temp[DATA_MAX_SIZE];
                char* tempString;
                time_t t = time(NULL);
                int charsWritten = sprintf_s(temp, DATA_MAX_SIZE, TEST_NOTIFICATION_DATA_FMT, ctime(&t), g_eventHubTestId, index);
                if ( (tempString = (char*)malloc(charsWritten + 1) ) == NULL)
                {
                    for (size_t removeIndex = 0; removeIndex < index-1; removeIndex++)
                    {
                        free( (void*)result->data[removeIndex]);
                    }
                    free(result);
                    result = NULL;
                    break;
                }
                else
                {
                    strcpy(tempString, temp);
                    result->data[index] = tempString;
                    result->dataSize[index] = strlen(result->data[index]);
                }
            }

            if (result != NULL)
            {
                result->numOfMsg = numOfMessages;
                result->MessagesFound = 0;
                result->dataWasSent = false;
                result->eventhubConfirmResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;
            }
        }
        return result;
    }

    static void NotificationData_Destroy(EXPECTED_RECEIVE_DATA* data)
    {
        if (data != NULL)
        {
            if (data->data != NULL)
            {
                for (size_t index = 0; index < data->numOfMsg; index++)
                {
                    free( (void*)data->data[index]);
                }
            }
        }
        free(data);
    }

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        g_eventHubTestId++;
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
    }

    TEST_FUNCTION(EventHub_SendTelemetry_E2ETests)
    {
        // arrange
        size_t messageToCreate = 1;

        EXPECTED_RECEIVE_DATA* messageData = MessageData_Create(messageToCreate);
        ASSERT_IS_NOT_NULL(messageData);

        EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory( (const unsigned char*)messageData->data[0], messageData->dataSize[0]);
        ASSERT_IS_NOT_NULL(eventDataHandle);

        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(EventHubAccount_GetConnectionString(), EventHubAccount_GetName() );
        ASSERT_IS_NOT_NULL(eventHubClientHandle);

        EVENTHUBCLIENT_RESULT result = EventHubClient_Send(eventHubClientHandle, eventDataHandle);
        ASSERT_ARE_EQUAL(int, EVENTHUBCLIENT_OK, result);

        // Destroy the data
        EventHubClient_Destroy(eventHubClientHandle);
        EventData_Destroy(eventDataHandle);

        EVENTHUB_TEST_CLIENT_HANDLE eventhubTestHandle = EventHub_Initialize(EventHubAccount_GetConnectionString(), EventHubAccount_GetName() );
        ASSERT_IS_NOT_NULL(eventhubTestHandle);

        EVENTHUB_TEST_CLIENT_RESULT eventTestResult = EventHub_ListenForMsg(eventhubTestHandle, EventhubTestClientCallback, EventHubAccount_PartitionCount(), MAX_EXECUTE_TIME, messageData);
        ASSERT_ARE_EQUAL(EVENTHUB_TEST_CLIENT_RESULT, EVENTHUB_TEST_CLIENT_OK, eventTestResult);

        // assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, messageToCreate, messageData->MessagesFound, "Message should get written in the EventHub Client Callback."); /* was MessagesFound is written by the callback... */

        // cleanup
        EventHub_Deinit(eventhubTestHandle);
        NotificationData_Destroy(messageData);
    }

    TEST_FUNCTION(EventHub_SendTelemetryAsync_E2ETests)
    {
        // arrange
        size_t messageToCreate = 1;

        EXPECTED_RECEIVE_DATA* messageData = MessageData_Create(messageToCreate);
        ASSERT_IS_NOT_NULL(messageData);

        EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory( (const unsigned char*)messageData->data[0], messageData->dataSize[0]);
        ASSERT_IS_NOT_NULL(eventDataHandle);

        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(EventHubAccount_GetConnectionString(), EventHubAccount_GetName() );
        ASSERT_IS_NOT_NULL(eventHubClientHandle);

        EVENTHUBCLIENT_RESULT result = EventHubClient_SendAsync(eventHubClientHandle, eventDataHandle, EventhubClientCallback, messageData);
        ASSERT_ARE_EQUAL(int, EVENTHUBCLIENT_OK, result);

        time_t beginOp = time(NULL);
        while ( 
            (!messageData->dataWasSent) &&
            (difftime(time(NULL), beginOp) < MAX_CLOUD_TRAVEL_TIME)
            )
        {
            // give it some time
        }

        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_OK, messageData->eventhubConfirmResult);

        // Destroy the data
        EventHubClient_Destroy(eventHubClientHandle);
        EventData_Destroy(eventDataHandle);

        EVENTHUB_TEST_CLIENT_HANDLE eventhubTestHandle = EventHub_Initialize(EventHubAccount_GetConnectionString(), EventHubAccount_GetName() );
        ASSERT_IS_NOT_NULL(eventhubTestHandle);

        EVENTHUB_TEST_CLIENT_RESULT eventTestResult = EventHub_ListenForMsg(eventhubTestHandle, EventhubTestClientCallback, EventHubAccount_PartitionCount(), MAX_EXECUTE_TIME, messageData);
        ASSERT_ARE_EQUAL(EVENTHUB_TEST_CLIENT_RESULT, EVENTHUB_TEST_CLIENT_OK, eventTestResult);

        // assert
        ASSERT_ARE_EQUAL(int, messageToCreate, messageData->MessagesFound); /* was found is written by the callback... */

        // cleanup
        EventHub_Deinit(eventhubTestHandle);
        NotificationData_Destroy(messageData);
    }

    TEST_FUNCTION(EventHub_SendTelemetryBatch_E2ETest)
    {
        // arrange
        size_t messageToCreate = MAX_NUM_OF_MESSAGES;

        EXPECTED_RECEIVE_DATA* messageData = MessageData_Create(messageToCreate);
        ASSERT_IS_NOT_NULL(messageData);

        EVENTDATA_HANDLE eventDataList[MAX_NUM_OF_MESSAGES];
        for (size_t index = 0; index < messageToCreate; index++)
        {
            eventDataList[index] = EventData_CreateWithNewMemory((const unsigned char*)messageData->data[index], messageData->dataSize[index]);
            ASSERT_IS_NOT_NULL(eventDataList[index]);
        }

        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(EventHubAccount_GetConnectionString(), EventHubAccount_GetName() );
        ASSERT_IS_NOT_NULL(eventHubClientHandle);

        EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatch(eventHubClientHandle, eventDataList, messageToCreate);
        ASSERT_ARE_EQUAL(int, EVENTHUBCLIENT_OK, result);

        // Destroy the data
        EventHubClient_Destroy(eventHubClientHandle);
        for (size_t index = 0; index < messageToCreate; index++)
        {
            EventData_Destroy(eventDataList[index]);
        }

        EVENTHUB_TEST_CLIENT_HANDLE eventhubTestHandle = EventHub_Initialize(EventHubAccount_GetConnectionString(), EventHubAccount_GetName() );
        ASSERT_IS_NOT_NULL(eventhubTestHandle);

        EVENTHUB_TEST_CLIENT_RESULT eventTestResult = EventHub_ListenForMsg(eventhubTestHandle, EventhubTestClientCallback, EventHubAccount_PartitionCount(), MAX_EXECUTE_TIME, messageData);
        ASSERT_ARE_EQUAL(EVENTHUB_TEST_CLIENT_RESULT, EVENTHUB_TEST_CLIENT_OK, eventTestResult);

        // assert
        ASSERT_ARE_EQUAL(int, messageToCreate, messageData->MessagesFound); /* was found is written by the callback... */

        // cleanup
        EventHub_Deinit(eventhubTestHandle);
        NotificationData_Destroy(messageData);
    }

    TEST_FUNCTION(EventHub_SendTelemetryBatchAsync_E2ETest)
    {
        // arrange
        size_t messageToCreate = MAX_NUM_OF_MESSAGES;

        EXPECTED_RECEIVE_DATA* messageData = MessageData_Create(messageToCreate);
        ASSERT_IS_NOT_NULL(messageData);

        EVENTDATA_HANDLE eventDataList[MAX_NUM_OF_MESSAGES];
        for (size_t index = 0; index < messageToCreate; index++)
        {
            eventDataList[index] = EventData_CreateWithNewMemory((const unsigned char*)messageData->data[index], messageData->dataSize[index]);
            ASSERT_IS_NOT_NULL(eventDataList[index]);
        }

        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(EventHubAccount_GetConnectionString(), EventHubAccount_GetName() );
        ASSERT_IS_NOT_NULL(eventHubClientHandle);

        EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatchAsync(eventHubClientHandle, eventDataList, messageToCreate, EventhubClientCallback, messageData);
        ASSERT_ARE_EQUAL(int, EVENTHUBCLIENT_OK, result);

        time_t beginOp = time(NULL);
        while ( 
            (!messageData->dataWasSent) &&
            (difftime(time(NULL), beginOp) < MAX_CLOUD_TRAVEL_TIME)
            )
        {
            // give it some time
        }

        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_OK, messageData->eventhubConfirmResult);

        // Destroy the data
        EventHubClient_Destroy(eventHubClientHandle);
        for (size_t index = 0; index < messageToCreate; index++)
        {
            EventData_Destroy(eventDataList[index]);
        }

        EVENTHUB_TEST_CLIENT_HANDLE eventhubTestHandle = EventHub_Initialize(EventHubAccount_GetConnectionString(), EventHubAccount_GetName() );
        ASSERT_IS_NOT_NULL(eventhubTestHandle);

        EVENTHUB_TEST_CLIENT_RESULT eventTestResult = EventHub_ListenForMsg(eventhubTestHandle, EventhubTestClientCallback, EventHubAccount_PartitionCount(), MAX_EXECUTE_TIME, messageData);
        ASSERT_ARE_EQUAL(EVENTHUB_TEST_CLIENT_RESULT, EVENTHUB_TEST_CLIENT_OK, eventTestResult);

        // assert
        ASSERT_ARE_EQUAL(int, messageToCreate, messageData->MessagesFound); /* was found is written by the callback... */

        // cleanup
        EventHub_Deinit(eventhubTestHandle);
        NotificationData_Destroy(messageData);
    }

END_TEST_SUITE(eventhubclient_e2etests)
