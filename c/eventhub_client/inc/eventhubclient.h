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
