// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <limits.h>
#include <inttypes.h>
#include <time.h>

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"

#include "eventhubclient.h"
#include "eventhubclient_ll.h"
#include "eventhubreceiver.h"
#include "eventhubreceiver_ll.h"
#include "eventhub_account.h"

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

const char* TEST_NOTIFICATION_DATA_FMT = "{\"id\":%d,\"index\":%d,\"notifyData\":\"%24s\"}";
const char* TEST_NOTIFICATION_DATA_PARSE_FMT = "{\"id\":%d,\"index\":%d";

static size_t g_eventHubTestId = 0;

#define MAX_CLOUD_TRAVEL_TIME           60.0

/* the following time expressed in seconds denotes the maximum time to read all the events available in an event hub */
#define MAX_EXECUTE_TIME_MS             60 * 1000
#define DATA_MAX_SIZE                   256
#define MAX_NUM_OF_MESSAGES             3
#define MAX_UNSIGNED_SHORT_BASE_10_LEN  6 //65536\0

#define RECEIVER_SLEEP_TIME_MS          1000

DEFINE_MICROMOCK_ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_RESULT_VALUES);
DEFINE_MICROMOCK_ENUM_TO_STRING(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_RESULT_VALUES);

typedef struct EXPECTED_RECEIVE_DATA_TAG
{
    const char* data[MAX_NUM_OF_MESSAGES];
    size_t dataSize[MAX_NUM_OF_MESSAGES];
    bool   dataWasSent;
    bool   partitionKeyWasSet;
    size_t numOfMsg;
    size_t testId;
    uint64_t enqueuedTimestamp;
    EVENTHUBCLIENT_CONFIRMATION_RESULT eventhubConfirmResult;
} EXPECTED_RECEIVE_DATA;

typedef struct EVENTHUB_RECEIVED_DATA_TAG
{
    DLIST_ENTRY             entry;
    EVENTDATA_HANDLE        dataHandle;
} EVENTHUB_RECEIVED_DATA;

typedef struct EVENTHUB_RECEIVER_TEST_DATA_TAG
{
    EVENTHUBRECEIVER_HANDLE receiver;
    EVENTHUBRECEIVER_RESULT errorCode;
    int exitConditionObserved;
    DLIST_ENTRY receiveCallbackDataList;
    LOCK_HANDLE lock;
} EVENTHUB_RECEIVER_TEST_DATA;

typedef struct EVENTHUB_RECEIVERS_DATA_TAG
{
    EVENTHUB_RECEIVER_TEST_DATA* receivers;
    size_t numReceivers;
    LOCK_HANDLE receiversLock;
} EVENTHUB_RECEIVERS_DATA;

static void EventhubClientCallback(EVENTHUBCLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    EXPECTED_RECEIVE_DATA* expectedData = (EXPECTED_RECEIVE_DATA*)userContextCallback;
    expectedData->eventhubConfirmResult = result;
    expectedData->dataWasSent = true;
}

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

static void PrintEventDataReceiveParams(EVENTDATA_HANDLE eventDataHandle)
{
    uint64_t timestamp = EventData_GetEnqueuedTimestampUTCInMs(eventDataHandle);
    (void)printf("   >>>Event Data Enqueue Timestamp In Ms Raw:[%" PRIu64 "]\r\n", timestamp);
}

static void OnReceiveCB(EVENTHUBRECEIVER_RESULT result, EVENTDATA_HANDLE eventDataHandle, void* userContext)
{
    EVENTHUB_RECEIVER_TEST_DATA* rxData = (EVENTHUB_RECEIVER_TEST_DATA*)userContext;
    LOCK_RESULT lockResult;
    bool endReceiver = false;

    lockResult = Lock(rxData->lock);
    ASSERT_ARE_EQUAL_WITH_MSG(int, LOCK_OK, lockResult, "OnReceiveCB Lock Failure");
    switch (result)
    {
        case EVENTHUBRECEIVER_OK:
        {
            size_t dataSize;
            const unsigned char *dataBuffer;
            EVENTDATA_RESULT eventDataResult;
            EVENTHUB_RECEIVED_DATA* receivedData;
            receivedData = (EVENTHUB_RECEIVED_DATA*)malloc(sizeof(EVENTHUB_RECEIVED_DATA));
            ASSERT_IS_NOT_NULL(receivedData);
            receivedData->dataHandle = EventData_Clone(eventDataHandle);
            if ((eventDataResult = EventData_GetData(eventDataHandle, &dataBuffer, &dataSize)) == EVENTDATA_OK)
            {
                PrintData(dataBuffer, dataSize);
            }
            PrintEventDataReceiveParams(eventDataHandle);
            ASSERT_IS_NOT_NULL(receivedData->dataHandle);
            DList_InsertTailList(&rxData->receiveCallbackDataList, &(receivedData->entry));
            break;
        }
        default:
            rxData->exitConditionObserved = 1;
            rxData->errorCode = result;
            endReceiver = true;
    };
    lockResult = Unlock(rxData->lock);
    ASSERT_ARE_EQUAL_WITH_MSG(int, LOCK_OK, lockResult, "OnReceiveCB Unlock Failure");
    if (endReceiver)
    {
        EVENTHUBRECEIVER_RESULT rxResult = EventHubReceiver_ReceiveEndAsync(rxData->receiver, NULL, NULL);
        ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, rxResult, "OnReceiveCB EventHubReceiver_ReceiveEndAsync Failure");
    }
}

static void OnReceiveErrorCB(EVENTHUBRECEIVER_RESULT errorCode, void* userContext)
{
    EVENTHUB_RECEIVER_TEST_DATA_TAG* rxData = (EVENTHUB_RECEIVER_TEST_DATA_TAG*)userContext;
    LOCK_RESULT lockResult;
    EVENTHUBRECEIVER_RESULT rxResult;

    lockResult = Lock(rxData->lock);
    ASSERT_ARE_EQUAL_WITH_MSG(int, LOCK_OK, lockResult, "OnReceiveErrorCB Lock Failure");
    rxData->exitConditionObserved = 1;
    rxData->errorCode = errorCode;
    lockResult = Unlock(rxData->lock);
    ASSERT_ARE_EQUAL_WITH_MSG(int, LOCK_OK, lockResult, "OnReceiveErrorCB Unlock Failure");
    rxResult = EventHubReceiver_ReceiveEndAsync(rxData->receiver, NULL, NULL);
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, rxResult, "OnReceiveErrorCB EventHubReceiver_ReceiveEndAsync Failure");
}

static void AsyncDataCallbackListInit(PDLIST_ENTRY list)
{
    DList_InitializeListHead(list);
}

static void AsyncDataCallbackListDeInit(PDLIST_ENTRY list)
{
    PDLIST_ENTRY tempEntry;
    while ((tempEntry = DList_RemoveHeadList(list)) != list)
    {
        EVENTHUB_RECEIVED_DATA* callbackData = containingRecord(tempEntry, EVENTHUB_RECEIVED_DATA, entry);
        if (callbackData->dataHandle)
        {
            EventData_Destroy(callbackData->dataHandle);
        }
        free(callbackData);
    }
}

BEGIN_TEST_SUITE(eventhubclient_e2etests)
    
    static EXPECTED_RECEIVE_DATA* MessageData_Create(size_t numOfMessages, uint64_t enqueuedTimestamp)
    {
        EXPECTED_RECEIVE_DATA* result = (EXPECTED_RECEIVE_DATA*)malloc(sizeof(EXPECTED_RECEIVE_DATA));
        if (result != NULL)
        {
            for (size_t index = 0; index < numOfMessages; index++)
            {
                char temp[DATA_MAX_SIZE];
                char* tempString;
                time_t t = time(NULL);
                int charsWritten = sprintf_s(temp, DATA_MAX_SIZE, TEST_NOTIFICATION_DATA_FMT, g_eventHubTestId, index, ctime(&t));
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
                result->enqueuedTimestamp = enqueuedTimestamp;
                result->testId = g_eventHubTestId;
                result->numOfMsg = numOfMessages;
                result->dataWasSent = false;
                result->partitionKeyWasSet = false;
                result->eventhubConfirmResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;
            }
        }
        return result;
    }

    static void MessageData_Destroy(EXPECTED_RECEIVE_DATA* data)
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
            free(data);
        }
    }

    static int ValidateReceivedData(EVENTDATA_HANDLE eventDataHandle, EXPECTED_RECEIVE_DATA* expectedData)
    {
        int result;
        size_t dataSize;
        const unsigned char *dataBuffer;
        EVENTDATA_RESULT eventDataResult;
        uint64_t enqueuedtimestamp;

        if ((eventDataResult = EventData_GetData(eventDataHandle, &dataBuffer, &dataSize)) != EVENTDATA_OK)
        {
            LogError("Error seen in EventData_GetData. Code:%u\r\n", eventDataResult);
            result = __LINE__;
        }
        else if ((enqueuedtimestamp = EventData_GetEnqueuedTimestampUTCInMs(eventDataHandle)) < expectedData->enqueuedTimestamp * (uint64_t)1000)
        {
            LogError("Unexpected Enqueued Timestamp seen EventData_GetEnqueuedTimestampUTC. Expected Timestamp Greater Than or Equal To:%" PRIu64 "  Received:%" PRIu64 "\r\n", eventDataResult);
            result = __LINE__;
        }
        else
        {
            int receivedId = -1, receivedIndex = -1;
            int status;
            BUFFER_HANDLE buffHandle;

            buffHandle = BUFFER_new();
            if (buffHandle == NULL)
            {
                LogError("Could not create buffHandle\r\n");
                result = __LINE__;
            }
            else if ((status = BUFFER_pre_build(buffHandle, dataSize + 1)) != 0)
            {
                LogError("Failed BUFFER_pre_build. Code:%d\r\n", status);
                result = __LINE__;
            }
            else
            {
                unsigned char* temp = BUFFER_u_char(buffHandle);
                memset(temp, 0, dataSize + 1);
                memcpy(temp, dataBuffer, dataSize);
                status = sscanf((const char*)temp, TEST_NOTIFICATION_DATA_PARSE_FMT, &receivedId, &receivedIndex);
                if (status != 2)
                {
                    LogError("Error seen when parsing EventData Binary Data. Result:%d\r\n", status);
                    result = __LINE__;
                }
                else
                {
                    if (receivedId != (int)expectedData->testId)
                    {
                        LogError("Mismatch Seen For Test ID. Expected:[%d] Received:%d\r\n", (int)expectedData->testId, receivedId);
                        result = __LINE__;
                    }
                    else if ((receivedIndex < 0) || (receivedIndex >(int)expectedData->numOfMsg))
                    {
                        LogError("Index Seen Outside of Expected Range. Expected:[0..%d] Received:%d\r\n", (int)expectedData->numOfMsg, receivedIndex);
                        result = __LINE__;
                    }
                    else if (expectedData->dataSize[receivedIndex] != dataSize)
                    {
                        LogError("Mismatch seen in Received Data Size. Expected:%u Received:%u\r\n", expectedData->dataSize[receivedIndex], dataSize);
                        result = __LINE__;
                    }
                    else if (memcmp(dataBuffer, expectedData->data[receivedIndex], dataSize) != 0)
                    {
                        LogError("Mismatch seen in Received Data.\r\n");
                        result = __LINE__;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                BUFFER_delete(buffHandle);
            }
        }

        return result;
    }

    static int EventHubReceiversTest_ValidateReceivedMessageData(EVENTHUB_RECEIVERS_DATA* receiversData, EXPECTED_RECEIVE_DATA* expectedData)
    {
        int result = 0; // 0 means keep processing data
        bool multiplePartitionsReceivedData = false;
        // validate if the data was really sent
        if ((expectedData->numOfMsg > 0) && ((expectedData->dataWasSent == false) || (expectedData->eventhubConfirmResult != EVENTHUBCLIENT_CONFIRMATION_OK)))
        {
            LogError("EventHubClient Send Failed\r\n");
            result = __LINE__;
        }
        else
        {
            int idx, receivedPartitionIndex = -1;
            size_t numMessagesReceived = 0;
            // scan through all the receivers and examine all received data messages
            for (idx = 0; idx < (int)receiversData->numReceivers; idx++)
            {
                PDLIST_ENTRY list = &receiversData->receivers[idx].receiveCallbackDataList;
                PDLIST_ENTRY tempEntry = list->Flink;
                bool wasDataReceived = false;
                while (tempEntry != list)
                {
                    EVENTHUB_RECEIVED_DATA* callbackData = containingRecord(tempEntry, EVENTHUB_RECEIVED_DATA, entry);
                    EVENTDATA_HANDLE eventDataHandle = callbackData->dataHandle;
                    if (eventDataHandle)
                    {
                        result = ValidateReceivedData(eventDataHandle, expectedData);
                        if (result != 0)
                        {
                            break;
                        }
                        numMessagesReceived++;
                        if (numMessagesReceived == 1)
                        {
                            receivedPartitionIndex = idx;
                        }
                        wasDataReceived = true;
                    }
                    tempEntry = tempEntry->Flink;
                }
                if ((wasDataReceived) && (receivedPartitionIndex != idx))
                {
                    multiplePartitionsReceivedData = true;
                }
            }
            if (result == 0)
            {
                if (numMessagesReceived != expectedData->numOfMsg)
                {
                    LogError("Mismatch seen in Number of Messages. Expected:%u Received:%u\r\n", expectedData->numOfMsg, numMessagesReceived);
                    result = __LINE__;
                }
                else if ((expectedData->partitionKeyWasSet) && (multiplePartitionsReceivedData))
                {
                    LogError("Received Event Data from multiple partitions. Expected a single partition.\r\n");
                    result = __LINE__;
                }
            }
        }
        return result;
    }

    static void EventHubReceiversTest_Destroy(EVENTHUB_RECEIVERS_DATA* receiversData)
    {
        if (receiversData != NULL)
        {
            size_t idx;
            for (idx = 0; idx < receiversData->numReceivers; idx++)
            {
                AsyncDataCallbackListDeInit(&receiversData->receivers[idx].receiveCallbackDataList);
                EventHubReceiver_Destroy(receiversData->receivers[idx].receiver);
            }
            free(receiversData->receivers);
            (void)Lock_Deinit(receiversData->receiversLock);
            free(receiversData);
        }
    }

    static EVENTHUB_RECEIVERS_DATA* EventHubReceiversTest_Create_EnqueuedTimestamp
    (
        const char* connectionString,
        const char* eventHubPath,
        const char* consumerGroup,
        size_t numPartitions,
        uint64_t enqueuedTimestamp,
        unsigned int waitTimoutMs
    )
    {
        EVENTHUB_RECEIVERS_DATA* receiversData = NULL;
        if ((numPartitions > 0) && (numPartitions < USHRT_MAX))
        {
            char partitionBuffer[MAX_UNSIGNED_SHORT_BASE_10_LEN];

            if ((receiversData = (EVENTHUB_RECEIVERS_DATA*)malloc(sizeof(EVENTHUB_RECEIVERS_DATA))) == NULL)
            {
                LogError("Could not allocate EVENTHUB_RECEIVERS_DATA \r\n");
            }
            else
            {
                receiversData->numReceivers = numPartitions;
                if ((receiversData->receivers = (EVENTHUB_RECEIVER_TEST_DATA*)malloc(sizeof(EVENTHUB_RECEIVER_TEST_DATA) * numPartitions)) == NULL)
                {
                    LogError("Could not allocate EVENTHUB_RECEIVER_TEST_DATA \r\n");
                    free(receiversData);
                    receiversData = NULL;
                }
                else if ((receiversData->receiversLock = Lock_Init()) == NULL)
                {
                    LogError("Could not allocate EVENTHUB_RECEIVER_TEST_DATA \r\n");
                    free(receiversData->receivers);
                    free(receiversData);
                    receiversData = NULL;
                }
                else
                {
                    EVENTHUBRECEIVER_RESULT receiverResult;
                    size_t idx;
                    int done, errorSeen = 0;
                    for (idx = 0; idx < numPartitions; idx++)
                    {
                        receiversData->receivers[idx].exitConditionObserved = 0;
                        receiversData->receivers[idx].errorCode = EVENTHUBRECEIVER_OK;
                        receiversData->receivers[idx].receiver = NULL;
                        AsyncDataCallbackListInit(&receiversData->receivers[idx].receiveCallbackDataList);
                    }
                    for (idx = 0; idx < numPartitions; idx++)
                    {
                        snprintf(partitionBuffer, MAX_UNSIGNED_SHORT_BASE_10_LEN, "%u", (unsigned int)idx);
                        receiversData->receivers[idx].receiver = EventHubReceiver_Create(connectionString, eventHubPath, consumerGroup, partitionBuffer);
                        if (receiversData->receivers[idx].receiver == NULL)
                        {
                            LogError("Could not create receiver using EventHubReceiver_Create for index %u\r\n", idx);
                            errorSeen = 1;
                            break;
                        }
                        receiversData->receivers[idx].lock = receiversData->receiversLock;
                        (void)EventHubReceiver_SetConnectionTracing(receiversData->receivers[idx].receiver, true);
                        receiverResult = EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync(receiversData->receivers[idx].receiver,
                                                                                                    OnReceiveCB,
                                                                                                    &receiversData->receivers[idx],
                                                                                                    OnReceiveErrorCB,
                                                                                                    &receiversData->receivers[idx],
                                                                                                    enqueuedTimestamp, waitTimoutMs);
                        if (receiverResult != EVENTHUBRECEIVER_OK)
                        {
                            LogError("EventHubReceiver_ReceiveFromStartTimestampWithTimeoutAsync Failed for index %u. Code:%u\r\n", idx, receiverResult);
                            errorSeen = 1;
                            break;
                        }
                    }
                    if (errorSeen)
                    {
                        for (idx = 0; idx < numPartitions; idx++)
                        {
                            if (receiversData->receivers[idx].receiver != NULL)
                            {
                                EventHubReceiver_Destroy(receiversData->receivers[idx].receiver);
                            }
                        }
                        free(receiversData->receivers);
                        (void)Lock_Deinit(receiversData->receiversLock);
                        free(receiversData);
                        receiversData = NULL;
                    }
                    else
                    {
                        do
                        {
                            LOCK_RESULT lockResult;
                            done = 1;
                            ThreadAPI_Sleep(RECEIVER_SLEEP_TIME_MS);
                            lockResult = Lock(receiversData->receiversLock);
                            ASSERT_ARE_EQUAL_WITH_MSG(int, LOCK_OK, lockResult, "Lock Failure");
                            for (idx = 0; idx < numPartitions; idx++)
                            {
                                if (receiversData->receivers[idx].exitConditionObserved == 0)
                                {
                                    done = 0;
                                    break;
                                }
                            }
                            lockResult = Unlock(receiversData->receiversLock);
                            ASSERT_ARE_EQUAL_WITH_MSG(int, LOCK_OK, lockResult, "Unlock Failure");
                        } while (!done);
                    }
                }
            }
        }

        return receiversData;
    }

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        int result = platform_init();
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, result, "Failed initializing platform!\r\n");
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
        platform_deinit();
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
        /* arrange */
        size_t messageToCreate = 1;
        uint64_t timestampNow = static_cast<uint64_t>(time(NULL));

        // setup test data
        EXPECTED_RECEIVE_DATA* messageData = MessageData_Create(messageToCreate, timestampNow);
        ASSERT_IS_NOT_NULL_WITH_MSG(messageData, "Could not create messageData using MessageData_Create\r\n");

        EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory( (const unsigned char*)messageData->data[0], messageData->dataSize[0]);
        ASSERT_IS_NOT_NULL_WITH_MSG(eventDataHandle, "Could not create eventDataHandle using EventData_CreateWithNewMemory\r\n");

        /* arrange event hub send */
        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(EventHubAccount_GetConnectionString(), EventHubAccount_GetName() );
        ASSERT_IS_NOT_NULL_WITH_MSG(eventHubClientHandle, "Could not create eventHubClientHandle using EventHubClient_CreateFromConnectionString\r\n");

        EventHubClient_SetLogTrace(eventHubClientHandle, true);

        EVENTHUBCLIENT_RESULT result = EventHubClient_Send(eventHubClientHandle, eventDataHandle);
        ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBCLIENT_OK, result, "EventHubClient_Send Failed.\r\n");

        // since data send was successful setup the messageData accordingly
        messageData->dataWasSent = true;
        messageData->eventhubConfirmResult = EVENTHUBCLIENT_CONFIRMATION_OK;

        /* arrange event hub receive */
        // create receivers to read the sent data and validate against the expected test data
        EVENTHUB_RECEIVERS_DATA* receiversData = EventHubReceiversTest_Create_EnqueuedTimestamp(EventHubAccount_GetConnectionString(),
                                                                                                EventHubAccount_GetName(),
                                                                                                EventHubAccount_ConsumerGroup(),
                                                                                                EventHubAccount_PartitionCount(),
                                                                                                timestampNow, MAX_EXECUTE_TIME_MS);
        ASSERT_IS_NOT_NULL_WITH_MSG(receiversData, "EventHubReceiversTest_Create_EnqueuedTimestamp Failed\r\n");

        int validationResult;
        validationResult = EventHubReceiversTest_ValidateReceivedMessageData(receiversData, messageData);
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, validationResult, "Received Data Validation Failed\r\n");

        /* cleanup */
        // destroy the sender data
        EventHubClient_Destroy(eventHubClientHandle);
        
        // destroy data handle(s)
        EventData_Destroy(eventDataHandle);

        // destroy the receiver(s)
        EventHubReceiversTest_Destroy(receiversData);

        // destroy the test message data
        MessageData_Destroy(messageData);
    }


    TEST_FUNCTION(EventHub_SendTelemetryBatchWithPartitionKey_E2ETest)
    {
        /* arrange */
        size_t messageToCreate = MAX_NUM_OF_MESSAGES;
        uint64_t timestampNow = static_cast<uint64_t>(time(NULL));

        // setup test data
        EXPECTED_RECEIVE_DATA* messageData = MessageData_Create(messageToCreate, timestampNow);
        ASSERT_IS_NOT_NULL_WITH_MSG(messageData, "Could not create messageData using MessageData_Create\r\n");

        EVENTDATA_HANDLE eventDataList[MAX_NUM_OF_MESSAGES];
        for (size_t index = 0; index < messageToCreate; index++)
        {
            EVENTDATA_RESULT eventDataResult;
            eventDataList[index] = EventData_CreateWithNewMemory((const unsigned char*)messageData->data[index], messageData->dataSize[index]);
            ASSERT_IS_NOT_NULL_WITH_MSG(eventDataList[index], "Could not create eventDataHandle using EventData_CreateWithNewMemory\r\n");
            eventDataResult = EventData_SetPartitionKey(eventDataList[index], "PartitionKeyTest");
            ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTDATA_OK, eventDataResult, "Failed in Setting PartitionKey\r\n");
        }
        messageData->partitionKeyWasSet = true;

        /* arrange event hub send */
        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(EventHubAccount_GetConnectionString(), EventHubAccount_GetName());
        ASSERT_IS_NOT_NULL_WITH_MSG(eventHubClientHandle, "Could not create eventHubClientHandle using EventHubClient_CreateFromConnectionString\r\n");

        EventHubClient_SetLogTrace(eventHubClientHandle, true);

        EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatch(eventHubClientHandle, eventDataList, messageToCreate);
        ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBCLIENT_OK, result, "EventHubClient_SendBatch Failed.\r\n");

        // since data send was successful setup the messageData accordingly
        messageData->dataWasSent = true;
        messageData->eventhubConfirmResult = EVENTHUBCLIENT_CONFIRMATION_OK;

        /* arrange event hub receive */
        // create receivers to read the sent data and validate against the expected test data
        EVENTHUB_RECEIVERS_DATA* receiversData = EventHubReceiversTest_Create_EnqueuedTimestamp(EventHubAccount_GetConnectionString(),
                                                                                                EventHubAccount_GetName(),
                                                                                                EventHubAccount_ConsumerGroup(),
                                                                                                EventHubAccount_PartitionCount(),
                                                                                                timestampNow, MAX_EXECUTE_TIME_MS);
        ASSERT_IS_NOT_NULL_WITH_MSG(receiversData, "EventHubReceiversTest_Create_EnqueuedTimestamp Failed\r\n");

        int validationResult;
        validationResult = EventHubReceiversTest_ValidateReceivedMessageData(receiversData, messageData);
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, validationResult, "Received Data Validation Failed\r\n");

        /* cleanup */
        // destroy the sender data
        EventHubClient_Destroy(eventHubClientHandle);

        // destroy data handle(s)
        for (size_t index = 0; index < messageToCreate; index++)
        {
            EventData_Destroy(eventDataList[index]);
        }

        // destroy the receiver(s)
        EventHubReceiversTest_Destroy(receiversData);

        // destroy the test message data
        MessageData_Destroy(messageData);
    }

    TEST_FUNCTION(EventHub_SendTelemetryBatch_E2ETest)
    {
        /* arrange */
        size_t messageToCreate = MAX_NUM_OF_MESSAGES;
        uint64_t timestampNow = static_cast<uint64_t>(time(NULL));

        // setup test data
        EXPECTED_RECEIVE_DATA* messageData = MessageData_Create(messageToCreate, timestampNow);
        ASSERT_IS_NOT_NULL_WITH_MSG(messageData, "Could not create messageData using MessageData_Create\r\n");

        EVENTDATA_HANDLE eventDataList[MAX_NUM_OF_MESSAGES];
        for (size_t index = 0; index < messageToCreate; index++)
        {
            eventDataList[index] = EventData_CreateWithNewMemory((const unsigned char*)messageData->data[index], messageData->dataSize[index]);
            ASSERT_IS_NOT_NULL_WITH_MSG(eventDataList[index], "Could not create eventDataHandle using EventData_CreateWithNewMemory\r\n");
        }

        /* arrange event hub send */
        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(EventHubAccount_GetConnectionString(), EventHubAccount_GetName());
        ASSERT_IS_NOT_NULL_WITH_MSG(eventHubClientHandle, "Could not create eventHubClientHandle using EventHubClient_CreateFromConnectionString\r\n");

        EventHubClient_SetLogTrace(eventHubClientHandle, true);

        EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatch(eventHubClientHandle, eventDataList, messageToCreate);
        ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBCLIENT_OK, result, "EventHubClient_SendBatch Failed.\r\n");

        // since data send was successful setup the messageData accordingly
        messageData->dataWasSent = true;
        messageData->eventhubConfirmResult = EVENTHUBCLIENT_CONFIRMATION_OK;

        /* arrange event hub receive */
        // create receivers to read the sent data and validate against the expected test data
        EVENTHUB_RECEIVERS_DATA* receiversData = EventHubReceiversTest_Create_EnqueuedTimestamp(EventHubAccount_GetConnectionString(),
                                                                                                EventHubAccount_GetName(),
                                                                                                EventHubAccount_ConsumerGroup(),
                                                                                                EventHubAccount_PartitionCount(),
                                                                                                timestampNow, MAX_EXECUTE_TIME_MS);
        ASSERT_IS_NOT_NULL_WITH_MSG(receiversData, "EventHubReceiversTest_Create_EnqueuedTimestamp Failed\r\n");

        int validationResult;
        validationResult = EventHubReceiversTest_ValidateReceivedMessageData(receiversData, messageData);
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, validationResult, "Received Data Validation Failed\r\n");

        /* cleanup */
        // destroy the sender data
        EventHubClient_Destroy(eventHubClientHandle);

        // destroy data handle(s)
        for (size_t index = 0; index < messageToCreate; index++)
        {
            EventData_Destroy(eventDataList[index]);
        }

        // destroy the receiver(s)
        EventHubReceiversTest_Destroy(receiversData);

        // destroy the test message data
        MessageData_Destroy(messageData);
    }

    TEST_FUNCTION(EventHub_SendTelemetryBatchAsync_E2ETest)
    {
        /* arrange */
        size_t messageToCreate = MAX_NUM_OF_MESSAGES;
        uint64_t timestampNow = static_cast<uint64_t>(time(NULL));

        // setup test data
        EXPECTED_RECEIVE_DATA* messageData = MessageData_Create(messageToCreate, timestampNow);
        ASSERT_IS_NOT_NULL_WITH_MSG(messageData, "Could not create messageData using MessageData_Create\r\n");

        EVENTDATA_HANDLE eventDataList[MAX_NUM_OF_MESSAGES];
        for (size_t index = 0; index < messageToCreate; index++)
        {
            eventDataList[index] = EventData_CreateWithNewMemory((const unsigned char*)messageData->data[index], messageData->dataSize[index]);
            ASSERT_IS_NOT_NULL_WITH_MSG(eventDataList[index], "Could not create eventDataHandle using EventData_CreateWithNewMemory\r\n");
        }

        /* arrange event hub send */
        EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(EventHubAccount_GetConnectionString(), EventHubAccount_GetName());
        ASSERT_IS_NOT_NULL_WITH_MSG(eventHubClientHandle, "Could not create eventHubClientHandle using EventHubClient_CreateFromConnectionString\r\n");

        EventHubClient_SetLogTrace(eventHubClientHandle, true);

        EVENTHUBCLIENT_RESULT result = EventHubClient_SendBatchAsync(eventHubClientHandle, eventDataList, messageToCreate, EventhubClientCallback, messageData);
        ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBCLIENT_OK, result, "EventHubClient_SendBatchAsync Failed.\r\n");

        time_t beginOp = time(NULL);
        while (
            (!messageData->dataWasSent) &&
            (difftime(time(NULL), beginOp) < MAX_CLOUD_TRAVEL_TIME)
            )
        {
            // give it some time
        }

        /* arrange event hub receive */
        // create receivers to read the sent data and validate against the expected test data
        EVENTHUB_RECEIVERS_DATA* receiversData = EventHubReceiversTest_Create_EnqueuedTimestamp(EventHubAccount_GetConnectionString(),
                                                                                                EventHubAccount_GetName(),
                                                                                                EventHubAccount_ConsumerGroup(),
                                                                                                EventHubAccount_PartitionCount(),
                                                                                                timestampNow, MAX_EXECUTE_TIME_MS);
        ASSERT_IS_NOT_NULL_WITH_MSG(receiversData, "EventHubReceiversTest_Create_EnqueuedTimestamp Failed\r\n");

        int validationResult;
        validationResult = EventHubReceiversTest_ValidateReceivedMessageData(receiversData, messageData);
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, validationResult, "Received Data Validation Failed\r\n");

        /* cleanup */
        // destroy the sender data
        EventHubClient_Destroy(eventHubClientHandle);

        // destroy data handle(s)
        for (size_t index = 0; index < messageToCreate; index++)
        {
            EventData_Destroy(eventDataList[index]);
        }

        // destroy the receiver(s)
        EventHubReceiversTest_Destroy(receiversData);

        // destroy the test message data
        MessageData_Destroy(messageData);
    }

    TEST_FUNCTION(EventHub_NoSendReceiveTimeout_E2ETests)
    {
        /* arrange */
        size_t messageToCreate = 0;
        uint64_t timestampNow = static_cast<uint64_t>(time(NULL));

        // setup test data
        EXPECTED_RECEIVE_DATA* messageData = MessageData_Create(messageToCreate, timestampNow);
        ASSERT_IS_NOT_NULL_WITH_MSG(messageData, "Could not create messageData using MessageData_Create\r\n");

        /* arrange event hub receive */
        // create receivers to read the sent data and validate against the expected test data
        EVENTHUB_RECEIVERS_DATA* receiversData = EventHubReceiversTest_Create_EnqueuedTimestamp(EventHubAccount_GetConnectionString(),
                                                                                                EventHubAccount_GetName(),
                                                                                                EventHubAccount_ConsumerGroup(),
                                                                                                EventHubAccount_PartitionCount(),
                                                                                                timestampNow, MAX_EXECUTE_TIME_MS);
        ASSERT_IS_NOT_NULL_WITH_MSG(receiversData, "EventHubReceiversTest_Create_EnqueuedTimestamp Failed\r\n");

        int validationResult;
        validationResult = EventHubReceiversTest_ValidateReceivedMessageData(receiversData, messageData);
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, validationResult, "Received Data Validation Failed\r\n");

        /* cleanup */
        // destroy the receiver(s)
        EventHubReceiversTest_Destroy(receiversData);

        // destroy the test message data
        MessageData_Destroy(messageData);
    }

END_TEST_SUITE(eventhubclient_e2etests)

