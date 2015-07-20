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

#include "stdio.h"
#include "malloc.h"
#include "string.h"
#include "ctype.h"
#include "httpapi.h"
#include "httpheaders.h"
#include "crt_abstractions.h"
#include "curl/curl.h"
#include "iot_logging.h"
#include "stddef.h"
#include "strings.h"

#define TEMP_BUFFER_SIZE 1024

DEFINE_ENUM_STRINGS(HTTPAPI_RESULT, HTTPAPI_RESULT_VALUES);

typedef struct HTTP_HANDLE_DATA_TAG
{
    CURL* curl;
    char hostURL[512];
} HTTP_HANDLE_DATA;

typedef struct HTTP_RESPONSE_CONTENT_BUFFER_TAG
{
    unsigned char* buffer;
    size_t bufferSize;
    unsigned char error;
} HTTP_RESPONSE_CONTENT_BUFFER;

static size_t nUsersOfHTTPAPI = 0; /*used for reference counting (a weak one)*/

HTTPAPI_RESULT HTTPAPI_Init(void)
{
    HTTPAPI_RESULT result;
    if (nUsersOfHTTPAPI == 0)
    {
        LogInfo("HTTP_API first init.\r\n");
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
        {
            result = HTTPAPI_INIT_FAILED;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        }
        else
        {
            nUsersOfHTTPAPI++;
            result = HTTPAPI_OK;
            LogInfo("HTTP_API has now %d users\r\n", (int)nUsersOfHTTPAPI);
        }
    }
    else
    {
        nUsersOfHTTPAPI++;
        result = HTTPAPI_OK;
        LogInfo("HTTP_API has now %d users\r\n", (int)nUsersOfHTTPAPI);
    }

    return result;
}

void HTTPAPI_Deinit(void)
{
    if (nUsersOfHTTPAPI > 0)
    {
        nUsersOfHTTPAPI--;
        LogInfo("HTTP_API has now %d users\r\n", (int)nUsersOfHTTPAPI);
        if (nUsersOfHTTPAPI == 0)
        {
            LogInfo("Deinitializing HTTP_API\r\n");
            curl_global_cleanup();
        }
    }
}

HTTP_HANDLE HTTPAPI_CreateConnection(const char* hostName)
{
    HTTP_HANDLE_DATA* httpHandleData = NULL;

    if (hostName != NULL)
    {
        httpHandleData = (HTTP_HANDLE_DATA*)malloc(sizeof(HTTP_HANDLE_DATA));
        if (httpHandleData != NULL)
        {
            if ((strcpy_s(httpHandleData->hostURL, sizeof(httpHandleData->hostURL), "https://") == 0) &&
                (strcat_s(httpHandleData->hostURL, sizeof(httpHandleData->hostURL), hostName) == 0))
            {
                httpHandleData->curl = curl_easy_init();
                if (httpHandleData->curl == NULL)
                {
                    free(httpHandleData);
                    httpHandleData = NULL;
                }
                else
                {
                    /*uncomment the below line to get into curl debug mode*/
                    //curl_easy_setopt(httpHandleData->curl, CURLOPT_VERBOSE, 1L);
                }
            }
        }
    }

    return (HTTP_HANDLE)httpHandleData;
}

void HTTPAPI_CloseConnection(HTTP_HANDLE handle)
{
    HTTP_HANDLE_DATA* httpHandleData = (HTTP_HANDLE_DATA*)handle;
    if (httpHandleData != NULL)
    {
        curl_easy_cleanup(httpHandleData->curl);
        free(httpHandleData);
    }
}

static size_t HeadersWriteFunction(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    HTTP_HEADERS_HANDLE responseHeadersHandle = (HTTP_HEADERS_HANDLE)userdata;
    char* headerLine = (char*)ptr;

    if (headerLine != NULL)
    {
        char* token = strtok(headerLine, "\r\n");
        while ((token != NULL) &&
               (token[0] != '\0'))
        {
            char* whereIsColon = strchr(token, ':');
            if(whereIsColon!=NULL)
            {
                *whereIsColon='\0';
                HTTPHeaders_AddHeaderNameValuePair(responseHeadersHandle, token, whereIsColon+1);
                *whereIsColon=':';
            }
            else
            {
                /*not a header, maybe a status-line*/
            }
            token = strtok(NULL, "\r\n");
        }
    }

    return size * nmemb;
}

static size_t ContentWriteFunction(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    HTTP_RESPONSE_CONTENT_BUFFER* responseContentBuffer = (HTTP_RESPONSE_CONTENT_BUFFER*)userdata;
    if ((userdata != NULL) &&
        (ptr != NULL) &&
        (size * nmemb > 0))
    {
        void* newBuffer = realloc(responseContentBuffer->buffer, responseContentBuffer->bufferSize + (size * nmemb));
        if (newBuffer != NULL)
        {
            responseContentBuffer->buffer = newBuffer;
            memcpy(responseContentBuffer->buffer + responseContentBuffer->bufferSize, ptr, size * nmemb);
            responseContentBuffer->bufferSize += size * nmemb;
        }
        else
        {
            LogError("Could not allocate buffer of size %zu\r\n", (size_t)(responseContentBuffer->bufferSize + (size * nmemb)));
            responseContentBuffer->error = 1;
            if (responseContentBuffer->buffer != NULL)
            {
                free(responseContentBuffer->buffer);
            }
        }
    }

    return size * nmemb;
}

HTTPAPI_RESULT HTTPAPI_ExecuteRequest(HTTP_HANDLE handle, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
                                      HTTP_HEADERS_HANDLE httpHeadersHandle, const unsigned char* content,
                                      size_t contentLength, unsigned int* statusCode,
                                      HTTP_HEADERS_HANDLE responseHeadersHandle, BUFFER_HANDLE responseContent)
{
    HTTPAPI_RESULT result;
    HTTP_HANDLE_DATA* httpHandleData = (HTTP_HANDLE_DATA*)handle;
    size_t headersCount;
    HTTP_RESPONSE_CONTENT_BUFFER responseContentBuffer;

    if ((httpHandleData == NULL) ||
        (relativePath == NULL) ||
        (httpHeadersHandle == NULL) ||
        ((content == NULL) && (contentLength > 0))
    )
    {
        result = HTTPAPI_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
    }
    else if (HTTPHeaders_GetHeaderCount(httpHeadersHandle, &headersCount) != HTTP_HEADERS_OK)
    {
        result = HTTPAPI_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
    }
    else
    {
        char tempHostURL[1024];

        /* set the URL */
        if ((strcpy_s(tempHostURL, sizeof(tempHostURL), httpHandleData->hostURL) != 0) ||
            (strcat_s(tempHostURL, sizeof(tempHostURL), relativePath) != 0))
        {
            result = HTTPAPI_STRING_PROCESSING_ERROR;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        }
        else if (curl_easy_setopt(httpHandleData->curl, CURLOPT_URL, tempHostURL) != CURLE_OK)
        {
            result = HTTPAPI_SET_OPTION_FAILED;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        }
        else if (curl_easy_setopt(httpHandleData->curl, CURLOPT_LOW_SPEED_LIMIT, 300L) != CURLE_OK) /*300 bytes per second is 25% of the bandwidth of a 9600bps modem*/
        {
            result = HTTPAPI_SET_OPTION_FAILED;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        }
        else if (curl_easy_setopt(httpHandleData->curl, CURLOPT_LOW_SPEED_TIME, 15L) != CURLE_OK) /*if the speed is smaller than 300 bytes per second for 15 seconds... then timeout
                                                                                                  this also implies that a server response comes in less than 15 seconds, too!*/
        {
            result = HTTPAPI_SET_OPTION_FAILED;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        }
        else if (curl_easy_setopt(httpHandleData->curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1) != CURLE_OK)
        {
            result = HTTPAPI_SET_OPTION_FAILED;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        }
        else
        {
            result = HTTPAPI_OK;

            switch (requestType)
            {
                default:
                    result = HTTPAPI_INVALID_ARG;
                    LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    break;

                case HTTPAPI_REQUEST_GET:
                    if (curl_easy_setopt(httpHandleData->curl, CURLOPT_HTTPGET, 1L) != CURLE_OK)
                    {
                        result = HTTPAPI_SET_OPTION_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    }
                    else
                    {
                        if (curl_easy_setopt(httpHandleData->curl, CURLOPT_CUSTOMREQUEST, NULL) != CURLE_OK)
                        {
                            result = HTTPAPI_SET_OPTION_FAILED;
                            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                        }
                    }
                    
                    break;

                case HTTPAPI_REQUEST_POST:
                    if (curl_easy_setopt(httpHandleData->curl, CURLOPT_POST, 1L) != CURLE_OK)
                    {
                        result = HTTPAPI_SET_OPTION_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    }
                    else
                    {
                        if (curl_easy_setopt(httpHandleData->curl, CURLOPT_CUSTOMREQUEST, NULL) != CURLE_OK)
                        {
                            result = HTTPAPI_SET_OPTION_FAILED;
                            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                        }
                    }

                    break;

                case HTTPAPI_REQUEST_PUT:
                    if (curl_easy_setopt(httpHandleData->curl, CURLOPT_POST, 1L))
                    {
                        result = HTTPAPI_SET_OPTION_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    }
                    else
                    {
                        if (curl_easy_setopt(httpHandleData->curl, CURLOPT_CUSTOMREQUEST, "PUT") != CURLE_OK)
                        {
                            result = HTTPAPI_SET_OPTION_FAILED;
                            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                        }
                    }
                    break;

                case HTTPAPI_REQUEST_DELETE:
                    if (curl_easy_setopt(httpHandleData->curl, CURLOPT_POST, 1L) != CURLE_OK)
                    {
                        result = HTTPAPI_SET_OPTION_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    }
                    else
                    {
                        if (curl_easy_setopt(httpHandleData->curl, CURLOPT_CUSTOMREQUEST, "DELETE") != CURLE_OK)
                        {
                            result = HTTPAPI_SET_OPTION_FAILED;
                            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                        }
                    }
                    break;

                case HTTPAPI_REQUEST_PATCH:
                    if (curl_easy_setopt(httpHandleData->curl, CURLOPT_POST, 1L) != CURLE_OK)
                    {
                        result = HTTPAPI_SET_OPTION_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    }
                    else
                    {
                        if (curl_easy_setopt(httpHandleData->curl, CURLOPT_CUSTOMREQUEST, "PATCH") != CURLE_OK)
                        {
                            result = HTTPAPI_SET_OPTION_FAILED;
                            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                        }
                    }
                    
                    break;
            }

            if (result == HTTPAPI_OK)
            {
                /* add headers */
                struct curl_slist* headers = NULL;
                size_t i;

                for (i = 0; i < headersCount; i++)
                {
                    char tempBuffer[TEMP_BUFFER_SIZE];
                    if(HTTPHeaders_GetHeader(httpHeadersHandle, i, tempBuffer, TEMP_BUFFER_SIZE)!=HTTP_HEADERS_OK)
                    {
                        /* error */
                        result = HTTPAPI_HTTP_HEADERS_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                        break;
                    }
                    else
                    {
                        struct curl_slist* newHeaders = curl_slist_append(headers, tempBuffer);
                        if (newHeaders == NULL)
                        {
                            result = HTTPAPI_ALLOC_FAILED;
                            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                            break;
                        }
                        else
                        {
                            headers = newHeaders;
                        }
                    }
                }

                if (result == HTTPAPI_OK)
                {
                    if (curl_easy_setopt(httpHandleData->curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK)
                    {
                        result = HTTPAPI_SET_OPTION_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    }
                    else
                    {
                        /* add content */
                        if ((content != NULL) &&
                            (contentLength > 0))
                        {
                            if ((curl_easy_setopt(httpHandleData->curl, CURLOPT_POSTFIELDS, (void*)content) != CURLE_OK) ||
                                (curl_easy_setopt(httpHandleData->curl, CURLOPT_POSTFIELDSIZE, contentLength) != CURLE_OK))
                            {
                                result = HTTPAPI_SET_OPTION_FAILED;
                                LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                            }
                        }
                        else
                        {
                            if (requestType != HTTPAPI_REQUEST_GET)
                            {
                                if ((curl_easy_setopt(httpHandleData->curl, CURLOPT_POSTFIELDS, (void*)NULL) != CURLE_OK) ||
                                    (curl_easy_setopt(httpHandleData->curl, CURLOPT_POSTFIELDSIZE, 0) != CURLE_OK))
                                {
                                    result = HTTPAPI_SET_OPTION_FAILED;
                                    LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                }
                            }
                            else
                            {
                                /*GET request cannot POST, so "do nothing*/
                            }
                        }

                        if (result == HTTPAPI_OK)
                        {
                            if ((curl_easy_setopt(httpHandleData->curl, CURLOPT_WRITEHEADER, NULL) != CURLE_OK) ||
                                (curl_easy_setopt(httpHandleData->curl, CURLOPT_HEADERFUNCTION, NULL) != CURLE_OK) ||
                                (curl_easy_setopt(httpHandleData->curl, CURLOPT_WRITEFUNCTION, ContentWriteFunction) != CURLE_OK))
                            {
                                result = HTTPAPI_SET_OPTION_FAILED;
                                LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                            }
                            else
                            {
                                if (responseHeadersHandle != NULL)
                                {
                                    /* setup the code to get the response headers */
                                    if ((curl_easy_setopt(httpHandleData->curl, CURLOPT_WRITEHEADER, responseHeadersHandle) != CURLE_OK) ||
                                        (curl_easy_setopt(httpHandleData->curl, CURLOPT_HEADERFUNCTION, HeadersWriteFunction) != CURLE_OK))
                                    {
                                        result = HTTPAPI_SET_OPTION_FAILED;
                                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                    }
                                }

                                if (result == HTTPAPI_OK)
                                {
                                    responseContentBuffer.buffer = NULL;
                                    responseContentBuffer.bufferSize = 0;
                                    responseContentBuffer.error = 0;

                                    if (curl_easy_setopt(httpHandleData->curl, CURLOPT_WRITEDATA, &responseContentBuffer) != CURLE_OK)
                                    {
                                        result = HTTPAPI_SET_OPTION_FAILED;
                                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                    }

                                    if (result == HTTPAPI_OK)
                                    {
                                        /* Execute request */
                                        CURLcode curlRes = curl_easy_perform(httpHandleData->curl);
                                        if (curlRes != CURLE_OK)
                                        {
                                            LogInfo("curl_easy_perform() failed: %s\n", curl_easy_strerror(curlRes));
                                            result = HTTPAPI_OPEN_REQUEST_FAILED;
                                            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                        }
                                        else
                                        {
                                            long httpCode;

                                            /* get the status code */
                                            if (curl_easy_getinfo(httpHandleData->curl, CURLINFO_RESPONSE_CODE, &httpCode) != CURLE_OK)
                                            {
                                                result = HTTPAPI_QUERY_HEADERS_FAILED;
                                                LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                            }
                                            else if (responseContentBuffer.error)
                                            {
                                                result = HTTPAPI_READ_DATA_FAILED;
                                                LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                            }
                                            else
                                            {
                                                if (statusCode != NULL)
                                                {
                                                    *statusCode = httpCode;
                                                }

                                                /* fill response content length */
                                                if (responseContent != NULL)
                                                {
                                                    if ((responseContentBuffer.bufferSize>0) && (BUFFER_build(responseContent, responseContentBuffer.buffer, responseContentBuffer.bufferSize) != 0))
                                                    {
                                                        result = HTTPAPI_INSUFFICIENT_RESPONSE_BUFFER;
                                                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                    }
                                                    else
                                                    {
                                                        /*all nice*/
                                                    }
                                                }

                                                if (httpCode >= 300)
                                                {
                                                    LogError("Failure in HTTP communication: server reply code is %ld\r\n", httpCode );
                                                    LogInfo("HTTP Response:\r\n%*.*s\r\n", (int)responseContentBuffer.bufferSize,
                                                        (int)responseContentBuffer.bufferSize, responseContentBuffer.buffer);
                                                }
                                                else
                                                {
                                                    result = HTTPAPI_OK;
                                                }
                                            }
                                        }
                                    }

                                    if (responseContentBuffer.buffer != NULL)
                                    {
                                        free(responseContentBuffer.buffer);
                                    }
                                }
                            }
                        }
                    }
                }

                curl_slist_free_all(headers);
            }
        }
    }

    return result;
}
