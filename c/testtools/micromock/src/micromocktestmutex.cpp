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

#include "stdafx.h"
#include "micromocktestmutex.h"

#ifdef WIN32

#include "windows.h"

#if (defined _WIN32_WCE)

HANDLE MicroMockCreateMutex(void)
{
    return CreateMutexW(NULL, FALSE, NULL);
}

HANDLE MicroMockCreateGlobalSemaphore(const char* name, unsigned int highWaterCount)
{
    WCHAR temp[MAX_PATH]; 
    if(MultiByteToWideChar(CP_ACP, 0, name, -1, temp,MAX_PATH)==0)
    {
        return NULL;
    }
    else
    {
        return CreateSemaphoreW(NULL, highWaterCount,highWaterCount, temp);
    }
}

#else

HANDLE MicroMockCreateMutex(void)
{
    return CreateMutex(NULL, FALSE, NULL);
}

HANDLE MicroMockCreateGlobalSemaphore(const char* name, unsigned int highWaterCount)
{
    HANDLE temp = CreateSemaphore(NULL, highWaterCount, highWaterCount, name);
    return temp;
}
#endif



int MicroMockAcquireMutex(HANDLE Mutex)
{
    int temp = (WaitForSingleObject((Mutex), INFINITE) == WAIT_OBJECT_0);
    return temp;
}

int MicroMockAcquireGlobalSemaphore(HANDLE semaphore)
{
    int temp = (WaitForSingleObject(semaphore, INFINITE) == WAIT_OBJECT_0);
    return temp;
}

int MicroMockReleaseGlobalSemaphore(HANDLE semaphore)
{
    LONG prev;
    (void)ReleaseSemaphore(semaphore, 1, &prev);
    return prev;
}

void MicroMockDestroyGlobalSemaphore(HANDLE semaphore)
{
    (void)CloseHandle(semaphore);
}

void MicroMockDestroyMutex(HANDLE Mutex)
{
    (void)CloseHandle(Mutex);
}

int MicroMockReleaseMutex(HANDLE Mutex)
{   
    return ReleaseMutex(Mutex);
}

#else

typedef void* MICROMOCK_MUTEX_HANDLE;

#define MicroMockCreateMutex()              ((MICROMOCK_MUTEX_HANDLE)1)
#define MicroMockCreateGlobalMutex(x)       ((MICROMOCK_GLOBAL_MUTEX_HANDLE)2)
#define MicroMockDestroyMutex(Mutex)
#define MicroMockAcquireMutex(Mutex)    (1)
#define MicroMockReleaseMutex(Mutex)    (1)

#endif
