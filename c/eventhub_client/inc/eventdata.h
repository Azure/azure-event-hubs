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

#ifndef EVENTDATA_H
#define EVENTDATA_H

#include "macro_utils.h"

#ifdef __cplusplus
#include <cstddef>
extern "C" 
{
#else
#include <stddef.h>
#endif

#define EVENTDATA_RESULT_VALUES        \
    EVENTDATA_OK,                      \
    EVENTDATA_INVALID_ARG,             \
    EVENTDATA_MISSING_PROPERTY_NAME,   \
    EVENTDATA_INDEX_OUT_OF_BOUNDS,     \
    EVENTDATA_ERROR                    \

DEFINE_ENUM(EVENTDATA_RESULT, EVENTDATA_RESULT_VALUES);

typedef void* EVENTDATA_HANDLE;

extern EVENTDATA_HANDLE EventData_CreateWithNewMemory(const unsigned char* data, size_t length);
extern EVENTDATA_RESULT EventData_GetData(EVENTDATA_HANDLE eventDataHandle, const unsigned char** buffer, size_t* size);
extern EVENTDATA_HANDLE EventData_Clone(EVENTDATA_HANDLE eventDataHandle);

extern void EventData_Destroy(EVENTDATA_HANDLE eventDataHandle);

extern const char* EventData_GetPartitionKey(EVENTDATA_HANDLE eventDataHandle);
extern EVENTDATA_RESULT EventData_SetPartitionKey(EVENTDATA_HANDLE eventDataHandle, const char* partitionKey);

extern EVENTDATA_RESULT EventData_GetPropertyByName(EVENTDATA_HANDLE eventDataHandle, const char* propertyName, const char** propertyValue);
extern EVENTDATA_RESULT EventData_GetPropertyByIndex(EVENTDATA_HANDLE eventDataHandle, size_t propertyIndex, const char** propertyName, const char** propertyValue);
extern EVENTDATA_RESULT EventData_SetProperty(EVENTDATA_HANDLE eventDataHandle, const char* propertyName, const char* propertyValue);
extern size_t EventData_GetPropertyCount(EVENTDATA_HANDLE eventDataHandle);

#ifdef __cplusplus
}
#endif

#endif /* EVENTDATA_H */
