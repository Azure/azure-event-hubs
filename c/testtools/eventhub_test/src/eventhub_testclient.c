/*
Microsoft Azure IoT Client Libraries

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

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "gballoc.h"
#include <stdio.h>

#include <time.h>
#include <signal.h>

#include <proton/message.h>
#include <proton/messenger.h>
#include <proton/error.h>

#include "crt_abstractions.h"
#include "eventhub_testclient.h"
#include "strings.h"
#include "threadapi.h"
#include "base64.h"
#include "iot_logging.h"


static const char* AMQP_RECV_ADDRESS_FMT = "amqps://ManageRule:%s@%s%s/%s/ConsumerGroups/$default/Partitions/%u";
static const char* HTTP_ADDRESS_PATH = "/api/commandandcontrol/InvokeAction";
static const char* HTTP_HOST_ADDRESS_FMT = "%s.azurewebsites.net";

static const char* DATA_PREFIX_FMT = "{\"DeviceId\":\"%s\",\"Action\":\"%s\"}";

DEFINE_ENUM_STRINGS(EVENTHUB_TEST_CLIENT_RESULT, EVENTHUB_TEST_CLIENT_RESULT_VALUES);

typedef struct EVENTHUB_TEST_CLIENT_INFO_TAG
{
    STRING_HANDLE eventHubName;
    char* svcBusName;
    char* manageKey;
    char* hostName;
    THREAD_HANDLE hThread;
    volatile sig_atomic_t messageThreadExit;
    unsigned int partitionCount;
} EVENTHUB_TEST_CLIENT_INFO;

#define PROTON_DEFAULT_TIMEOUT      3*1000

#define THREAD_CONTINUE             0
#define THREAD_END                  1

static const char* FILTER_MAP_KEY = "apache.org:selector-filter:string";
static const char* FILTER_MAP_VALUE_FMT = "amqp.annotation.x-opt-offset > '%d'";

static int AddTimeFilter(pn_message_t* message)
{
    int result;
    pn_data_t* partitionMsg = pn_message_annotations(message);
    if (partitionMsg == NULL)
    {
        LogError("Failed: pn_message_annotations returned NULL\r\n");
        result = __LINE__;
    }
    else if (pn_data_put_map(partitionMsg) != 0)
    {
        LogError("Failed: pn_data_put_map returned nonzero value\r\n");
        result = __LINE__;
    }
    else if (!pn_data_enter(partitionMsg))
    {
        LogError("Failed: pn_data_enter\r\n");
        result = __LINE__;
    }
    else if (pn_data_put_symbol(partitionMsg, pn_bytes(strlen(FILTER_MAP_KEY), FILTER_MAP_KEY)) != 0)
    {
        LogError("Failed: pn_data_put_symbol returned nonzero value\r\n");
        result = __LINE__;
    }
    else
    {
        char filterValue[128];
        sprintf_s(filterValue, 128, FILTER_MAP_VALUE_FMT, time(NULL) );
        if (pn_data_put_string(partitionMsg, pn_bytes(strlen(filterValue), filterValue)) != 0)
        {
            LogError("Failed: pn_data_put_string returned nonzero value\r\n");
            result = __LINE__;
        }
        else
        {
            pn_data_exit(partitionMsg);
            result = 0;
        }
    }
    return result;
}

static int RetrieveEventHubClientInfo(const char* pszconnString, EVENTHUB_TEST_CLIENT_INFO* evhInfo)
{
    int result;
    int beginSvcBus, endSvcBus;
    int beginHost, endHost;
    int beginKey;

    int endKey = strlen(pszconnString);
    if (sscanf(pszconnString, "Endpoint=sb://%n%*[^.]%n%n%*[^/]%n/;SharedAccessKeyName=ManageRule;SharedAccessKey=%n", &beginSvcBus, &endSvcBus, &beginHost, &endHost, &beginKey) != 0)
    {
        LogError("Endpoint formatting failure.  Expecting ManageRule Access\r\n");
        result = __LINE__;
    }
    else
    {
        if ((evhInfo->manageKey = (char*)malloc(endKey - beginKey + 1)) == NULL)
        {
            result = __LINE__;
        }
        else if ((evhInfo->hostName = (char*)malloc(endHost - beginHost + 1)) == NULL)
        {
            free(evhInfo->manageKey);
            result = __LINE__;
        }
        else if ((evhInfo->svcBusName = (char*)malloc(endSvcBus - beginSvcBus + 1)) == NULL)
        {
            free(evhInfo->svcBusName);
            free(evhInfo->manageKey);
            result = __LINE__;
        }
        else
        {
            if (sscanf(pszconnString, "Endpoint=sb://%[^.]%[^/]/;SharedAccessKeyName=ManageRule;SharedAccessKey=%s", evhInfo->svcBusName, evhInfo->hostName, evhInfo->manageKey) != 3)
            {
                free(evhInfo->hostName);
                free(evhInfo->manageKey);
                free(evhInfo->svcBusName);
                LogError("Endpoint formatting failure.  Expecting ManageRule Access\r\n");
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}

EVENTHUB_TEST_CLIENT_HANDLE EventHub_Initialize(const char* pszconnString, const char* pszeventHubName)
{
    EVENTHUB_TEST_CLIENT_HANDLE result;
    EVENTHUB_TEST_CLIENT_INFO* evhInfo;

    if (pszconnString == NULL || pszeventHubName == NULL)
    {
        result = NULL;
    }
    else if ((evhInfo = (EVENTHUB_TEST_CLIENT_INFO*)malloc(sizeof(EVENTHUB_TEST_CLIENT_INFO))) == NULL)
    {
        result = NULL;
    }
    else if ((evhInfo->eventHubName = STRING_construct(pszeventHubName)) == NULL)
    {
        free(evhInfo);
        result = NULL;
    }
    else if (RetrieveEventHubClientInfo(pszconnString, evhInfo) != 0)
    {
        STRING_delete(evhInfo->eventHubName);
        free(evhInfo);
        result = NULL;
    }
    else
    {
        evhInfo->hThread = NULL;
        evhInfo->messageThreadExit = THREAD_CONTINUE;
        evhInfo->partitionCount = 0;
        result = evhInfo;
    }
    return result;
}

/*this function will read messages from EventHub until exhaustion of all the messages (as indicated by a timeout in the proton messenger)*/
/*while reading the messages, it will invoke a callback. If the callback msgCallback is NULL, it will simply not call it*/
/*when the function returns all the existing messages have been consumed (given to the callback or simply skipped)*/
/*the callback shall return "0" if more messages are to be processed or "1" if no more messages are to be processed*/
/*the whole operation is time-bound*/
EVENTHUB_TEST_CLIENT_RESULT EventHub_ListenForMsg(EVENTHUB_TEST_CLIENT_HANDLE eventhubHandle, pfEventHubMessageCallback msgCallback, int partitionCount, double maxExecutionTimePerPartition, void* context)
{
    EVENTHUB_TEST_CLIENT_RESULT result;

    if (eventhubHandle == NULL)
    {
        result = EVENTHUB_TEST_CLIENT_INVALID_ARG;
    }
    else
    {
        EVENTHUB_TEST_CLIENT_INFO* evhInfo = (EVENTHUB_TEST_CLIENT_INFO*)eventhubHandle;
        time_t beginExecutionTime, nowExecutionTime;
        double timespan;
        pn_messenger_t* messenger;
        if ((messenger = pn_messenger(NULL)) == NULL)
        {
            result = EVENTHUB_TEST_CLIENT_ERROR;
        }
        else
        {
            // Sets the Messenger Windows
            pn_messenger_set_incoming_window(messenger, 1);

            pn_messenger_start(messenger);
            if (pn_messenger_errno(messenger))
            {
                result = EVENTHUB_TEST_CLIENT_ERROR;
            }
            else
            {
                if (pn_messenger_set_timeout(messenger, PROTON_DEFAULT_TIMEOUT) != 0)
                {
                    result = EVENTHUB_TEST_CLIENT_ERROR;
                }
                else
                {
                    result = EVENTHUB_TEST_CLIENT_OK;

                    evhInfo->partitionCount = partitionCount;
                    evhInfo->messageThreadExit = THREAD_CONTINUE;

                    // Allocate the Address
                    int addressLen = strlen(AMQP_RECV_ADDRESS_FMT) + strlen(evhInfo->manageKey) + strlen(evhInfo->svcBusName) + strlen(evhInfo->hostName) + STRING_length(evhInfo->eventHubName) + 5;
                    char* szAddress = (char*)malloc(addressLen + 1);
                    if (szAddress == NULL)
                    {
                        printf("error in malloc\r\n");
                    }
                    else
                    {
                        size_t part;
                        int pnErrorNo = 0;
                        int queueDepth = 0;
                        const char* eventhubName = STRING_c_str(evhInfo->eventHubName);

                        /*subscribe the messenger to all the partitions*/
                        for (part = 0; part < evhInfo->partitionCount; part++)
                        {
                            bool atLeastOneMessageReceived = true;
                            beginExecutionTime = time(NULL);
                            sprintf_s(szAddress, addressLen + 1, AMQP_RECV_ADDRESS_FMT, evhInfo->manageKey, evhInfo->svcBusName, evhInfo->hostName, eventhubName, part);
                            pn_messenger_subscribe(messenger, szAddress);
                            while (
                                (atLeastOneMessageReceived) &&
                                (evhInfo->messageThreadExit == THREAD_CONTINUE) &&
                                ((nowExecutionTime = time(NULL)), timespan = difftime(nowExecutionTime, beginExecutionTime), timespan < maxExecutionTimePerPartition)
                                )
                            {
                                atLeastOneMessageReceived = false;
                                // Wait for the message to be recieved
                                pn_messenger_recv(messenger, -1);
                                if ( (pnErrorNo = pn_messenger_errno(messenger) ) != 0)
                                {
                                    LogError("Error found on pn_messenger_recv: %d\r\n", pnErrorNo);
                                    break;
                                }
                                else
                                {
                                    while ((evhInfo->messageThreadExit == THREAD_CONTINUE) && (queueDepth = pn_messenger_incoming(messenger) ) > 0)
                                    {
                                        pn_message_t* pnMessage = pn_message();
                                        if (pnMessage == NULL)
                                        {
                                            evhInfo->messageThreadExit = THREAD_END;
                                        }
                                        else
                                        {
                                            if (pn_messenger_get(messenger, pnMessage) != 0)
                                            {
                                                evhInfo->messageThreadExit = THREAD_END;
                                                break;
                                            }
                                            else
                                            {
                                                pn_tracker_t tracker = pn_messenger_incoming_tracker(messenger);

                                                pn_data_t* pBody = pn_message_body(pnMessage);
                                                if (pBody != NULL)
                                                {
                                                    if (!pn_data_next(pBody))
                                                    {
                                                        evhInfo->messageThreadExit = THREAD_END;
                                                    }
                                                    else
                                                    {
                                                        pn_type_t typeOfBody = pn_data_type(pBody);
                                                        atLeastOneMessageReceived = true;
                                                        if (PN_STRING == typeOfBody)
                                                        {
                                                            pn_bytes_t descriptor = pn_data_get_string(pBody);
                                                            if (msgCallback != NULL)
                                                            {
                                                                if (msgCallback(context, descriptor.start, descriptor.size) != 0)
                                                                {
                                                                    evhInfo->messageThreadExit = THREAD_END;
                                                                }
                                                            }
                                                        }
                                                        else if (PN_BINARY == typeOfBody)
                                                        {
                                                            pn_bytes_t descriptor = pn_data_get_binary(pBody);
                                                            if (msgCallback != NULL)
                                                            {
                                                                if (msgCallback(context, descriptor.start, descriptor.size) != 0)
                                                                {
                                                                    evhInfo->messageThreadExit = THREAD_END;
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            //Unknown Data Item
                                                        }
                                                    }
                                                }
                                                pn_messenger_accept(messenger, tracker, 0);
                                                pn_message_clear(pnMessage);
                                                pn_messenger_settle(messenger, tracker, 0);
                                            }

                                            pn_message_free(pnMessage);
                                        }
                                    }

                                }
                            }
                        }
                        free(szAddress);
                    }
                }
            }

            /*bring down the messenger*/
            while (!pn_messenger_stopped(messenger))
            {
                pn_messenger_stop(messenger);
            }
            pn_messenger_free(messenger);
        }
    }
    return result;
}

void EventHub_Deinit(EVENTHUB_TEST_CLIENT_HANDLE eventhubHandle)
{
    if (eventhubHandle != NULL)
    {
        EVENTHUB_TEST_CLIENT_INFO* evhInfo = (EVENTHUB_TEST_CLIENT_INFO*)eventhubHandle;
        int notUsed = 0;

        evhInfo->messageThreadExit = THREAD_END;

        if (evhInfo->hThread != NULL)
        {
            ThreadAPI_Join(evhInfo->hThread, &notUsed);
        }

        STRING_delete(evhInfo->eventHubName);
        free(evhInfo->manageKey);
        free(evhInfo->hostName);
        free(evhInfo->svcBusName);
        free(evhInfo);
    }
}
