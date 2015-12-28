/*
Microsoft Azure IoT Client Libraries
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

#ifndef EVENTHUB_TEST_CLIENT_H
#define EVENTHUB_TEST_CLIENT_H

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

#include "macro_utils.h"
#include "buffer_.h"

typedef void* EVENTHUB_TEST_CLIENT_HANDLE;

#define EVENTHUB_TEST_CLIENT_RESULT_VALUES \
    EVENTHUB_TEST_CLIENT_OK, \
    EVENTHUB_TEST_CLIENT_INVALID_ARG, \
    EVENTHUB_TEST_CLIENT_SEND_NOT_SUCCESSFUL, \
    EVENTHUB_TEST_CLIENT_ERROR

DEFINE_ENUM(EVENTHUB_TEST_CLIENT_RESULT, EVENTHUB_TEST_CLIENT_RESULT_VALUES);

typedef int (*pfEventHubMessageCallback)(void* context, const char* data, size_t size);

extern EVENTHUB_TEST_CLIENT_HANDLE EventHub_Initialize(const char* pszconnString, const char* pszeventHubName);
extern EVENTHUB_TEST_CLIENT_RESULT EventHub_ListenForMsg(EVENTHUB_TEST_CLIENT_HANDLE eventhubHandle, pfEventHubMessageCallback msgCallback, int partitionCount, double maxExecutionTimePerPartition, void* context);
extern void EventHub_Deinit(EVENTHUB_TEST_CLIENT_HANDLE eventhubHandle);
extern EVENTHUB_TEST_CLIENT_RESULT EventHub_SendCommandMsg(EVENTHUB_TEST_CLIENT_HANDLE eventhubHandle, const char* authKey, const char* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /*EVENTHUB_TEST_CLIENT_H */
