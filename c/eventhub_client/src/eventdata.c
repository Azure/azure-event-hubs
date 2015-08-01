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

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "gballoc.h"

#include "eventdata.h"
#include "iot_logging.h"
#include "buffer_.h"
#include "strings.h"
#include "vector.h"

DEFINE_ENUM_STRINGS(EVENTDATA_RESULT, EVENTDATA_ENUM_VALUES)

typedef struct EVENT_DATA_TAG
{
    BUFFER_HANDLE buffer;
    STRING_HANDLE partitionKey;
    MAP_HANDLE properties;
} EVENT_DATA;

typedef struct EVENT_PROPERTY_TAG
{
    STRING_HANDLE key;
    STRING_HANDLE value;
} EVENT_PROPERTY;

static bool ContainsOnlyUsAscii(const char* asciiValue)
{
    bool result = true;;
    const char* iterator = asciiValue;
    while (iterator != NULL && *iterator != '\0')
    {
        // Allow only printable ascii char 
        if (*iterator < ' ' || *iterator > '~')
        {
            result = false;
            break;
        }
        iterator++;
    }
    return result;
}

static int ValidateAsciiCharactersFilter(const char* mapKey, const char* mapValue)
{
    int result;
    if (!ContainsOnlyUsAscii(mapKey) || !ContainsOnlyUsAscii(mapValue) )
    {
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

EVENTDATA_HANDLE EventData_CreateWithNewMemory(const unsigned char* data, size_t length)
{
    EVENTDATA_HANDLE result;
    EVENT_DATA* eventData;
    /* Codes_SRS_EVENTDATA_03_003: [EventData_Create shall return a NULL value if length is not zero and data is NULL.] */
    if (length != 0 && data == NULL)
    {
        result = NULL;
        LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_INVALID_ARG));
    }
    else if ((eventData = (EVENT_DATA*)malloc(sizeof(EVENT_DATA))) == NULL)
    {
        /* Codes_SRS_EVENTDATA_03_004: [For all other errors, EventData_Create shall return NULL.] */
        result = NULL;
        LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_ERROR));
    }
    /* SRS_EVENTDATA_03_008: [EventData_CreateWithNewMemory shall allocate new memory to store the specified data.] */
    else if ((eventData->buffer = BUFFER_new()) == NULL)
    {
        free(eventData);
        /* Codes_SRS_EVENTDATA_03_004: [For all other errors, EventData_Create shall return NULL.] */
        result = NULL;
        LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_ERROR));
    }
    else if (length != 0)
    {
        /* Codes_SRS_EVENTDATA_03_002: [EventData_CreateWithNewMemory shall provide a none-NULL handle encapsulating the storage of the data provided.] */
        if (BUFFER_build(eventData->buffer, data, length) != 0)
        {
            BUFFER_delete(eventData->buffer);
            free(eventData);
            /* Codes_SRS_EVENTDATA_03_004: [For all other errors, EventData_Create shall return NULL.] */
            result = NULL;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_ERROR));
        }
        else
        {
            eventData->partitionKey = NULL;
            if ( (eventData->properties = Map_Create(ValidateAsciiCharactersFilter) ) == NULL)
            {
                BUFFER_delete(eventData->buffer);
                free(eventData);
                /* Codes_SRS_EVENTDATA_03_004: [For all other errors, EventData_Create shall return NULL.] */
                result = NULL;
                LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_ERROR));
            }
            else
            {
                result = (EVENTDATA_HANDLE)eventData;
            }
        }
    }
    else
    {
        eventData->partitionKey = NULL;
        if ( (eventData->properties = Map_Create(NULL) ) == NULL)
        {
            BUFFER_delete(eventData->buffer);
            free(eventData);
            /* Codes_SRS_EVENTDATA_03_004: [For all other errors, EventData_Create shall return NULL.] */
            result = NULL;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_ERROR));
        }
        else
        {
            result = (EVENTDATA_HANDLE)eventData;
        }
    }
    return result;
}

void EventData_Destroy(EVENTDATA_HANDLE eventDataHandle)
{
    EVENT_DATA* eventData;
    /* Codes_SRS_EVENTDATA_03_006: [EventData_Destroy shall not do anything if eventDataHandle is NULL.] */
    if (eventDataHandle != NULL)
    {
        /* Codes_SRS_EVENTDATA_03_005: [EventData_Destroy shall deallocate all resources related to the eventDataHandle specified.] */
        eventData = (EVENT_DATA*)eventDataHandle;
        BUFFER_delete(eventData->buffer);
        STRING_delete(eventData->partitionKey);
        eventData->partitionKey = NULL;
        Map_Destroy(eventData->properties);

        free(eventData);
    }
}

/* Codes_SRS_EVENTDATA_03_019: [EventData_GetData shall provide a pointer and size for the data associated with the eventDataHandle.] */
EVENTDATA_RESULT EventData_GetData(EVENTDATA_HANDLE eventDataHandle, const unsigned char** buffer, size_t* size)
{
    EVENTDATA_RESULT result;

    /* Codes_SRS_EVENTDATA_03_022: [If any of the arguments passed to EventData_GetData is NULL, EventData_GetData shall return EVENTDATA_INVALID_ARG.] */
    if (eventDataHandle == NULL || buffer == NULL || size == NULL)
    {
        result = EVENTDATA_INVALID_ARG;
        LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, result));
    }
    else
    {
        /* Codes_SRS_EVENTDATA_03_020: [The pointer shall be obtained by using BUFFER_content and it shall be copied in the buffer argument. The size of the associated data shall be obtained by using BUFFER_size and it shall be copied to the size argument.] */
        if (BUFFER_content(((EVENT_DATA*)eventDataHandle)->buffer, buffer) != 0)
        {
            /* Codes_SRS_EVENTDATA_03_023: [If EventData_GetData fails because of any other error it shall return EVENTDATA_ERROR.]*/
            result = EVENTDATA_ERROR;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, result));
        }
        else if (BUFFER_size(((EVENT_DATA*)eventDataHandle)->buffer, size) != 0)
        {
            /* Codes_SRS_EVENTDATA_03_023: [If EventData_GetData fails because of any other error it shall return EVENTDATA_ERROR.]*/
            result = EVENTDATA_ERROR;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, result));
        }
        else
        {
            /* Codes_SRS_EVENTDATA_03_021: [On success, EventData_GetData shall return EVENTDATA_OK.] */
            result = EVENTDATA_OK;
        }
    }
    return result;
}

const char* EventData_GetPartitionKey(EVENTDATA_HANDLE eventDataHandle)
{
    const char* result;
    /* Codes_SRS_EVENTDATA_07_024: [EventData_GetPartitionKey shall return NULL if the eventDataHandle parameter is NULL.] */
    if (eventDataHandle == NULL)
    {
        result = NULL;
        LogError("EventData_GetPartitionKey result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_INVALID_ARG));
    }
    else
    {
        /* Codes_SRS_EVENTDATA_07_025: [EventData_GetPartitionKey shall return NULL if the partitionKey in the EVENTDATA_HANDLE is NULL.] */
        EVENT_DATA* eventData = (EVENT_DATA*)eventDataHandle;
        if (eventData->partitionKey == NULL)
        {
            result = NULL;
        }
        else
        {
            /* Codes_SRS_EVENTDATA_07_026: [On success EventData_GetPartitionKey shall return a const char* variable that is pointing to the Partition Key value that is stored in the EVENTDATA_HANDLE.] */
            result = STRING_c_str(eventData->partitionKey);
        }
    }
    return result;
}

EVENTDATA_RESULT EventData_SetPartitionKey(EVENTDATA_HANDLE eventDataHandle, const char* partitionKey)
{
    EVENTDATA_RESULT result;
    /* Codes_SRS_EVENTDATA_07_031: [EventData_SetPartitionKey shall return a nonzero value if eventDataHandle or partitionKey is NULL.] */
    if (eventDataHandle == NULL)
    {
        result = EVENTDATA_INVALID_ARG;
        LogError("EventData_SetPartitionKey result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, result));
    }
    else
    {
        /* Codes_SRS_EVENTDATA_07_027: [If the partitionKey variable contained in the eventDataHandle parameter is not NULL then EventData_SetPartitionKey shall delete the partitionKey STRING_HANDLE.] */
        EVENT_DATA* eventData = (EVENT_DATA*)eventDataHandle;
        if (eventData->partitionKey != NULL)
        {
            // Delete the memory if there is any
            STRING_delete(eventData->partitionKey);
        }
        if (partitionKey != NULL)
        {
            /* Codes_SRS_EVENTDATA_07_028: [On success EventData_SetPartitionKey shall store the const char* partitionKey parameter in the EVENTDATA_HANDLE data structure partitionKey variable.] */
            eventData->partitionKey = STRING_construct(partitionKey);
        }
        /* Codes_SRS_EVENTDATA_07_030: [On Success EventData_SetPartitionKey shall return EVENTDATA_OK.] */
        result = EVENTDATA_OK;
    }
    return result;
}

EVENTDATA_HANDLE EventData_Clone(EVENTDATA_HANDLE eventDataHandle)
{
    EVENT_DATA* result;
    if (eventDataHandle == NULL)
    {
        /* Codes_SRS_EVENTDATA_07_050: [EventData_Clone shall return NULL when the eventDataHandle is NULL.] */
        result = NULL;
        LogError("EventData_Clone result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_INVALID_ARG));
    }
    else
    {
        EVENT_DATA* srcData = (EVENT_DATA*)eventDataHandle;

        if ( (result = (EVENT_DATA*)malloc(sizeof(EVENT_DATA) ) ) == NULL)
        {
            /* Codes_SRS_EVENTDATA_07_053: [EventData_Clone shall return NULL if it fails for any reason.] */
            result = NULL;
            LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_ERROR));
        }
        else
        {
            // Need to initialize this here in case it doesn't get set later
            result->partitionKey = NULL;

            /* Codes_SRS_EVENTDATA_07_051: [EventData_Clone shall make use of BUFFER_Clone to clone the EVENT_DATA buffer.] */
            if ( (result->buffer = BUFFER_clone(srcData->buffer) ) == NULL)
            {
                /* Codes_SRS_EVENTDATA_07_053: [EventData_Clone shall return NULL if it fails for any reason.] */
                free(result);
                result = NULL;
                LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_ERROR));
            }
            /* Codes_SRS_EVENTDATA_07_052: [EventData_Clone shall make use of STRING_Clone to clone the partitionKey if it is not set.] */
            else if ( (srcData->partitionKey != NULL) && (result->partitionKey = STRING_clone(srcData->partitionKey) ) == NULL)
            {
                /* Codes_SRS_EVENTDATA_07_053: [EventData_Clone shall return NULL if it fails for any reason.] */
                BUFFER_delete(result->buffer);
                free(result);
                result = NULL;
                LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_ERROR));
            }
            else if ( (result->properties = Map_Clone(srcData->properties)) == NULL)
            {
                /* Codes_SRS_EVENTDATA_07_053: [EventData_Clone shall return NULL if it fails for any reason.] */
                STRING_delete(result->partitionKey);
                BUFFER_delete(result->buffer);
                free(result);
                result = NULL;
                LogError("result = %s\r\n", ENUM_TO_STRING(EVENTDATA_RESULT, EVENTDATA_ERROR));
            }
        }
    }
    return (EVENTDATA_HANDLE)result; 
}

MAP_HANDLE EventData_Properties(EVENTDATA_HANDLE eventDataHandle)
{
    MAP_HANDLE result;
    /* SRS_EVENTDATA_07_034: [if eventDataHandle is NULL then EventData_Properties shall return NULL.] */
    if (eventDataHandle == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_Properties\r\n")
        result = NULL;
    }
    else
    {
        /* SRS_EVENTDATA_07_035: [Otherwise, for any non-NULL eventDataHandle it shall return a non-NULL MAP_HANDLE.] */
        EVENT_DATA* handleData = (EVENT_DATA*)eventDataHandle;
        result = handleData->properties;
    }
    return result;
}
