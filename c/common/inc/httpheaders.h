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

#ifndef HTTPHEADERS_H
#define HTTPHEADERS_H

#include "macro_utils.h"

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif

/*Codes_SRS_HTTP_HEADERS_99_001:[ HttpHeaders shall have the following interface]*/

#define HTTP_HEADERS_RESULT_VALUES \
HTTP_HEADERS_OK,                  \
HTTP_HEADERS_INVALID_ARG,         \
HTTP_HEADERS_ALLOC_FAILED,        \
HTTP_HEADERS_INSUFFICIENT_BUFFER, \
HTTP_HEADERS_ERROR                \

DEFINE_ENUM(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);
typedef void* HTTP_HEADERS_HANDLE;
 
extern HTTP_HEADERS_HANDLE HTTPHeaders_Alloc(void);
extern void HTTPHeaders_Free(HTTP_HEADERS_HANDLE httpHeadersHandle);
extern HTTP_HEADERS_RESULT HTTPHeaders_AddHeaderNameValuePair(HTTP_HEADERS_HANDLE httpHeadersHandle, const char* name, const char* value);
extern HTTP_HEADERS_RESULT HTTPHeaders_ReplaceHeaderNameValuePair(HTTP_HEADERS_HANDLE httpHeadersHandle, const char* name, const char* value);
extern const char* HTTPHeaders_FindHeaderValue(HTTP_HEADERS_HANDLE httpHeadersHandle, const char* name);
extern HTTP_HEADERS_RESULT HTTPHeaders_GetHeaderCount(HTTP_HEADERS_HANDLE httpHeadersHandle, size_t* headersCount);
extern HTTP_HEADERS_RESULT HTTPHeaders_GetHeader(HTTP_HEADERS_HANDLE httpHeadersHandle, size_t index, char* buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif 

#endif /* HTTPHEADERS_H */
