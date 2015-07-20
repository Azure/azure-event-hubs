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

#ifndef HTTPAPI_H
#define HTTPAPI_H

#include "httpheaders.h"
#include "macro_utils.h"
#include "buffer_.h"

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif

typedef void* HTTP_HANDLE;

#define AMBIGUOUS_STATUS_CODE           (300)

#define HTTPAPI_RESULT_VALUES                \
HTTPAPI_OK,                                  \
HTTPAPI_INVALID_ARG,                         \
HTTPAPI_ERROR,                               \
HTTPAPI_OPEN_REQUEST_FAILED,                 \
HTTPAPI_SET_OPTION_FAILED,                   \
HTTPAPI_SEND_REQUEST_FAILED,                 \
HTTPAPI_RECEIVE_RESPONSE_FAILED,             \
HTTPAPI_QUERY_HEADERS_FAILED,                \
HTTPAPI_QUERY_DATA_AVAILABLE_FAILED,         \
HTTPAPI_READ_DATA_FAILED,                    \
HTTPAPI_ALREADY_INIT,                        \
HTTPAPI_NOT_INIT,                            \
HTTPAPI_HTTP_HEADERS_FAILED,                 \
HTTPAPI_STRING_PROCESSING_ERROR,             \
HTTPAPI_ALLOC_FAILED,                        \
HTTPAPI_INIT_FAILED,                         \
HTTPAPI_INSUFFICIENT_RESPONSE_BUFFER,        \
HTTPAPI_SET_TIMEOUTS_FAILED                  \

DEFINE_ENUM(HTTPAPI_RESULT, HTTPAPI_RESULT_VALUES);

#define HTTPAPI_REQUEST_TYPE_VALUES\
    HTTPAPI_REQUEST_GET,            \
    HTTPAPI_REQUEST_POST,           \
    HTTPAPI_REQUEST_PUT,            \
    HTTPAPI_REQUEST_DELETE,         \
    HTTPAPI_REQUEST_PATCH           \

DEFINE_ENUM(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE_VALUES);


extern HTTPAPI_RESULT HTTPAPI_Init(void);
extern void HTTPAPI_Deinit(void);
extern HTTP_HANDLE HTTPAPI_CreateConnection(const char* hostName);
extern void HTTPAPI_CloseConnection(HTTP_HANDLE handle);
extern HTTPAPI_RESULT HTTPAPI_ExecuteRequest(HTTP_HANDLE handle, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
                                             HTTP_HEADERS_HANDLE httpHeadersHandle, const unsigned char* content,
                                             size_t contentLength, unsigned int* statusCode,
                                             HTTP_HEADERS_HANDLE responseHeadersHandle, BUFFER_HANDLE responseContent);

#ifdef __cplusplus
}
#endif

#endif /* HTTPAPI_H */
