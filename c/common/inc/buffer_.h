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

#ifndef BUFFER_H
#define BUFFER_H

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

typedef void* BUFFER_HANDLE;

extern BUFFER_HANDLE BUFFER_new(void);
extern void BUFFER_delete(BUFFER_HANDLE handle);
extern int BUFFER_pre_build(BUFFER_HANDLE handle, size_t size);
extern int BUFFER_build(BUFFER_HANDLE handle, const unsigned char* source, size_t size);
extern int BUFFER_unbuild(BUFFER_HANDLE handle);
extern int BUFFER_enlarge(BUFFER_HANDLE handle, size_t enlargeSize);
extern int BUFFER_content(BUFFER_HANDLE handle, const unsigned char** content);
extern int BUFFER_size(BUFFER_HANDLE handle, size_t* size);
extern int BUFFER_append(BUFFER_HANDLE handle1, BUFFER_HANDLE handle2);
extern unsigned char* BUFFER_u_char(BUFFER_HANDLE handle);
extern size_t BUFFER_length(BUFFER_HANDLE handle);
extern BUFFER_HANDLE BUFFER_clone(BUFFER_HANDLE handle);

#ifdef __cplusplus
}
#endif


#endif  /* BUFFER_H */
