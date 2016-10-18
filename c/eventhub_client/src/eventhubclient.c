// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include <eventhubclient.h>
#include "azure_c_shared_utility/xlogging.h"
#include "version.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/condition.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/lock.h"
#include <signal.h>

#define EVENTHUB_SEND_SLEEP_TIME		1 // One Millisecond
#define THREAD_CONTINUE					0
#define THREAD_END						1
#define CALLBACK_WAITING				0
#define CALLBACK_NOTIFIED				1 

#define LOG_ERROR(x) LogError("result = %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, x));

typedef struct EVENTHUBCLIENT_STRUCT_TAG
{
    EVENTHUBCLIENT_LL_HANDLE eventhubclientLLHandle;
    THREAD_HANDLE threadHandle;
    LOCK_HANDLE lockInfo;
    volatile sig_atomic_t threadToContinue;
} EVENTHUBCLIENT_STRUCT;

typedef struct EVENTHUB_CALLBACK_STRUCT_TAG
 {
     volatile sig_atomic_t callbackStatus;
     EVENTHUBCLIENT_CONFIRMATION_RESULT confirmationResult;
     LOCK_HANDLE completionLock;
     COND_HANDLE completionCondition;
 } EVENTHUB_CALLBACK_STRUCT;
 

static int EventhubClientThread(void* userContextCallback)
{
    EVENTHUBCLIENT_STRUCT* eventhubInfo = (EVENTHUBCLIENT_STRUCT*)userContextCallback;
    while (eventhubInfo->threadToContinue == THREAD_CONTINUE)
    {
        if (Lock(eventhubInfo->lockInfo) == LOCK_OK)
        {
            EventHubClient_LL_DoWork(eventhubInfo->eventhubclientLLHandle);
            (void)Unlock(eventhubInfo->lockInfo);
        }
        else
        {
            LOG_ERROR(EVENTHUBCLIENT_ERROR);
        }
    }
    return 0;
}

static void EventhubClientLLCallback(EVENTHUBCLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    if (userContextCallback != NULL)
    {
        EVENTHUB_CALLBACK_STRUCT* callbackInfo = (EVENTHUB_CALLBACK_STRUCT*)userContextCallback;
        callbackInfo->callbackStatus = CALLBACK_NOTIFIED;
        callbackInfo->confirmationResult = result;
        Condition_Post(callbackInfo->completionCondition);
    }
}

EVENTHUB_CALLBACK_STRUCT * EventHubClient_InitUserContext(void)
{
    EVENTHUB_CALLBACK_STRUCT* eventhubUserContext = malloc(sizeof(EVENTHUB_CALLBACK_STRUCT) );
    if ( eventhubUserContext != NULL)
    {
        eventhubUserContext->callbackStatus = CALLBACK_WAITING;
        eventhubUserContext->confirmationResult = -1;
        // init and set the lock. completion unlocks
        eventhubUserContext->completionLock = Lock_Init();
       Lock(eventhubUserContext->completionLock);
        eventhubUserContext->completionCondition = Condition_Init();
    }
    return eventhubUserContext;
}

void EventHub_DestroyUserContext(EVENTHUB_CALLBACK_STRUCT * eventhubUserContext)
{
    Unlock(eventhubUserContext->completionLock);
    Lock_Deinit(eventhubUserContext->completionLock);
    Condition_Deinit(eventhubUserContext->completionCondition);
    free(eventhubUserContext);
}

static int Create_DoWorkThreadIfNeccesary(EVENTHUBCLIENT_STRUCT* eventhubClientInfo)
{
    int result;
    // Create the thread if neccessary
    if (eventhubClientInfo->threadHandle == NULL)
    {
        /* Codes SRS_EVENTHUBCLIENT_07_034: [Create_DoWorkThreadIfNeccesary shall use the ThreadAPI_Create API to create a thread and execute EventhubClientThread function.] */
        THREADAPI_RESULT threadResult = ThreadAPI_Create(&eventhubClientInfo->threadHandle, EventhubClientThread, eventhubClientInfo);
        if (threadResult != THREADAPI_OK)
        {
            /* Codes_SRS_EVENTHUBCLIENT_07_035: [Create_DoWorkThreadIfNeccesary shall return EVENTHUBCLIENT_ERROR if any failure is encountered.] */
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        /* Codes_SRS_EVENTHUBCLIENT_07_033: [Create_DoWorkThreadIfNeccesary shall set result to EVENTHUBCLIENT_OK and return if threadHandle parameter is not a NULL value.] */
        result = 0;
    }
    return result;
}

static int Execute_LowerLayerSendAsync(EVENTHUBCLIENT_STRUCT* eventhubClientInfo, EVENTDATA_HANDLE eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK notificationCallback, void* userContextCallback)
{
    int result;

    /* Codes_SRS_EVENTHUBCLIENT_07_029: [Execute_LowerLayerSendAsync shall Lock on the EVENTHUBCLIENT_STRUCT lockInfo to protect calls to Lower Layer and Thread function calls.] */
    if (Lock(eventhubClientInfo->lockInfo) == LOCK_OK)
    {
        /* Codes_SRS_EVENTHUBCLIENT_07_031: [Execute_LowerLayerSendAsync shall call into the Create_DoWorkThreadIfNeccesary function to create the DoWork thread.]*/
        if (Create_DoWorkThreadIfNeccesary(eventhubClientInfo) == 0)
        {
            /* Codes_SRS_EVENTHUBCLIENT_07_038: [Execute_LowerLayerSendAsync shall call EventHubClient_LL_SendAsync to send data to the Eventhub Endpoint.] */
            result = EventHubClient_LL_SendAsync(eventhubClientInfo->eventhubclientLLHandle, eventDataHandle, notificationCallback, userContextCallback);
            if (result != EVENTHUBCLIENT_OK)
            {
                /* Codes_SRS_EVENTHUBCLIENT_07_039: [If the EventHubClient_LL_SendAsync call fails then Execute_LowerLayerSendAsync shall return a nonzero value.] */
                result = __LINE__;
            }
            else
            {
                /* Codes_SRS_EVENTHUBCLIENT_07_028: [If Execute_LowerLayerSendAsync is successful then it shall return 0.] */
                result = 0;
            }
        }
        else
        {
            /* Codes_SRS_EVENTHUBCLIENT_07_032: [If Create_DoWorkThreadIfNeccesary does not return 0 then Execute_LowerLayerSendAsync shall return a nonzero value.] */
            result = __LINE__;
        }
        (void)Unlock(eventhubClientInfo->lockInfo);
    }
    else
    {
        /* Codes_SRS_EVENTHUBCLIENT_07_030: [Execute_LowerLayerSendAsync shall return a nonzero value if it is unable to obtain the lock with the Lock function.] */
        result = __LINE__;
    }
    return result;
}

static int Execute_LowerLayerSendBatchAsync(EVENTHUBCLIENT_STRUCT* eventhubClientInfo, EVENTDATA_HANDLE* eventDataList, size_t count, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK notificationCallback, void* userContextCallback)
{
    int result;

    /* Codes_SRS_EVENTHUBCLIENT_07_043: [Execute_LowerLayerSendBatchAsync shall Lock on the EVENTHUBCLIENT_STRUCT lockInfo to protect calls to Lower Layer and Thread function calls.] */
    if (Lock(eventhubClientInfo->lockInfo) == LOCK_OK)
    {
        /* Codes_SRS_EVENTHUBCLIENT_07_045: [Execute_LowerLayerSendAsync shall call into the Create_DoWorkThreadIfNeccesary function to create the DoWork thread.] */
        if (Create_DoWorkThreadIfNeccesary(eventhubClientInfo) == 0)
        {
            /* Codes_SRS_EVENTHUBCLIENT_07_048: [Execute_LowerLayerSendAsync shall call EventHubClient_LL_SendAsync to send data to the Eventhub Endpoint.] */
            result = EventHubClient_LL_SendBatchAsync(eventhubClientInfo->eventhubclientLLHandle, eventDataList, count, notificationCallback, userContextCallback);
            if (result != EVENTHUBCLIENT_OK)
            {
                /* Codes_SRS_EVENTHUBCLIENT_07_049: [If the EventHubClient_LL_SendAsync call fails then Execute_LowerLayerSendAsync shall return a nonzero value.] */
                result = __LINE__;
            }
            else
            {
                /* Codes_SRS_EVENTHUBCLIENT_07_047: [If Execute_LowerLayerSendAsync is successful then it shall return 0.] */
                result = 0;
            }
        }
        else
        {
            /* Code_SRS_EVENTHUBCLIENT_07_046: [If Create_DoWorkThreadIfNeccesary does not return 0 then Execute_LowerLayerSendAsync shall return a nonzero value.] */
            result = __LINE__;
        }
        (void)Unlock(eventhubClientInfo->lockInfo);
    }
    else
    {
        /* Codes_SRS_EVENTHUBCLIENT_07_044: [Execute_LowerLayerSendBatchAsync shall return a nonzero value if it is unable to obtain the lock with the Lock function.] */
        result = __LINE__;
    }
    return result;
}

EVENTHUBCLIENT_HANDLE EventHubClient_CreateFromConnectionString(const char* connectionString, const char* eventHubPath)
{
    EVENTHUBCLIENT_LL_HANDLE llHandle;
    EVENTHUBCLIENT_STRUCT* result;

    /* Codes_SRS_EVENTHUBCLIENT_03_004: [EventHubClient_CreateFromConnectionString shall pass the connectionString and eventHubPath variables to EventHubClient_CreateFromConnectionString_LL.] */
    llHandle = EventHubClient_LL_CreateFromConnectionString(connectionString, eventHubPath);
    if (llHandle == NULL)
    {
        /* Codes_SRS_EVENTHUBCLIENT_03_006: [EventHubClient_CreateFromConnectionString shall return a NULL value if EventHubClient_CreateFromConnectionString_LL  returns NULL.] */
        result = NULL;
        LOG_ERROR(EVENTHUBCLIENT_INVALID_ARG);
    }
    else
    {
        result = malloc(sizeof(EVENTHUBCLIENT_STRUCT));
        if (result == NULL)
        {
            /* Codes_SRS_EVENTHUBCLIENT_03_006: [EventHubClient_CreateFromConnectionString shall return a NULL value if EventHubClient_CreateFromConnectionString_LL returns NULL.] */
            EventHubClient_LL_Destroy(llHandle);
            LOG_ERROR(EVENTHUBCLIENT_ERROR);
        }
        else
        {
            if ( (result->lockInfo = Lock_Init() ) == NULL)
            {
                /* Codes_SRS_EVENTHUBCLIENT_03_006: [EventHubClient_CreateFromConnectionString shall return a NULL value if EventHubClient_CreateFromConnectionString_LL  returns NULL.] */
                EventHubClient_LL_Destroy(llHandle);
                free(result);
                result = NULL;
                LOG_ERROR(EVENTHUBCLIENT_ERROR);
            }
            else
            {
                /* Codes_SRS_EVENTHUBCLIENT_03_002: [Upon Success of EventHubClient_CreateFromConnectionString_LL,  EventHubClient_CreateFromConnectionString shall allocate the internal structures required by this module.] */
                result->eventhubclientLLHandle = llHandle;
                result->threadHandle = NULL;
                result->threadToContinue = THREAD_CONTINUE;
            }
        }
    }
    /* Codes_SRS_EVENTHUBCLIENT_03_005: [Upon Success EventHubClient_CreateFromConnectionString shall return the EVENTHUBCLIENT_HANDLE.] */
    return (EVENTHUBCLIENT_LL_HANDLE)result;
}

EVENTHUBCLIENT_RESULT EventHubClient_Send(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE eventDataHandle)
{
    EVENTHUBCLIENT_RESULT result;

    if (eventHubHandle == NULL || eventDataHandle == NULL)
    {
        /* Codes_SRS_EVENTHUBCLIENT_03_007: [EventHubClient_Send shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL.] */
        result = EVENTHUBCLIENT_INVALID_ARG;
        LOG_ERROR(result);
    }
    else
    {
        EVENTHUBCLIENT_STRUCT* eventhubClientInfo = (EVENTHUBCLIENT_STRUCT*)eventHubHandle;
        EVENTHUB_CALLBACK_STRUCT* eventhubUserContext = EventHubClient_InitUserContext();
        if (eventhubUserContext == NULL)
        {
            /* Codes_SRS_EVENTHUBCLIENT_03_009: [EventHubClient_Send shall return EVENTHUBCLIENT_ERROR on any failure that is encountered..] */
            result = EVENTHUBCLIENT_ERROR;
            LOG_ERROR(result);
        }
        else
        {
            eventhubUserContext->callbackStatus = CALLBACK_WAITING;
            /* Codes_SRS_EVENTHUBCLIENT_03_008: [EventHubClient_Send shall call into the Execute_LowerLayerSendAsync function to send the eventDataHandle parameter to the EventHub.] */
            if (Execute_LowerLayerSendAsync(eventhubClientInfo, eventDataHandle, EventhubClientLLCallback, eventhubUserContext) == 0)
            {
                Condition_Wait(eventhubUserContext->completionCondition, eventhubUserContext->completionLock, 0);

                /* Codes_SRS_EVENTHUBCLIENT_07_012: [EventHubClient_Send shall return EVENTHUBCLIENT_ERROR.] */
                if (eventhubUserContext->confirmationResult == EVENTHUBCLIENT_CONFIRMATION_OK)
                {
                    /* Codes_SRS_EVENTHUBCLIENT_03_013: [EventHubClient_Send shall return EVENTHUBCLIENT_OK upon successful completion of the SendDataAsync and the callback function.] */
                    result = EVENTHUBCLIENT_OK;
                }
                else
                {
                    /* Codes_SRS_EVENTHUBCLIENT_03_009: [EventHubClient_Send shall return EVENTHUBCLIENT_ERROR on any failure that is encountered.] */
                    result = EVENTHUBCLIENT_ERROR;
                    LOG_ERROR(result);
                }
            }
            else
            {
                /* Codes_SRS_EVENTHUBCLIENT_03_009: [EventHubClient_Send shall return EVENTHUBCLIENT_ERROR on any failure that is encountered.] */
                result = EVENTHUBCLIENT_ERROR;
                LOG_ERROR(result);
            }
            EventHub_DestroyUserContext(eventhubUserContext);
        }
    }
    return result;
}

EVENTHUBCLIENT_RESULT EventHubClient_SendAsync(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK notificationCallback, void* userContextCallback)
{
    EVENTHUBCLIENT_RESULT result;

    /* Codes_SRS_EVENTHUBCLIENT_03_021: [EventHubClient_SendAsync shall return EVENTHUBCLIENT_INVALID_ARG if either eventHubHandle or eventDataHandle is NULL.] */
    if (eventHubHandle == NULL || eventDataHandle == NULL)
    {
        result = EVENTHUBCLIENT_INVALID_ARG;
        LOG_ERROR(result);
    }
    else
    {
        /* Codes_SRS_EVENTHUBCLIENT_07_022: [EventHubClient_SendAsync shall call into Execute_LowerLayerSendAsync and return EVENTHUBCLIENT_ERROR on a nonzero return value.] */
        EVENTHUBCLIENT_STRUCT* eventhubClientInfo = (EVENTHUBCLIENT_STRUCT*)eventHubHandle;
        if (Execute_LowerLayerSendAsync(eventhubClientInfo, eventDataHandle, notificationCallback, userContextCallback) != 0)
        {
            result = EVENTHUBCLIENT_ERROR;
        }
        else
        {
            /* Codes_SRS_EVENTHUBCLIENT_07_037: [On Success EventHubClient_SendAsync shall return EVENTHUBCLIENT_OK.] */
            result = EVENTHUBCLIENT_OK;
        }
    }
    return result;
}

EVENTHUBCLIENT_RESULT EventHubClient_SendBatch(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE *eventDataList, size_t count)
{
    EVENTHUBCLIENT_RESULT result;

    if (eventHubHandle == NULL || eventDataList == NULL || (count == 0))
    {
        result = EVENTHUBCLIENT_INVALID_ARG;
        LOG_ERROR(result);
    }
    else
    {
        EVENTHUBCLIENT_STRUCT* eventhubClientInfo = (EVENTHUBCLIENT_STRUCT*)eventHubHandle;
        EVENTHUB_CALLBACK_STRUCT* eventhubUserContext = EventHubClient_InitUserContext();
        if (eventhubUserContext == NULL)
        {
            /* Codes_SRS_EVENTHUBCLIENT_07_050: [EventHubClient_SendBatch shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL.] */
            result = EVENTHUBCLIENT_ERROR;
            LOG_ERROR(result);
        }
        else
        {
            eventhubUserContext->callbackStatus = CALLBACK_WAITING;
            /* Codes_SRS_EVENTHUBCLIENT_07_051: [EventHubClient_SendBatch shall call into the Execute_LowerLayerSendBatchAsync function to send the eventDataHandle parameter to the EventHub.] */
            if (Execute_LowerLayerSendBatchAsync(eventhubClientInfo, eventDataList, count, EventhubClientLLCallback, eventhubUserContext) == 0)
            {
                Condition_Wait(eventhubUserContext->completionCondition, eventhubUserContext->completionLock, 0);

                if (eventhubUserContext->confirmationResult == EVENTHUBCLIENT_CONFIRMATION_OK)
                {
                    /* Codes_SRS_EVENTHUBCLIENT_07_054: [EventHubClient_SendBatch shall return EVENTHUBCLIENT_OK upon successful completion of the Execute_LowerLayerSendBatchAsync and the callback function.] */
                    result = EVENTHUBCLIENT_OK;
                }
                else
                {
                    /* Codes_SRS_EVENTHUBCLIENT_07_052: [EventHubClient_SendBatch shall return EVENTHUBCLIENT_ERROR on any failure that is encountered.] */
                    result = EVENTHUBCLIENT_ERROR;
                    LOG_ERROR(result);
                }
            }
            else
            {
                /* Codes_SRS_EVENTHUBCLIENT_07_052: [EventHubClient_SendBatch shall return EVENTHUBCLIENT_ERROR on any failure that is encountered.]  */
                result = EVENTHUBCLIENT_ERROR;
                LOG_ERROR(result);
            }
            EventHub_DestroyUserContext(eventhubUserContext);
        }
    }
    return result;
}

EVENTHUBCLIENT_RESULT EventHubClient_SendBatchAsync(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE* eventDataList, size_t count, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsycCallback, void* userContextCallback)
{
    EVENTHUBCLIENT_RESULT result;
    /* Code_SRS_EVENTHUBCLIENT_07_040: [EventHubClient_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventHubHandle or eventDataHandle is NULL or count is zero.] */
    if (eventHubHandle == NULL || eventDataList == NULL || (count == 0) )
    {
        result = EVENTHUBCLIENT_INVALID_ARG;
        LOG_ERROR(result);
    }
    else
    {
        EVENTHUBCLIENT_STRUCT* eventhubClientInfo = (EVENTHUBCLIENT_STRUCT*)eventHubHandle;
        if (Execute_LowerLayerSendBatchAsync(eventhubClientInfo, eventDataList, count, sendAsycCallback, userContextCallback) != 0)
        {
            /* Code_SRS_EVENTHUBCLIENT_07_041: [EventHubClient_SendBatchAsync shall call into Execute_LowerLayerSendBatchAsync and return EVENTHUBCLIENT_ERROR on a nonzero return value.] */
            result = EVENTHUBCLIENT_ERROR;
        }
        else
        {
            /* Code_SRS_EVENTHUBCLIENT_07_042: [On Success EventHubClient_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
            result = EVENTHUBCLIENT_OK;
        }
    }
    return result;
}

void EventHubClient_Destroy(EVENTHUBCLIENT_HANDLE eventHubHandle)
{
    /* Codes_SRS_EVENTHUBCLIENT_03_018: [If the eventHubHandle is NULL, EventHubClient_Destroy shall not do anything.] */
    if (eventHubHandle != NULL)
    {
        EVENTHUBCLIENT_STRUCT* ehStruct = (EVENTHUBCLIENT_STRUCT*)eventHubHandle;
        /* Codes_SRS_EVENTHUBCLIENT_03_019: [EventHubClient_Destroy shall terminate the usage of this EventHubClient specified by the eventHubHandle and cleanup all associated resources.] */
        if (ehStruct->threadHandle != NULL)
        {
            int res = 0;
            ehStruct->threadToContinue = THREAD_END;
            if (ThreadAPI_Join(ehStruct->threadHandle, &res) != THREADAPI_OK)
            {
                LOG_ERROR(EVENTHUBCLIENT_ERROR);
            }
        }
        /* Codes_SRS_EVENTHUBCLIENT_03_020: [EventHubClient_Destroy shall call EventHubClient_LL_Destroy with the lower level handle.] */
        EventHubClient_LL_Destroy(ehStruct->eventhubclientLLHandle);
        Lock_Deinit(ehStruct->lockInfo);
        free(ehStruct);
    }
}
