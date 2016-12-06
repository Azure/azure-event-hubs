#EventData Requirements
 
##Overview

EventData encapsulates the data to be sent to an EventHub. It contains the body of the event, user-defined properties, and various metadata describing the event.

##References

EventData Class in .Net [http://msdn.microsoft.com/en-us/library/microsoft.servicebus.messaging.eventdata.aspx]

##Exposed API

```c
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
extern MAP_HANDLE EventData_Properties(EVENTDATA_HANDLE eventDataHandle);
extern uint64_t EventData_GetEnqueuedTimestampUTCInMs(EVENTDATA_HANDLE eventDataHandle);
extern EVENTDATA_RESULT EventData_SetEnqueuedTimestampUTCInMs(EVENTDATA_HANDLE eventDataHandle, uint64_t timestampInMs);
```

###EventData_CreateWithNewMemory

```c
extern EVENTDATA_HANDLE EventData_CreateWithNewMemory(const unsigned char* data, size_t length);
```

**SRS_EVENTDATA_03_008: \[**EventData_CreateWithNewMemory shall allocate new memory to store the specified data.**\]**
**SRS_EVENTDATA_03_002: \[**EventData_CreateWithNewMemory shall provide a non-NULL handle encapsulating the storage of the data provided.**\]**
**SRS_EVENTDATA_03_003: \[**EventData_Create shall return a NULL value if length is not zero and data is NULL.**\]** 
**SRS_EVENTDATA_03_004: \[**For all other errors, EventData_Create shall return NULL.**\]** 

###EventData_GetData

```c
extern EVENTDATA_RESULT EventData_GetData(EVENTDATA_HANDLE eventDataHandle, const unsigned char** buffer, size_t* size);
```

**SRS_EVENTDATA_03_019: \[**EventData_GetData shall provide a pointer and size for the data associated with the eventDataHandle.**\]**
**SRS_EVENTDATA_03_020: \[**The pointer shall be obtained by using BUFFER_content and it shall be copied in the buffer argument. The size of the associated data shall be obtained by using BUFFER_size and it shall be copied to the size argument.**\]**
**SRS_EVENTDATA_03_021: \[**On success, EventData_GetData shall return EVENTDATA_OK.**\]**
**SRS_EVENTDATA_03_022: \[**If any of the arguments passed to EventData_GetData is NULL, EventData_GetData shall return EVENTDATA_INVALID_ARG.**\]**
**SRS_EVENTDATA_03_023: \[**If EventData_GetData fails because of any other error it shall return EVENTDATA_ERROR.**\]**

###EventData_Clone

```c
extern EVENTDATA_HANDLE EventData_Clone(EVENTDATA_HANDLE eventDataHandle);
```

**SRS_EVENTDATA_07_050: \[**If parameter eventDataHandle is NULL then EventData_Clone shall return NULL.**\]** 
**SRS_EVENTDATA_07_051: \[**EventData_Clone shall make use of BUFFER_Clone to clone the EVENT_DATA buffer.**\]** 
**SRS_EVENTDATA_07_052: \[**EventData_Clone shall make use of STRING_Clone to clone the partitionKey if it is not set.**\]** 
**SRS_EVENTDATA_07_053: \[**EventData_Clone shall return NULL if it fails for any reason.**\]** 
**SRS_EVENTDATA_07_054: \[**EventData_Clone shall iterate the EVENTDATA VECTOR variable and clone each element.**\]** 

###EventData_Destroy

```c
extern void EventData_Destroy(EVENTDATA_HANDLE eventDataHandle);
```

**SRS_EVENTDATA_03_005: \[**EventData_Destroy shall deallocate all resources related to the eventDataHandle specified.**\]** 
**SRS_EVENTDATA_03_006: \[**EventData_Destroy shall not do anything if eventDataHandle is NULL.**\]**

###EventData_GetPartitionKey

```c
extern const char* EventData_GetPartitionKey(EVENTDATA_HANDLE eventDataHandle);
```

**SRS_EVENTDATA_07_024: \[**EventData_GetPartitionKey shall return NULL if the eventDataHandle parameter is NULL.**\]**
**SRS_EVENTDATA_07_025: \[**EventData_GetPartitionKey shall return NULL if the partitionKey in the EVENTDATA_HANDLE structure is NULL.**\]** 
**SRS_EVENTDATA_07_026: \[**On success EventData_GetPartitionKey shall return a const char* variable that is pointing to the Partition Key value that is stored in the EVENTDATA_HANDLE.**\]** 

###EventData_SetPartitionKey

```c
extern EVENTDATA_RESULT EventData_SetPartitionKey(EVENTDATA_HANDLE eventDataHandle, const char* partitionKey);
```

**SRS_EVENTDATA_07_031: \[**EventData_SetPartitionKey shall return EVENTDATA_INVALID_ARG if eventDataHandle parameter is NULL.**\]** 
**SRS_EVENTDATA_07_027: \[**If the partitionKey variable contained in the eventDataHandle parameter is not NULL then EventData_SetPartitionKey shall delete the partitionKey STRING_HANDLE.**\]** 
**SRS_EVENTDATA_07_028: \[**On success EventData_SetPartitionKey shall store the const char* partitionKey parameter in the EVENTDATA_HANDLE data structure partitionKey variable.**\]** 
**SRS_EVENTDATA_07_029: \[**if the partitionKey parameter is NULL EventData_SetPartitionKey shall not assign any value and return EVENTDATA_OK.**\]** 
**SRS_EVENTDATA_07_030: \[**On Success EventData_SetPartitionKey shall return EVENTDATA_OK.**\]** 

###EventData_Properties

```c
extern MAP_HANDLE EventData_Properties(EVENTDATA_HANDLE eventDataHandle);
```

**SRS_EVENTDATA_07_034: \[**if eventDataHandle is NULL then EventData_Properties shall return NULL.**\]** 
**SRS_EVENTDATA_07_035: \[**Otherwise, for any non-NULL eventDataHandle it shall return a non-NULL MAP_HANDLE.**\]** 

###EventData_GetEnqueuedTimestampUTCInMs

```c
extern uint64_t EventData_GetEnqueuedTimestampUTCInMs(EVENTDATA_HANDLE eventDataHandle);
```

**SRS_EVENTDATA_07_050: \[**`EventData_GetEnqueuedTimestampUTCInMs` shall return 0 if the eventDataHandle parameter is NULL.**\]**
**SRS_EVENTDATA_07_051: \[**If eventDataHandle is not null, `EventData_GetEnqueuedTimestampUTCInMs` shall return the timestamp value stored in the EVENTDATA_HANDLE.**\]**

###EventData_SetEnqueuedTimestampUTCInMs

```c
extern EVENTDATA_RESULT EventData_SetEnqueuedTimestampUTCInMs(EVENTDATA_HANDLE eventDataHandle, uint64_t timestamp)
```

**SRS_EVENTDATA_29_060: \[**`EventData_SetEnqueuedTimestampUTCInMs` shall return EVENTDATA_INVALID_ARG if eventDataHandle parameter is NULL.**\]**
**SRS_EVENTDATA_29_061: \[**On success `EventData_SetEnqueuedTimestampUTCInMs` shall store the timestamp parameter in the EVENTDATA_HANDLE data structure.**\]**
**SRS_EVENTDATA_29_062: \[**On Success `EventData_SetEnqueuedTimestampUTCInMs` shall return EVENTDATA_OK.**\]**
