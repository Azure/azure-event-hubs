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

#ifndef EVENTHUBCLIENT_LL_H
#define EVENTHUBCLIENT_LL_H

#include "macro_utils.h"
#include "eventdata.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

#define EVENTHUBCLIENT_RESULT_VALUES            \
    EVENTHUBCLIENT_OK,                          \
    EVENTHUBCLIENT_INVALID_ARG,                 \
    EVENTHUBCLIENT_INVALID_CONNECTION_STRING,   \
    EVENTHUBCLIENT_URL_ENCODING_FAILURE,        \
    EVENTHUBCLIENT_EVENT_DATA_FAILURE,          \
    EVENTHUBCLIENT_PARTITION_KEY_MISMATCH,      \
    EVENTHUBCLIENT_DATA_SIZE_EXCEEDED,          \
    EVENTHUBCLIENT_ERROR                        \

DEFINE_ENUM(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_RESULT_VALUES);

#define EVENTHUBCLIENT_CONFIRMATION_RESULT_VALUES     \
    EVENTHUBCLIENT_CONFIRMATION_OK,                   \
    EVENTHUBCLIENT_CONFIRMATION_DESTROY,              \
    EVENTHUBCLIENT_CONFIRMATION_EXCEED_MAX_SIZE,      \
    EVENTHUBCLIENT_CONFIRMATION_UNKNOWN,              \
    EVENTHUBCLIENT_CONFIRMATION_ERROR                 \

DEFINE_ENUM(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_RESULT_VALUES);

typedef void* EVENTHUBCLIENT_LL_HANDLE;
typedef void(*EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK)(EVENTHUBCLIENT_CONFIRMATION_RESULT result, void* userContextCallback);

extern EVENTHUBCLIENT_LL_HANDLE EventHubClient_LL_CreateFromConnectionString(const char* connectionString, const char* eventHubPath);

extern EVENTHUBCLIENT_RESULT EventHubClient_LL_SendAsync(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle, EVENTDATA_HANDLE eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncConfirmationCallback, void* userContextCallback);
extern EVENTHUBCLIENT_RESULT EventHubClient_LL_SendBatchAsync(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle, EVENTDATA_HANDLE* eventDataList, size_t count, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsyncConfirmationCallback, void* userContextCallback);

extern void EventHubClient_LL_DoWork(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle);

extern void EventHubClient_LL_Destroy(EVENTHUBCLIENT_LL_HANDLE eventHubClientLLHandle);

#ifdef __cplusplus
}
#endif

#endif /* EVENTHUBCLIENT_LL_H */
