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

/*defines*/
#ifndef HYPOTHETIC_MODULE_INTERFACE_H
#define HYPOTHETIC_MODULE_INTERFACE_H

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/*types*/
typedef void(*pVoidFunction)(void);
typedef char* pChar;

/*variable exports*/
/*function exports*/
extern void whenzero(void);
extern int whenizero(void);
extern int whenone  (_In_ int i);
extern int whentwo  (_In_z_ pChar s, _In_ int i);
extern int whenthree(_In_ char c, _In_z_ pChar s, _In_ int i);
extern int whenfour (_In_ unsigned short int si, _In_ char c, _In_z_ pChar s, _In_ int i);
extern int whenfive (_In_opt_ pVoidFunction pVoid, _In_ unsigned short int si, _In_ char c, _In_z_ pChar s, _In_ int i);
extern int whensix  (_In_ char c1, _In_ char c2, _In_ char c3, _In_ char c4, _In_ char c5, _In_ char c6);

#ifdef __cplusplus
}
#endif

#endif
