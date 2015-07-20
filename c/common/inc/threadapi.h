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

#ifndef THREADAPI_H
#define THREADAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "macro_utils.h"

typedef int(*THREAD_START_FUNC)(void *);

#define THREADAPI_RESULT_VALUES \
    THREADAPI_OK,               \
    THREADAPI_INVALID_ARG,      \
    THREADAPI_NO_MEMORY,        \
    THREADAPI_ERROR

DEFINE_ENUM(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

typedef void* THREAD_HANDLE;

extern THREADAPI_RESULT ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg);
extern THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE threadHandle, int* res);
extern void ThreadAPI_Exit(int res);
extern void ThreadAPI_Sleep(unsigned int milliseconds);

#ifdef __cplusplus
}
#endif

#endif /* THREADAPI_H */
