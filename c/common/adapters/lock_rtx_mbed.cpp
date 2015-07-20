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

#include "lock.h"
#include "iot_logging.h"
#include "rtos.h"

DEFINE_ENUM_STRINGS(LOCK_RESULT, LOCK_RESULT_VALUES);

/*Tests_SRS_LOCK_99_002:[ This API on success will return a valid lock handle which should be a non NULL value]*/
LOCK_HANDLE Lock_Init(void)
{
    Mutex* lock_mtx = new Mutex();
  
    return (LOCK_HANDLE)lock_mtx;
}


LOCK_RESULT Lock(LOCK_HANDLE handle)
{
    LOCK_RESULT result;
    if (handle == NULL)
    {
        /*Tests_SRS_LOCK_99_007:[ This API on NULL handle passed returns LOCK_ERROR]*/
        result = LOCK_ERROR;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(LOCK_RESULT, result));
    }
    else
    {
        Mutex* lock_mtx = (Mutex*)handle;
        if (lock_mtx->lock() == osOK)
        {
            /*Tests_SRS_LOCK_99_005:[ This API on success should return LOCK_OK]*/
            result = LOCK_OK;
        }
        else
        {
            /*Tests_SRS_LOCK_99_006:[ This API on error should return LOCK_ERROR]*/
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
        /*Tests_SRS_LOCK_99_011:[ This API on NULL handle passed returns LOCK_ERROR]*/
        result = LOCK_ERROR;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(LOCK_RESULT, result));
    }
    else
    {
        Mutex* lock_mtx = (Mutex*)handle;
        if (lock_mtx->unlock() == osOK)
        {
            /*Tests_SRS_LOCK_99_009:[ This API on success should return LOCK_OK]*/
            result = LOCK_OK;
        }
        else
        {
            /*Tests_SRS_LOCK_99_010:[ This API on error should return LOCK_ERROR]*/
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
        /*Tests_SRS_LOCK_99_013:[ This API on NULL handle passed returns LOCK_ERROR]*/
        result = LOCK_ERROR;
        LogError("(result = %s)\r\n", ENUM_TO_STRING(LOCK_RESULT, result));
    }
    else
    {
        /*Tests_SRS_LOCK_99_012:[ This API frees the memory pointed by handle]*/
        Mutex* lock_mtx = (Mutex*)handle;
        delete lock_mtx;
    }
    
    return result;
}
