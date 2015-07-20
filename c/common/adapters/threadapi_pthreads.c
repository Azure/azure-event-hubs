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

#define _BSD_SOURCE

#include "threadapi.h"
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "iot_logging.h"

DEFINE_ENUM_STRINGS(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

typedef struct THREAD_INSTANCE_TAG
{
    pthread_t Pthread_handle;
    THREAD_START_FUNC ThreadStartFunc;
    void* Arg;
} THREAD_INSTANCE;

static void* ThreadWrapper(void* threadInstanceArg)
{
    THREAD_INSTANCE* threadInstance = (THREAD_INSTANCE*)threadInstanceArg;
    int result = threadInstance->ThreadStartFunc(threadInstance->Arg);
    return (void*)(intptr_t)result;
}

THREADAPI_RESULT ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    THREADAPI_RESULT result;

    if ((threadHandle == NULL) ||
        (func == NULL))
    {
        result = THREADAPI_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
    }
    else
    {
        THREAD_INSTANCE* threadInstance = malloc(sizeof(THREAD_INSTANCE));
        if (threadInstance == NULL)
        {
            result = THREADAPI_NO_MEMORY;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
        }
        else
        {
            threadInstance->ThreadStartFunc = func;
            threadInstance->Arg = arg;
            int createResult = pthread_create(&threadInstance->Pthread_handle, NULL, ThreadWrapper, threadInstance);
            switch (createResult)
            {
            default:
                free(threadInstance);

                result = THREADAPI_ERROR;
                LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
                break;

            case 0:
                *threadHandle = threadInstance;
                result = THREADAPI_OK;
                break;

            case EAGAIN:
                free(threadInstance);

                result = THREADAPI_NO_MEMORY;
                LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
                break;
            }
        }
    }

    return result;
}

THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE threadHandle, int* res)
{
    THREADAPI_RESULT result;

    THREAD_INSTANCE* threadInstance = (THREAD_INSTANCE*)threadHandle;
    if (threadInstance == NULL)
    {
        result = THREADAPI_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
    }
    else
    {
        void* threadResult;
        if (pthread_join(threadInstance->Pthread_handle, &threadResult) != 0)
        {
            result = THREADAPI_ERROR;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
        }
        else
        {
            if (res != NULL)
            {
                *res = (int)(intptr_t)threadResult;
            }

            result = THREADAPI_OK;
        }

        free(threadInstance);
    }

    return result;
}

void ThreadAPI_Exit(int res)
{
    pthread_exit((void*)(intptr_t)res);
}

void ThreadAPI_Sleep(unsigned int milliseconds)
{
    time_t seconds = milliseconds / 1000;
    long nsRemainder = (milliseconds % 1000) * 1000000;
    struct timespec timeToSleep = { seconds, nsRemainder };
    (void)nanosleep(&timeToSleep, NULL);
}

