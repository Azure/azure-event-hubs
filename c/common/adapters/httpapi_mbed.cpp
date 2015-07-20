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

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include "httpapi.h"
#include "httpheaders.h"
#include "crt_abstractions.h"
#include "mbed.h"
#include "EthernetInterface.h"
#include "TLSConnection.h"
#include "iot_logging.h"
#include "string.h"
#include "NTPClient.h"

#define MAX_HOSTNAME     64
#define TEMP_BUFFER_SIZE 1024

#define CHAR_COUNT(A)   (sizeof(A) - 1)

DEFINE_ENUM_STRINGS(HTTPAPI_RESULT, HTTPAPI_RESULT_VALUES)

class HTTP_HANDLE_DATA
{
public:
    char          host[MAX_HOSTNAME];
    TLSConnection con;
};

// EthernetInterface instance
static EthernetInterface eth;
static NTPClient ntp;

HTTPAPI_RESULT HTTPAPI_Init(void)
{
    LogInfo("HTTPAPI_Init::Start\r\n");
    time_t ctTime;
    ctTime = time(NULL);
    HTTPAPI_RESULT result;
    LogInfo("HTTAPI_Init::Time is now (UTC) %s\r\n", ctime(&ctTime));
    if (eth.init())
    {
        LogInfo("HTTPAPI_Init::Error with initing the Ethernet Interface\r\n");
        result = HTTPAPI_INIT_FAILED;
    }
    else
    {
        LogInfo("HTTPAPI_Init::Ethernet interface was inited!\r\n");
        if (eth.connect(30000))
        {
            LogError("HTTPAPI_Init::Error with connecting.\r\n");
            result = HTTPAPI_INIT_FAILED;
        }
        else
        {
            LogInfo("HTTAPI_Init::Ethernet interface was connected (brought up)!\r\n");
            LogInfo("HTTAPI_Init::MAC address %s\r\n", eth.getMACAddress());
            LogInfo("HTTAPI_Init::IP address %s\r\n", eth.getIPAddress());
            if (ntp.setTime("0.pool.ntp.org") == 0)
            {
                LogInfo("HTTAPI_Init:: Successfully set time\r\n");
                LogInfo("HTTAPI_Init::Time is now (UTC) %s\r\n", ctime(&ctTime));
                result = HTTPAPI_OK;
            }
            else
            {
                LogError("HTTPAPI_INIT::Unable to set time.  Init failed");
                result = HTTPAPI_INIT_FAILED;
            }
        }
    }
    LogInfo("HTTPAPI_Init::End\r\n");
    return result;
}

void HTTPAPI_Deinit(void)
{
    (void)eth.disconnect();
}
HTTP_HANDLE HTTPAPI_CreateConnection(const char* hostName)
{
    LogInfo("HTTPAPI_CreateConnection::Start\r\n");
    HTTP_HANDLE_DATA* handle = NULL;

    if (hostName)
    {
        LogInfo("HTTPAPI_CreateConnection::Connecting to %s\r\n", hostName);
        handle = new HTTP_HANDLE_DATA();
        if (strcpy_s(handle->host, MAX_HOSTNAME, hostName) != 0)
        {
            LogError("HTTPAPI_CreateConnection::Could not strcpy_s\r\n");
            delete handle;
            handle = NULL;
        }
        else if(!handle->con.connect(hostName))
        {
            LogError("HTTPAPI_CreateConnection::Could not connect\r\n");
            delete handle;
            handle = NULL;
        }
        else
        {
            LogInfo("HTTPAPI_CreateConnection::Connection to %s successful!\r\n", hostName);
        }
    }
    else
    {
        LogInfo("HTTPAPI_CreateConnection:: null hostName parameter\r\n");
    }
    LogInfo("HTTPAPI_CreateConnection::End\r\n");

    return (HTTP_HANDLE)handle;
}

void HTTPAPI_CloseConnection(HTTP_HANDLE handle)
{
    HTTP_HANDLE_DATA* h = (HTTP_HANDLE_DATA*)handle;
    
    if (h)
    {
        LogInfo("HTTPAPI_CloseConnection to %s\r\n", h->host);
        if (h->con.is_connected())
        {
            LogInfo("HTTPAPI_CloseConnection  h->con.close(); to %s\r\n", h->host);
            h->con.close();
        }
        LogInfo("HTTPAPI_CloseConnection  delete h to %s\r\n", h->host);
        delete h;
    }
}

static int readLine(TLSConnection* con, char* buf, const size_t size)
{
    // reads until \r\n is encountered. writes in buf all the characters
    // read until \r\n and returns the number of characters in the buffer.
    // returns -1 in case of error.
    char* p = buf;
    char  c;
    if (con->receive(&c, 1) < 0)
        return -1;
    while (c != '\r') {
        if ((p - buf + 1) >= (int)size)
            return -1;
        *p++ = c;
        if (con->receive(&c, 1) < 0)
            return -1;
    }
    *p = 0;
    if (con->receive(&c, 1) < 0 || c != '\n') // skip \n
        return -1;
    return p - buf;
}

static int readChunk(TLSConnection* con, char* buf, size_t size)
{
    size_t cur, offset;

    // read content with specified length, even if it is received
    // only in chunks due to fragmentation in the networking layer.
    // returns -1 in case of error.
    offset = 0;
    while (size > 0)
    {
        cur = con->receive(buf + offset, size);

        // error during read
        if (cur < 0)
            return -1;

        // end of stream reached
        if (cur == 0)
            return offset;

        // read cur bytes (might be less than requested)
        size -= cur;
        offset += cur;
    }

    return offset;
}

static int skipN(TLSConnection* con, size_t n, char* buf, size_t size)
{
    size_t org = n;
    // read and abandon response content with specified length
    // returns -1 in case of error.
    while (n > size)
    {
        if (readChunk(con, (char*)buf, size) < 0)
            return -1;

        n -= size;
    }

    if (readChunk(con, (char*)buf, n) < 0)
        return -1;

    return org;
}

//Note: This function assumes that "Host:" and "Content-Length:" headers are setup
//      by the caller of HTTPAPI_ExecuteRequest() (which is true for httptransport.c).
HTTPAPI_RESULT HTTPAPI_ExecuteRequest(HTTP_HANDLE handle, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
    HTTP_HEADERS_HANDLE httpHeadersHandle, const unsigned char* content,
    size_t contentLength, unsigned int* statusCode,
    HTTP_HEADERS_HANDLE responseHeadersHandle, BUFFER_HANDLE responseContent)
{
    LogInfo("HTTPAPI_ExecuteRequest::Start\r\n");

    HTTPAPI_RESULT result;
    size_t  headersCount;
    char    buf[TEMP_BUFFER_SIZE];
    int     ret;
    TLSConnection* con = NULL;
    size_t  bodyLength = 0;
    bool    chunked = false;
    const unsigned char* receivedContent;

    const char* method = (requestType == HTTPAPI_REQUEST_GET) ? "GET"
        : (requestType == HTTPAPI_REQUEST_POST) ? "POST"
        : (requestType == HTTPAPI_REQUEST_PUT) ? "PUT"
        : (requestType == HTTPAPI_REQUEST_DELETE) ? "DELETE"
        : (requestType == HTTPAPI_REQUEST_PATCH) ? "PATCH"
        : NULL;

    if (handle == NULL ||
        relativePath == NULL ||
        httpHeadersHandle == NULL ||
        method == NULL ||
        HTTPHeaders_GetHeaderCount(httpHeadersHandle, &headersCount) != HTTP_HEADERS_OK)
    {
        result = HTTPAPI_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        goto exit;
    }

    con = &(((HTTP_HANDLE_DATA*)handle)->con);

    //Send request
    if ((ret = snprintf(buf, sizeof(buf), "%s %s HTTP/1.1\r\n", method, relativePath)) < 0
        || ret >= sizeof(buf))
    {
        result = HTTPAPI_STRING_PROCESSING_ERROR;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        goto exit;
    }
    LogInfo("HTTPAPI_ExecuteRequest::Sending=%*.*s\r\n", strlen(buf), strlen(buf), buf);
    if (con->send_all(buf, strlen(buf)) < 0)
    {
        result = HTTPAPI_SEND_REQUEST_FAILED;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        goto exit;
    }

    //Send default headers
    for (int i = 0; i < (int)headersCount; i++)
    {
        if (HTTPHeaders_GetHeader(httpHeadersHandle, i, buf, sizeof(buf)) != HTTP_HEADERS_OK)
        {
            result = HTTPAPI_HTTP_HEADERS_FAILED;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
            goto exit;
        }
        LogInfo("HTTPAPI_ExecuteRequest::Sending=%*.*s\r\n", strlen(buf), strlen(buf), buf);
        if (con->send_all(buf, strlen(buf)) < 0)
        {
            result = HTTPAPI_SEND_REQUEST_FAILED;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
            goto exit;
        }
        if (con->send_all("\r\n", 2) < 0)
        {
            result = HTTPAPI_SEND_REQUEST_FAILED;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
            goto exit;
        }
    }

    //Close headers
    if (con->send_all("\r\n", 2) < 0)
    {
        result = HTTPAPI_SEND_REQUEST_FAILED;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        goto exit;
    }

    //Send data (if available)
    if (content && contentLength > 0)
    {
        LogInfo("HTTPAPI_ExecuteRequest::Sending data=%*.*s\r\n", contentLength, contentLength, content);
        if (con->send_all((char*)content, contentLength) < 0)
        {
            result = HTTPAPI_SEND_REQUEST_FAILED;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
            goto exit;
        }
    }

    //Receive response
    if (readLine(con, buf, sizeof(buf)) < 0)
    {
        result = HTTPAPI_READ_DATA_FAILED;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        goto exit;
    }

    //Parse HTTP response
    if (sscanf(buf, "HTTP/%*d.%*d %d %*[^\r\n]", &ret) != 1)
    {
        //Cannot match string, error
        LogInfo("HTTPAPI_ExecuteRequest::Not a correct HTTP answer=%s\r\n", buf);
        result = HTTPAPI_READ_DATA_FAILED;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        goto exit;
    }
    if (statusCode)
        *statusCode = ret;
    LogInfo("HTTPAPI_ExecuteRequest::Received response=%*.*s\r\n", strlen(buf), strlen(buf), buf);

    //Read HTTP response headers
    if (readLine(con, buf, sizeof(buf)) < 0)
    {
        result = HTTPAPI_READ_DATA_FAILED;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        goto exit;
    }

    while (buf[0])
    {
        const char ContentLength[] = "content-length:";
        const char TransferEncoding[] = "transfer-encoding:";

        LogInfo("Receiving header=%*.*s\r\n", strlen(buf), strlen(buf), buf);

        if (strncasecmp(buf, ContentLength, CHAR_COUNT(ContentLength)) == 0)
        {
            if (sscanf(buf + CHAR_COUNT(ContentLength), " %d", &bodyLength) != 1)
            {
                result = HTTPAPI_READ_DATA_FAILED;
                LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                goto exit;
            }
        }
        else if (strncasecmp(buf, TransferEncoding, CHAR_COUNT(TransferEncoding)) == 0)
        {
            const char* p = buf + CHAR_COUNT(TransferEncoding);
            while (isspace(*p)) p++;
            if (strcasecmp(p, "chunked") == 0)
                chunked = true;
        }

        char* whereIsColon = strchr((char*)buf, ':');
        if (whereIsColon && responseHeadersHandle != NULL)
        {
            *whereIsColon = '\0';
            HTTPHeaders_AddHeaderNameValuePair(responseHeadersHandle, buf, whereIsColon + 1);
        }

        if (readLine(con, buf, sizeof(buf)) < 0)
        {
            result = HTTPAPI_READ_DATA_FAILED;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
            goto exit;
        }
    }

    //Read HTTP response body
    LogInfo("HTTPAPI_ExecuteRequest::Receiving body=%d,%x\r\n", bodyLength, responseContent);
    if (!chunked)
    {
        if (bodyLength)
        {
            if (responseContent != NULL)
            {
                if (BUFFER_pre_build(responseContent, bodyLength) != 0)
                {
                    result = HTTPAPI_ALLOC_FAILED;
                    LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                }
                else if (BUFFER_content(responseContent, &receivedContent) != 0)
                {
                    (void)BUFFER_unbuild(responseContent);

                    result = HTTPAPI_ALLOC_FAILED;
                    LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                }

                if (readChunk(con, (char*)receivedContent, bodyLength) < 0)
                {
                    result = HTTPAPI_READ_DATA_FAILED;
                    LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    goto exit;
                }
                else
                {
                    LogInfo("HTTPAPI_ExecuteRequest::Received response body=%*.*s\r\n", bodyLength, bodyLength, receivedContent);
                    result = HTTPAPI_OK;
                }
            }
            else
            {
                (void)skipN(con, bodyLength, buf, sizeof(buf));
                result = HTTPAPI_OK;
            }
        }
        else
        {
            result = HTTPAPI_OK;
        }
    }
    else
    {
        size_t size = 0;
        result = HTTPAPI_OK;
        for (;;)
        {
            int chunkSize;
            if (readLine(con, buf, sizeof(buf)) < 0)    // read [length in hex]/r/n
            {
                result = HTTPAPI_READ_DATA_FAILED;
                LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                goto exit;
            }
            if (sscanf(buf, "%x", &chunkSize) != 1)     // chunkSize is length of next line (/r/n is not counted)
            {
                //Cannot match string, error
                result = HTTPAPI_RECEIVE_RESPONSE_FAILED;
                LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                goto exit;
            }

            if (chunkSize == 0)
            {
                // 0 length means next line is just '\r\n' and end of chunks
                if (readChunk(con, (char*)buf, 2) < 0
                    || buf[0] != '\r' || buf[1] != '\n') // skip /r/n
                {
                    (void)BUFFER_unbuild(responseContent);

                    result = HTTPAPI_READ_DATA_FAILED;
                    LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    goto exit;
                }
                break;
            }
            else
            {
                if (responseContent != NULL)
                {
                    if (BUFFER_enlarge(responseContent, chunkSize) != 0)
                    {
                        (void)BUFFER_unbuild(responseContent);

                        result = HTTPAPI_ALLOC_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    }
                    else if (BUFFER_content(responseContent, &receivedContent) != 0)
                    {
                        (void)BUFFER_unbuild(responseContent);

                        result = HTTPAPI_ALLOC_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    }

                    if (readChunk(con, (char*)receivedContent + size, chunkSize) < 0)
                    {
                        result = HTTPAPI_READ_DATA_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                        goto exit;
                    }
                }
                else
                {
                    if (skipN(con, chunkSize, buf, sizeof(buf)) < 0)
                    {
                        result = HTTPAPI_READ_DATA_FAILED;
                        LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                        goto exit;
                    }
                }

                if (readChunk(con, (char*)buf, 2) < 0
                    || buf[0] != '\r' || buf[1] != '\n') // skip /r/n
                {
                    result = HTTPAPI_READ_DATA_FAILED;
                    LogError("(result = %s)\r\n", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    goto exit;
                }
                size += chunkSize;
            }
        }

        if (size > 0)
        {
            LogInfo("HTTPAPI_ExecuteRequest::Received chunk body=%*.*s\r\n", size, size, responseContent);
        }
    }

exit:
    LogInfo("HTTPAPI_ExecuteRequest::End=%d\r\n", result);
    return result;
}
