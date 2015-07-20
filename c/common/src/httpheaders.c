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

#include "httpheaders.h"
#include "string.h"
#include "crt_abstractions.h"
#include "stddef.h"
#include "ctype.h"
#include "iot_logging.h"

const char COLON_AND_SPACE[]={':', ' ', '\0'}; 
#define COLON_AND_SPACE_LENGTH  ((sizeof(COLON_AND_SPACE)/sizeof(COLON_AND_SPACE[0]))-1)

const char COMMA_AND_SPACE[] = { ',', ' ', '\0' };
#define COMMA_AND_SPACE_LENGTH  ((sizeof(COMMA_AND_SPACE)/sizeof(COMMA_AND_SPACE[0]))-1)

DEFINE_ENUM_STRINGS(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);

typedef struct HTTP_HEADER_TAG
{
    char* header;
} HTTP_HEADER;

typedef struct HTTP_HEADERS_DATA_TAG
{
    size_t HeaderCount;
    HTTP_HEADER* Headers;
} HTTP_HEADERS_DATA;

static size_t FindHeader(HTTP_HEADERS_DATA* headerData, const char* name)
{
    /* ASSERT(headerData != NULL); */
    /* ASSERT(name != NULL);       */

    size_t i;
    const size_t nameLength = strlen(name);

    for (i = 0; i < headerData->HeaderCount; i++)
    {
        if ((strncmp(headerData->Headers[i].header, name, nameLength) == 0) &&
            (headerData->Headers[i].header[nameLength] == ':'))
        {
            break;
        }
    }

    return i;
}

HTTP_HEADERS_HANDLE HTTPHeaders_Alloc(void)
{
    /*Codes_SRS_HTTP_HEADERS_99_002:[ This API shall produce a HTTP_HANDLE that can later be used in subsequent calls to the module.]*/
    HTTP_HEADERS_DATA* result;
    result = (HTTP_HEADERS_DATA*)malloc(sizeof(HTTP_HEADERS_DATA));

    if (result != NULL)
    {
        result->HeaderCount = 0;
        result->Headers = NULL;
    }

    /*Codes_SRS_HTTP_HEADERS_99_003:[ The function shall return NULL when the function cannot execute properly]*/
    return (HTTP_HEADERS_HANDLE)result;
}

/*Codes_SRS_HTTP_HEADERS_99_005:[ Calling this API shall de-allocate the data structures allocated by previous API calls to the same handle.]*/
void HTTPHeaders_Free(HTTP_HEADERS_HANDLE httpHeadersHandle)
{
    HTTP_HEADERS_DATA* headerData = (HTTP_HEADERS_DATA*)httpHeadersHandle;
    if (headerData != NULL)
    {
        if (headerData->Headers != NULL)
        {
            size_t i;
            for (i = 0; i < headerData->HeaderCount; i++)
            {
                HTTP_HEADER* headerPtr = headerData->Headers + i;
                if (headerPtr->header != NULL)
                {
                    free(headerData->Headers[i].header);
                }
            }

            free(headerData->Headers);
        }

        free(httpHeadersHandle);
    }
}

/*Codes_SRS_HTTP_HEADERS_99_012:[ Calling this API shall record a header from name and value parameters.]*/
static HTTP_HEADERS_RESULT headers_ReplaceHeaderNameValuePair(HTTP_HEADERS_HANDLE httpHeadersHandle, const char* name, const char* value, bool replace)
{
    HTTP_HEADERS_RESULT result = HTTP_HEADERS_OK;
    HTTP_HEADERS_DATA* headerData = (HTTP_HEADERS_DATA*)httpHeadersHandle;

    if ((headerData == NULL) ||
        (name == NULL) ||
        (value == NULL))
    {
        result = HTTP_HEADERS_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
    }
    else
    {
        /*Codes_SRS_HTTP_HEADERS_99_036:[ If name contains the characters outside character codes 33 to 126 then the return value shall be HTTP_HEADERS_INVALID_ARG]*/
        size_t i;
        size_t nameLen = strlen(name);
        for (i = 0; i<nameLen; i++)
        {
            if ((name[i]<33) || (126<name[i]) || (name[i] == ':'))
            {
                result = HTTP_HEADERS_INVALID_ARG;
                LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
                break;
            }
        }
    }

    if (result == HTTP_HEADERS_OK)
    {
        const size_t foundIndex = FindHeader(headerData, name);
        if (foundIndex != headerData->HeaderCount)
        {
            /*Codes_SRS_HTTP_HEADERS_99_017:[ If the name already exists in the collection of headers, the function shall concatenate the new value after the existing value, separated by a comma and a space as in: old-value+", "+new-value.]*/
            size_t headerSize;
            char* newHeader;
            if (replace)
            {
                headerSize = strlen(name) + COLON_AND_SPACE_LENGTH + strlen(value) + 1; /*1 is for '\0'*/
            }
            else
            {
                headerSize = strlen(headerData->Headers[foundIndex].header) + COMMA_AND_SPACE_LENGTH + strlen(value) + 1; /*1 is for '\0'*/
            }
            newHeader = (char*)realloc(headerData->Headers[foundIndex].header, headerSize);
            if (newHeader == NULL)
            {
                result = HTTP_HEADERS_ALLOC_FAILED;
                LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
            }
            else
            {
                headerData->Headers[foundIndex].header = newHeader;
                if (!replace)
                {
                    if ((strcat_s(newHeader, headerSize, COMMA_AND_SPACE) != 0) || strcat_s(newHeader, headerSize, value) != 0)
                    {
                        /*Codes_SRS_HTTP_HEADERS_99_037:[ The function shall return HTTP_HEADERS_ERROR when an internal error occurs.]*/
                        result = HTTP_HEADERS_ERROR;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
                    }
                    else
                    {
                        /*Codes_SRS_HTTP_HEADERS_99_013:[ The function shall return HTTP_HEADERS_OK when execution is successful.]*/
                        result = HTTP_HEADERS_OK;
                    }
                }
                else
                {
                    /*Codes_SRS_HTTP_HEADERS_99_016:[ The function shall store the name:value pair in such a way that when later retrieved by a call to GetHeader it will return a string that shall strcmp equal to the name+": "+value.]*/
                    int sResult = 0;
                    sResult = sResult || strcpy_s(headerData->Headers[foundIndex].header, headerSize, name);
                    sResult = sResult || strcat_s(headerData->Headers[foundIndex].header, headerSize, COLON_AND_SPACE);
                    sResult = sResult || strcat_s(headerData->Headers[foundIndex].header, headerSize, value);
                    /*Codes_SRS_HTTP_HEADERS_99_037:[ The function shall return HTTP_HEADERS_ERROR when an internal error occurs.]*/
                    if (sResult != 0)
                    {
                        result = HTTP_HEADERS_ERROR;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
                    }
                    else
                    {
                        /*Codes_SRS_HTTP_HEADERS_99_013:[ The function shall return HTTP_HEADERS_OK when execution is successful.]*/
                        result = HTTP_HEADERS_OK;
                    }
                }
            }
        }
        else
        {
            HTTP_HEADER* newHeaders = (HTTP_HEADER*)realloc(headerData->Headers, sizeof(HTTP_HEADER) * (headerData->HeaderCount + 1));
            if (newHeaders == NULL)
            {
                result = HTTP_HEADERS_ALLOC_FAILED;
                LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
            }
            else
            {
                size_t headerLength = strlen(name) + COLON_AND_SPACE_LENGTH + strlen(value) + 1; /*1 is for '\0'*/
                headerData->Headers = newHeaders;

                headerData->Headers[headerData->HeaderCount].header = (char*)malloc(headerLength);
                if (headerData->Headers[headerData->HeaderCount].header == NULL)
                {
                    result = HTTP_HEADERS_ALLOC_FAILED;
                    LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
                }
                else
                {
                    /*Codes_SRS_HTTP_HEADERS_99_016:[ The function shall store the name:value pair in such a way that when later retrieved by a call to GetHeader it will return a string that shall strcmp equal to the name+": "+value.]*/
                    int sResult = 0;
                    sResult = sResult || strcpy_s(headerData->Headers[headerData->HeaderCount].header, headerLength, name);
                    sResult = sResult || strcat_s(headerData->Headers[headerData->HeaderCount].header, headerLength, COLON_AND_SPACE);
                    sResult = sResult || strcat_s(headerData->Headers[headerData->HeaderCount].header, headerLength, value);
                    /*Codes_SRS_HTTP_HEADERS_99_037:[ The function shall return HTTP_HEADERS_ERROR when an internal error occurs.]*/
                    if (sResult != 0)
                    {
                        result = HTTP_HEADERS_ERROR;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
                    }
                    else
                    {
                        headerData->HeaderCount++;
                        /*Codes_SRS_HTTP_HEADERS_99_013:[ The function shall return HTTP_HEADERS_OK when execution is successful.]*/
                        result = HTTP_HEADERS_OK;
                    }
                }
            }
        }
    }

    return result;
}

HTTP_HEADERS_RESULT HTTPHeaders_AddHeaderNameValuePair(HTTP_HEADERS_HANDLE httpHeadersHandle, const char* name, const char* value)
{
    return headers_ReplaceHeaderNameValuePair(httpHeadersHandle, name, value, false);
}

/* Codes_SRS_HTTP_HEADERS_06_001: [This API will perform exactly as HTTPHeaders_AddHeaderNameValuePair except that if the header name already exists the already existing value will be replaced as opposed to concatenated to.] */
HTTP_HEADERS_RESULT HTTPHeaders_ReplaceHeaderNameValuePair(HTTP_HEADERS_HANDLE httpHeadersHandle, const char* name, const char* value)
{
    return headers_ReplaceHeaderNameValuePair(httpHeadersHandle, name, value, true);
}


const char* HTTPHeaders_FindHeaderValue(HTTP_HEADERS_HANDLE httpHeadersHandle, const char* name)
{
    const char* result = NULL;
    HTTP_HEADERS_DATA* headerData = (HTTP_HEADERS_DATA*)httpHeadersHandle;

    if ((headerData != NULL) &&
        (name != NULL))
    {
        size_t i = FindHeader(headerData, name);
        if (i != headerData->HeaderCount)
        {
            char* valuePos = headerData->Headers[i].header + strlen(name);
            while ((*valuePos != '\0') && isspace(*valuePos))
            {
                valuePos++;
            }

            if (*valuePos == ':')
            {
                do
                {
                    valuePos++;
                } while ((*valuePos != '\0') && isspace(*valuePos));

                /*Codes_SRS_HTTP_HEADERS_99_021:[ In this case the return value shall point to a string that shall strcmp equal to the original stored string.]*/
                result = valuePos;
            }
        }
    }

    return result;
}

HTTP_HEADERS_RESULT HTTPHeaders_GetHeaderCount(HTTP_HEADERS_HANDLE httpHeadersHandle, size_t* headerCount)
{
    HTTP_HEADERS_DATA* headerData = (HTTP_HEADERS_DATA*)httpHeadersHandle;
    HTTP_HEADERS_RESULT result;

    /*Codes_SRS_HTTP_HEADERS_99_024:[ The function shall return HTTP_HEADERS_INVALID_ARG when an invalid handle is passed.]*/
    /*Codes_SRS_HTTP_HEADERS_99_025:[ The function shall return HTTP_HEADERS_INVALID_ARG when headersCount is NULL.]*/
    if ((headerData == NULL) ||
        (headerCount == NULL))
    {
        result = HTTP_HEADERS_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
    }
    else
    {
        /*Codes_SRS_HTTP_HEADERS_99_023:[ Calling this API shall provide the number of stored headers.]*/
        /*Codes_SRS_HTTP_HEADERS_99_026:[ The function shall write in *headersCount the number of currently stored headers and shall return HTTP_HEADERS_OK]*/
        *headerCount = headerData->HeaderCount;
        result = HTTP_HEADERS_OK;
    }

    return result;
}

extern HTTP_HEADERS_RESULT HTTPHeaders_GetHeader(HTTP_HEADERS_HANDLE httpHeadersHandle, size_t index, char* buffer, size_t bufferSize)
{
    HTTP_HEADERS_RESULT result = HTTP_HEADERS_OK;

    HTTP_HEADERS_DATA* headerData = (HTTP_HEADERS_DATA*)httpHeadersHandle;

    /*Codes_SRS_HTTP_HEADERS_99_028:[ The function shall return NULL if the handle is invalid.]*/
    if (headerData == NULL)
    {
        result = HTTP_HEADERS_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
    }
    /*Codes_SRS_HTTP_HEADERS_99_029:[ The function shall return HTTP_HEADERS_INVALID_ARG if index is not valid (for example, out of range) for the currently stored headers.]*/
    else if (index >= headerData->HeaderCount)
    {
        result = HTTP_HEADERS_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
    }
    /*Codes_SRS_HTTP_HEADERS_99_032:[ The function shall return HTTP_HEADERS_INVALID_ARG if the buffer is NULL]*/
    else if(buffer==NULL)
    {
        result = HTTP_HEADERS_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
    }
    /*Codes_SRS_HTTP_HEADERS_99_033:[ The function shall return HTTP_HEADERS_INSUFFICIENT_BUFFER if bufferSize is not big enough to accommodate the resulting string]*/
    else if(bufferSize < strlen(headerData->Headers[index].header)+1)
    {
        result = HTTP_HEADERS_INSUFFICIENT_BUFFER;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
    }
    /*Codes_SRS_HTTP_HEADERS_99_027:[ Calling this API shall produce the string value+": "+pair) for the index header in the buffer pointed to by buffer.]*/
    else if(strcpy_s(buffer, bufferSize, headerData->Headers[index].header)!=0)
    {
        /*Codes_SRS_HTTP_HEADERS_99_034:[ The function shall return HTTP_HEADERS_ERROR when an internal error occurs]*/
        result = HTTP_HEADERS_ERROR;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTP_HEADERS_RESULT, result));
    }
    else
    {
        /*Codes_SRS_HTTP_HEADERS_99_035:[ The function shall return HTTP_HEADERS_OK when the function executed without error.]*/
        result = HTTP_HEADERS_OK;
    }

    return result;
}
