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

#ifndef STRINGS_H
#define STRINGS_H

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

typedef void* STRING_HANDLE;

extern STRING_HANDLE STRING_new(void);
extern STRING_HANDLE STRING_clone(STRING_HANDLE handle);
extern STRING_HANDLE STRING_construct(const char* psz);
extern STRING_HANDLE STRING_construct_n(const char* psz, size_t n);
extern STRING_HANDLE STRING_new_with_memory(const char* memory);
extern STRING_HANDLE STRING_new_quoted(const char*);
extern void STRING_delete(STRING_HANDLE handle);
extern int STRING_concat(STRING_HANDLE handle, const char* s2);
extern int STRING_concat_with_STRING(STRING_HANDLE s1, STRING_HANDLE s2);
extern int STRING_quote(STRING_HANDLE handle);
extern int STRING_copy(STRING_HANDLE s1, const char* s2);
extern int STRING_copy_n(STRING_HANDLE s1, const char* s2, size_t n);
extern const char* STRING_c_str(STRING_HANDLE handle);
extern int STRING_empty(STRING_HANDLE handle);
extern size_t STRING_length(STRING_HANDLE handle);
extern int STRING_compare(STRING_HANDLE s1, STRING_HANDLE s2);

#ifdef __cplusplus
}
#else
#endif

#endif  /*STRINGS_H*/
