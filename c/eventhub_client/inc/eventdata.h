// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef EVENTDATA_H
#define EVENTDATA_H

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/map.h" 

#ifdef __cplusplus
#include <cstddef>
extern "C" 
{
#else
#include <stdint.h>
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

MOCKABLE_FUNCTION(, EVENTDATA_HANDLE, EventData_CreateWithNewMemory, const unsigned char*, data, size_t, length);
MOCKABLE_FUNCTION(, EVENTDATA_RESULT, EventData_GetData, EVENTDATA_HANDLE, eventDataHandle, const unsigned char**, buffer, size_t*, size);
MOCKABLE_FUNCTION(, EVENTDATA_HANDLE, EventData_Clone, EVENTDATA_HANDLE, eventDataHandle);
MOCKABLE_FUNCTION(, void, EventData_Destroy, EVENTDATA_HANDLE, eventDataHandle);
MOCKABLE_FUNCTION(, const char*, EventData_GetPartitionKey, EVENTDATA_HANDLE, eventDataHandle);
MOCKABLE_FUNCTION(, EVENTDATA_RESULT, EventData_SetPartitionKey, EVENTDATA_HANDLE, eventDataHandle, const char*, partitionKey);
MOCKABLE_FUNCTION(, MAP_HANDLE, EventData_Properties, EVENTDATA_HANDLE, eventDataHandle);
MOCKABLE_FUNCTION(, EVENTDATA_RESULT, EventData_SetEnqueuedTimestampUTCInMs, EVENTDATA_HANDLE, eventDataHandle, uint64_t, timestampInMs);
MOCKABLE_FUNCTION(, uint64_t, EventData_GetEnqueuedTimestampUTCInMs, EVENTDATA_HANDLE, eventDataHandle);
#ifdef __cplusplus
}
#endif

#endif /* EVENTDATA_H */
