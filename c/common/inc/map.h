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

#ifndef MAP_H
#define MAP_H

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif


#include "macro_utils.h"
#include "strings.h"
#include "crt_abstractions.h"

#define MAP_RESULT_VALUES \
    MAP_OK, \
    MAP_ERROR, \
    MAP_INVALIDARG, \
    MAP_KEYEXISTS, \
    MAP_KEYNOTFOUND, \
    MAP_FILTER_REJECT

DEFINE_ENUM(MAP_RESULT, MAP_RESULT_VALUES);

typedef void* MAP_HANDLE;

typedef int (*MAP_FILTER_CALLBACK)(const char* mapProperty, const char* mapValue);

extern MAP_HANDLE Map_Create(MAP_FILTER_CALLBACK mapFilterFunc);
extern void Map_Destroy(MAP_HANDLE handle);
extern MAP_HANDLE Map_Clone(MAP_HANDLE handle);

extern MAP_RESULT Map_Add(MAP_HANDLE handle, const char* key, const char* value);
extern MAP_RESULT Map_AddOrUpdate(MAP_HANDLE handle, const char* key, const char* value);
extern MAP_RESULT Map_Delete(MAP_HANDLE handle, const char* key);

extern MAP_RESULT Map_ContainsKey(MAP_HANDLE handle, const char* key, bool* keyExists);
extern MAP_RESULT Map_ContainsValue(MAP_HANDLE handle, const char* value, bool* valueExists);
extern const char* Map_GetValueFromKey(MAP_HANDLE handle, const char* key);

extern MAP_RESULT Map_GetInternals(MAP_HANDLE handle, const char*const** keys, const char*const** values, size_t* count);

#ifdef __cplusplus
}
#endif

#endif /* MAP_H */
