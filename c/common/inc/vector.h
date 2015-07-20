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

#ifndef VECTOR_H
#define VECTOR_H

#include "crt_abstractions.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

typedef void* VECTOR_HANDLE;

typedef bool(*PREDICATE_FUNCTION)(const void* element, const void* value);

/* creation */
extern VECTOR_HANDLE VECTOR_create(size_t elementSize);
extern void VECTOR_destroy(VECTOR_HANDLE handle);

/* insertion */
extern int VECTOR_push_back(VECTOR_HANDLE handle, const void* elements, size_t numElements);

/* removal */
extern void VECTOR_erase(VECTOR_HANDLE handle, void* elements, size_t numElements);
extern void VECTOR_clear(VECTOR_HANDLE handle);

/* access */
extern void* VECTOR_element(const VECTOR_HANDLE handle, size_t index);
extern void* VECTOR_front(const VECTOR_HANDLE handle);
extern void* VECTOR_back(const VECTOR_HANDLE handle);
extern void* VECTOR_find_if(const VECTOR_HANDLE handle, PREDICATE_FUNCTION pred, const void* value);

/* capacity */
extern size_t VECTOR_size(const VECTOR_HANDLE handle);

#ifdef __cplusplus
}
#else
#endif

#endif /* VECTOR_H */
