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

#ifndef DOUBLYLINKEDLIST_H
#define DOUBLYLINKEDLIST_H

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

typedef struct DLIST_ENTRY_TAG
{
    struct DLIST_ENTRY_TAG *Flink;
    struct DLIST_ENTRY_TAG *Blink;
} DLIST_ENTRY, *PDLIST_ENTRY;

extern void DList_InitializeListHead(PDLIST_ENTRY listHead);
extern int DList_IsListEmpty(const PDLIST_ENTRY listHead);
extern void DList_InsertTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
extern void DList_InsertHeadList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
extern void DList_AppendTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY ListToAppend);
extern int DList_RemoveEntryList(PDLIST_ENTRY listEntry);
extern PDLIST_ENTRY DList_RemoveHeadList(PDLIST_ENTRY listHead);

//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//
#define containingRecord(address, type, field) ((type *)((uintptr_t)(address) - offsetof(type,field)))

#ifdef __cplusplus
}
#else
#endif

#endif /* DOUBLYLINKEDLIST_H */
