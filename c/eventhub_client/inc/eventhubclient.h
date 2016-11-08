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

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "eventdata.h"
#include "eventhubclient_ll.h"

typedef struct EVENTHUBCLIENT_STRUCT_TAG* EVENTHUBCLIENT_HANDLE;

MOCKABLE_FUNCTION(, const char*, EventHubClient_GetVersionString);
MOCKABLE_FUNCTION(, EVENTHUBCLIENT_HANDLE, EventHubClient_CreateFromConnectionString, const char*, connectionString, const char*, eventHubPath);
MOCKABLE_FUNCTION(, EVENTHUBCLIENT_RESULT, EventHubClient_Send, EVENTHUBCLIENT_HANDLE, eventHubHandle, EVENTDATA_HANDLE, eventDataHandle);
MOCKABLE_FUNCTION(, EVENTHUBCLIENT_RESULT, EventHubClient_SendAsync, EVENTHUBCLIENT_HANDLE, eventHubHandle, EVENTDATA_HANDLE, eventDataHandle, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK, notificationCallback, void*, userContextCallback);
MOCKABLE_FUNCTION(, EVENTHUBCLIENT_RESULT, EventHubClient_SendBatch, EVENTHUBCLIENT_HANDLE, eventHubHandle, EVENTDATA_HANDLE, *eventDataList, size_t, count);
MOCKABLE_FUNCTION(, EVENTHUBCLIENT_RESULT, EventHubClient_SendBatchAsync, EVENTHUBCLIENT_HANDLE, eventHubHandle, EVENTDATA_HANDLE, *eventDataList, size_t, count, EVENTHUB_CLIENT_SENDASYNC_CONFIRMATION_CALLBACK, sendAsycCallback, void*, userContextCallback);
MOCKABLE_FUNCTION(, EVENTHUBCLIENT_RESULT, EventHubClient_SetStateChangeCallback, EVENTHUBCLIENT_HANDLE, eventHubHandle, EVENTHUB_CLIENT_STATECHANGE_CALLBACK, state_change_cb, void*, userContextCallback);
MOCKABLE_FUNCTION(, EVENTHUBCLIENT_RESULT, EventHubClient_SetErrorCallback, EVENTHUBCLIENT_HANDLE, eventHubHandle, EVENTHUB_CLIENT_ERROR_CALLBACK, on_error_cb, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, EventHubClient_SetMessageTimeout, EVENTHUBCLIENT_HANDLE, eventHubHandle, size_t, timeout_value);
MOCKABLE_FUNCTION(, void, EventHubClient_SetLogTrace, EVENTHUBCLIENT_HANDLE, eventHubHandle, bool, log_trace_on);
MOCKABLE_FUNCTION(, void, EventHubClient_Destroy, EVENTHUBCLIENT_HANDLE, eventHubHandle);

#ifdef __cplusplus
}
#endif

#endif /* EVENTHUBCLIENT_H */
