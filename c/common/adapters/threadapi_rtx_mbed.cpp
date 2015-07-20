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

#include "stdlib.h"
#include "threadapi.h"
#include "rtos.h"
#include "iot_logging.h"

DEFINE_ENUM_STRINGS(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

#define MAX_THREADS 4
#define STACK_SIZE  0x4000

typedef struct _thread
{
    Thread*       thrd;
    osThreadId    id;
    Queue<int, 1> result;
} mbedThread;
static mbedThread threads[MAX_THREADS] = { 0 };

typedef struct _create_param
{
    THREAD_START_FUNC func;
    const void* arg;
    mbedThread *p_thread;
} create_param;

static void thread_wrapper(const void* createParamArg)
{
    const create_param* p = (const create_param*)createParamArg;
    p->p_thread->id = Thread::gettid();
    (*(p->func))((void*)p->arg);
    free((void*)p);
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
        size_t slot;
        for (slot = 0; slot < MAX_THREADS; slot++)
        {
            if (threads[slot].id == NULL)
                break;
        }

        if (slot < MAX_THREADS)
        {
            create_param* param = (create_param*)malloc(sizeof(create_param));
            if (param != NULL)
            {
                param->func = func;
                param->arg = arg;
                param->p_thread = threads + slot;
                threads[slot].thrd = new Thread(thread_wrapper, param, osPriorityNormal, STACK_SIZE);
                *threadHandle = (THREAD_HANDLE)(threads + slot);
                result = THREADAPI_OK;
            }
            else
            {
                result = THREADAPI_NO_MEMORY;
                LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
            }
        }
        else
        {
            result = THREADAPI_NO_MEMORY;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
        }
    }

    return result;
}

THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE thr, int *res)
{
    THREADAPI_RESULT result = THREADAPI_OK;
    mbedThread* p = (mbedThread*)thr;
    if (p)
    {
        osEvent evt = p->result.get();
        if (evt.status == osEventMessage) {
            Thread* t = p->thrd;
            if (res)
            {
                *res = (int)evt.value.p;
            }
            (void)t->terminate();
        }
        else
        {
            result = THREADAPI_ERROR;
            LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
        }
    }
    else
    {
        result = THREADAPI_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
    }
    return result;
}

void ThreadAPI_Exit(int res)
{
    mbedThread* p;
    for (p = threads; p < &threads[MAX_THREADS]; p++)
    {
        if (p->id == Thread::gettid())
        {
            p->result.put((int*)res);
            break;
        }
    }
}

void ThreadAPI_Sleep(unsigned int millisec)
{
    //
    // The timer on mbed seems to wrap around 65 seconds. Hmmm.
    // So we will do our waits in increments of 30 seconds.
    //
    const int thirtySeconds = 30000;
    int numberOfThirtySecondWaits = millisec / thirtySeconds;
    int remainderOfThirtySeconds = millisec % thirtySeconds;
    int i;
    for (i = 1; i <= numberOfThirtySecondWaits; i++)
    {
        Thread::wait(thirtySeconds);
    }
    Thread::wait(remainderOfThirtySeconds);
}
