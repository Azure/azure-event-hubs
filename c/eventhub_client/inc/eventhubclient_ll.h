// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef EVENTHUBCLIENT_LL_H
#define EVENTHUBCLIENT_LL_H

#include "azure_c_shared_utility/macro_utils.h"
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

typedef struct EVENTHUBCLIENT_LL_TAG* EVENTHUBCLIENT_LL_HANDLE;
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
