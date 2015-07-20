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

#ifndef GBALLOC_H
#define GBALLOC_H


#ifdef __cplusplus
#include <cstdlib>
extern "C"
{
#else
#include <stdlib.h>
#endif
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

/* all translation units that need memory measurement need to have GB_MEASURE_MEMORY_FOR_THIS defined */
/* GB_DEBUG_ALLOC is the switch that turns the measurement on/off, so that it is not on always */
#if defined(GB_DEBUG_ALLOC)

extern int gballoc_init(void);
extern void gballoc_deinit(void);
extern void* gballoc_malloc(size_t size);
extern void* gballoc_calloc(size_t nmemb, size_t size);
extern void* gballoc_realloc(void* ptr, size_t size);
extern void gballoc_free(void* ptr);

extern size_t gballoc_getMaximumMemoryUsed(void);
extern size_t gballoc_getCurrentMemoryUsed(void);

/* if GB_MEASURE_MEMORY_FOR_THIS is defined then we want to redirect memory allocation functions to gballoc_xxx functions */
#ifdef GB_MEASURE_MEMORY_FOR_THIS
#ifdef _CRTDBG_MAP_ALLOC
#undef _malloc_dbg
#undef _calloc_dbg
#undef _realloc_dbg
#undef _free_dbg
#define _malloc_dbg(size, ...) gballoc_malloc(size)
#define _calloc_dbg(nmemb, size, ...) gballoc_calloc(nmemb, size)
#define _realloc_dbg(ptr, size, ...) gballoc_realloc(ptr, size)
#define _free_dbg(ptr, ...) gballoc_free(ptr)
#else
#define malloc gballoc_malloc
#define calloc gballoc_calloc
#define realloc gballoc_realloc
#define free gballoc_free
#endif
#endif

#else /* GB_DEBUG_ALLOC */

#define gballoc_init() 0
#define gballoc_deinit() ((void)0)

#define gballoc_getMaximumMemoryUsed() SIZE_MAX
#define gballoc_getCurrentMemoryUsed() SIZE_MAX

#endif /* GB_DEBUG_ALLOC */

#ifdef __cplusplus
}
#endif

#endif /* GBALLOC_H */
