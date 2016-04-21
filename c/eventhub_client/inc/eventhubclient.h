// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef EVENTHUBCLIENT_H
#define EVENTHUBCLIENT_H

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

#include "eventhubclient_ll.h"
#include "macro_utils.h"
#include "eventdata.h"

typedef void* EVENTHUBCLIENT_HANDLE;

extern const char* EventHubClient_GetVersionString(void);
extern EVENTHUBCLIENT_HANDLE EventHubClient_CreateFromConnectionString(const char* connectionString, const char* eventHubPath);
extern EVENTHUBCLIENT_RESULT EventHubClient_Send(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE eventDataHandle);
extern EVENTHUBCLIENT_RESULT EventHubClient_SendAsync(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK notificationCallback, void* userContextCallback);
extern EVENTHUBCLIENT_RESULT EventHubClient_SendBatch(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE *eventDataList, size_t count);
extern EVENTHUBCLIENT_RESULT EventHubClient_SendBatchAsync(EVENTHUBCLIENT_HANDLE eventHubHandle, EVENTDATA_HANDLE *eventDataList, size_t count, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK sendAsycCallback, void* userContextCallback);
extern void EventHubClient_Destroy(EVENTHUBCLIENT_HANDLE eventHubHandle);

#ifdef __cplusplus
}
#endif

#endif /* EVENTHUBCLIENT_H */
