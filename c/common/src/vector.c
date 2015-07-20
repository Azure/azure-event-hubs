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
#include "gballoc.h"

#include "vector.h"
#include <string.h>

typedef struct VECTOR_TAG
{
    void* storage;
    size_t count;
    size_t elementSize;
} VECTOR;

VECTOR_HANDLE VECTOR_create(size_t elementSize)
{
    VECTOR_HANDLE result;

    VECTOR* vec = (VECTOR*)malloc(sizeof(VECTOR));
    if (vec == NULL)
    {
        result = NULL;
    }
    else
    {
        vec->storage = NULL;
        vec->count = 0;
        vec->elementSize = elementSize;
        result = (VECTOR_HANDLE)vec;
    }
    return result;
}

void VECTOR_destroy(VECTOR_HANDLE handle)
{
    if (handle != NULL)
    {
        VECTOR_clear(handle);
        VECTOR* vec = (VECTOR*)handle;
        free(vec);
    }
}

/* insertion */
int VECTOR_push_back(VECTOR_HANDLE handle, const void* elements, size_t numElements)
{
    int result;
    if (handle == NULL || elements == NULL || numElements == 0)
    {
        result = __LINE__;
    }
    else
    {
        VECTOR* vec = (VECTOR*)handle;
        const size_t curSize = vec->elementSize * vec->count;
        const size_t appendSize = vec->elementSize * numElements;

        void* temp = realloc(vec->storage, curSize + appendSize);
        if (temp == NULL)
        {
            result = __LINE__;
        }
        else
        {
            memcpy((unsigned char*)temp + curSize, elements, appendSize);
            vec->storage = temp;
            vec->count += numElements;
            result = 0;
        }
    }
    return result;
}

/* removal */
void VECTOR_erase(VECTOR_HANDLE handle, void* elements, size_t numElements)
{
    if (handle != NULL && elements != NULL && numElements > 0)
    {
        VECTOR* vec = (VECTOR*)handle;
        unsigned char* src = (unsigned char*)elements + (vec->elementSize * numElements);
        unsigned char* srcEnd = (unsigned char*)vec->storage + (vec->elementSize * vec->count);
        (void)memmove(elements, src, srcEnd - src);
        vec->count -= numElements;
        if (vec->count == 0)
        {
            free(vec->storage);
            vec->storage = NULL;
        }
        else
        {
            vec->storage = realloc(vec->storage, (vec->elementSize * vec->count));
        }
    }
}

void VECTOR_clear(VECTOR_HANDLE handle)
{
    if (handle != NULL)
    {
        VECTOR* vec = (VECTOR*)handle;
        if (vec->storage != NULL)
        {
            free(vec->storage);
            vec->storage = NULL;
        }
        vec->count = 0;
    }
}

/* access */

void* VECTOR_element(const VECTOR_HANDLE handle, size_t index)
{
    void* result = NULL;
    if (handle != NULL)
    {
        const VECTOR* vec = (const VECTOR*)handle;
        if (index <= vec->count)
        {
            result = (unsigned char*)vec->storage + (vec->elementSize * index);
        }
    }
    return result;
}

void* VECTOR_front(const VECTOR_HANDLE handle)
{
    void* result = NULL;
    if (handle != NULL)
    {
        const VECTOR* vec = (const VECTOR*)handle;
        result = vec->storage;
    }
    return result;
}

void* VECTOR_back(const VECTOR_HANDLE handle)
{
    void* result = NULL;
    if (handle != NULL)
    {
        const VECTOR* vec = (const VECTOR*)handle;
        result = (unsigned char*)vec->storage + (vec->elementSize * (vec->count - 1));
    }
    return result;
}

void* VECTOR_find_if(const VECTOR_HANDLE handle, PREDICATE_FUNCTION pred, const void* value)
{
    void* result = NULL;
    size_t i;

    if (handle != NULL && pred != NULL && value != NULL)
    {
        for (i = 0; i < VECTOR_size(handle); ++i)
        {
            void* elem = VECTOR_element(handle, i);
            if (!!pred(elem, value))
            {
                result = elem;
                break;
            }
        }
    }
    return result;
}

/* capacity */

size_t VECTOR_size(const VECTOR_HANDLE handle)
{
    size_t result = 0;
    if (handle != NULL)
    {
        const VECTOR* vec = (const VECTOR*)handle;
        result = vec->count;
    }
    return result;
}
