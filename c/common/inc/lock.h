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

#ifndef LOCK_H
#define LOCK_H

#include "macro_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* LOCK_HANDLE;

#define LOCK_RESULT_VALUES \
    LOCK_OK, \
    LOCK_ERROR \

DEFINE_ENUM(LOCK_RESULT, LOCK_RESULT_VALUES);

extern LOCK_HANDLE Lock_Init(void);
extern LOCK_RESULT Lock(LOCK_HANDLE  handle);
extern LOCK_RESULT Unlock(LOCK_HANDLE  handle);
extern LOCK_RESULT Lock_Deinit(LOCK_HANDLE  handle);

    
typedef void* COND_HANDLE;
    
#define COND_RESULT_VALUES \
    COND_OK, \
    COND_ERROR, \
    COND_TIMEOUT \

DEFINE_ENUM(COND_RESULT, COND_RESULT_VALUES);
    
extern COND_HANDLE Condition_Init(void);
extern COND_RESULT Condition_Post(COND_HANDLE  handle);
extern COND_RESULT Condition_Wait(COND_HANDLE  handle, LOCK_HANDLE lock, int timeout_milliseconds);
extern COND_RESULT Condition_Deinit(COND_HANDLE  handle);
    

#ifdef __cplusplus
}
#endif

#endif /* LOCK_H */
