// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef EVENTDATA_H
#define EVENTDATA_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/map.h" 

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

typedef struct EVENT_DATA* EVENTDATA_HANDLE;

extern EVENTDATA_HANDLE EventData_CreateWithNewMemory(const unsigned char* data, size_t length);
extern EVENTDATA_RESULT EventData_GetData(EVENTDATA_HANDLE eventDataHandle, const unsigned char** buffer, size_t* size);
extern EVENTDATA_HANDLE EventData_Clone(EVENTDATA_HANDLE eventDataHandle);

extern void EventData_Destroy(EVENTDATA_HANDLE eventDataHandle);

extern const char* EventData_GetPartitionKey(EVENTDATA_HANDLE eventDataHandle);
extern EVENTDATA_RESULT EventData_SetPartitionKey(EVENTDATA_HANDLE eventDataHandle, const char* partitionKey);

extern MAP_HANDLE EventData_Properties(EVENTDATA_HANDLE eventDataHandle);

#ifdef __cplusplus
}
#endif

#endif /* EVENTDATA_H */
