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
#include "threadapi.h"
#include "windows.h"
#include "iot_logging.h"

DEFINE_ENUM_STRINGS(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

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
		*threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);
		if(threadHandle == NULL)
		{
			result = THREADAPI_ERROR;
			LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
		}
		else
		{
			result = THREADAPI_OK;
		}
    }

    return result;
}

THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE threadHandle, int *res)
{
    THREADAPI_RESULT result = THREADAPI_OK;

    if (threadHandle == NULL)
    {
        result = THREADAPI_INVALID_ARG;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(THREADAPI_RESULT, result));
    }
    else
    {
        DWORD returnCode = WaitForSingleObject(threadHandle, INFINITE);
        
        if( returnCode != WAIT_OBJECT_0)
        {
            result = THREADAPI_ERROR;
            LogError("Error waiting for Single Object. Return Code: %d. Error Code: %d\r\n", returnCode);
        }
		else if((res != NULL) && !GetExitCodeThread(threadHandle, res)) //If thread end is signaled we need to get the Thread Exit Code;
        {
            DWORD errorCode = GetLastError();
            result = THREADAPI_ERROR;
            LogError("Error Getting Exit Code. Error Code: %d.\r\n", errorCode);
        }
        CloseHandle(threadHandle);
    }

    return result;
}

void ThreadAPI_Exit(int res)
{
	ExitThread(res);
}

void ThreadAPI_Sleep(unsigned int milliseconds)
{
    Sleep(milliseconds);
}
