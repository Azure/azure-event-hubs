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

#include "lock.h"
#include <pthread.h>
#include <stdlib.h>
#include <iot_logging.h>
#include <errno.h>
#include <time.h>

DEFINE_ENUM_STRINGS(LOCK_RESULT, LOCK_RESULT_VALUES);
DEFINE_ENUM_STRINGS(COND_RESULT, COND_RESULT_VALUES);

/*SRS_LOCK_99_002:[ This API on success will return a valid lock handle which should be a non NULL value]*/
LOCK_HANDLE Lock_Init(void)
{
	pthread_mutex_t* lock_mtx = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	if (NULL != lock_mtx)
	{
		if (pthread_mutex_init(lock_mtx, NULL) != 0)
		{
			/*SRS_LOCK_99_003:[ On Error Should return NULL]*/
			free(lock_mtx);
			lock_mtx = NULL;
			LogError("Failed to initialize mutex\r\n");
		}
	}
	
	return (LOCK_HANDLE)lock_mtx;
}


LOCK_RESULT Lock(LOCK_HANDLE handle)
{
	LOCK_RESULT result;
	if (handle == NULL)
	{
		/*SRS_LOCK_99_007:[ This API on NULL handle passed returns LOCK_ERROR]*/
		result = LOCK_ERROR;
		LogError("(result = %s)\r\n", ENUM_TO_STRING(LOCK_RESULT, result));
	}
	else
	{
		if ( pthread_mutex_lock((pthread_mutex_t*)handle) == 0)
		{
			/*SRS_LOCK_99_005:[ This API on success should return LOCK_OK]*/
			result = LOCK_OK;
		}
		else
		{
			/*SRS_LOCK_99_006:[ This API on error should return LOCK_ERROR]*/
			result = LOCK_ERROR;
			LogError("(result = %s)\r\n", ENUM_TO_STRING(LOCK_RESULT, result));
		}
	}
	return result;
}
LOCK_RESULT Unlock(LOCK_HANDLE handle)
{
	LOCK_RESULT result;
	if (handle == NULL)
	{
		/*SRS_LOCK_99_011:[ This API on NULL handle passed returns LOCK_ERROR]*/
		result = LOCK_ERROR;
		LogError("(result = %s)\r\n", ENUM_TO_STRING(LOCK_RESULT, result));
	}
	else
	{
		if (pthread_mutex_unlock((pthread_mutex_t*)handle) == 0)
		{
			/*SRS_LOCK_99_009:[ This API on success should return LOCK_OK]*/
			result = LOCK_OK;
		}
		else
		{
			/*SRS_LOCK_99_010:[ This API on error should return LOCK_ERROR]*/
			result = LOCK_ERROR;
			LogError("(result = %s)\r\n", ENUM_TO_STRING(LOCK_RESULT, result));
		}
	}
	return result;
}

LOCK_RESULT Lock_Deinit(LOCK_HANDLE handle)
{
	LOCK_RESULT result=LOCK_OK ;
	if (NULL == handle)
	{
		/*SRS_LOCK_99_013:[ This API on NULL handle passed returns LOCK_ERROR]*/
		result = LOCK_ERROR;
		LogError("(result = %s)\r\n", ENUM_TO_STRING(LOCK_RESULT, result));
	}
	else
	{
		/*SRS_LOCK_99_012:[ This API frees the memory pointed by handle]*/
		if(pthread_mutex_destroy((pthread_mutex_t*)handle)==0)
		{
			free(handle);
			handle = NULL;
		}
		else
		{
			result = LOCK_ERROR;
			LogError("(result = %s)\r\n", ENUM_TO_STRING(LOCK_RESULT, result));
		}
	}
	
	return result;
 }
 
 
COND_HANDLE Condition_Init(void)
{
    pthread_cond_t * cond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(cond, NULL);
    return cond;
}

COND_RESULT Condition_Post(COND_HANDLE handle)
{
    pthread_cond_broadcast((pthread_cond_t*)handle);
    return COND_OK;
}

COND_RESULT Condition_Wait(COND_HANDLE  handle, LOCK_HANDLE lock, int timeout_milliseconds)
{
    if ( timeout_milliseconds > 0)
    {
        struct timespec tm1;
        tm1.tv_sec = timeout_milliseconds / 1000;
        tm1.tv_nsec = (timeout_milliseconds % 1000) * 1000000L;
        int wait_result = pthread_cond_timedwait((pthread_cond_t *)handle, (pthread_mutex_t *)lock, &tm1);
        if ( wait_result == ETIMEDOUT)
        {
            return COND_TIMEOUT;
        }
        else
        {
            return COND_ERROR;
        }
    }
    else
    {
        if ( pthread_cond_wait((pthread_cond_t*)handle, (pthread_mutex_t *)lock) != 0 )
        {
            return COND_ERROR;
        }
    }
    return COND_OK;
}

COND_RESULT Condition_Deinit(COND_HANDLE  handle)
{
    pthread_cond_destroy((pthread_cond_t*)handle);
    return COND_OK;
}

