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

// guids.h: definitions of GUIDs/IIDs/CLSIDs used in this VsPackage

/*
Do not use #pragma once, as this file needs to be included twice.  Once to declare the externs
for the GUIDs, and again right after including initguid.h to actually define the GUIDs.
*/



// package guid
// { ec625337-5c7b-4d7c-b472-98a4e4b1a93d }
#define guidIotClientUnitTestsTemplatePkg { 0xEC625337, 0x5C7B, 0x4D7C, { 0xB4, 0x72, 0x98, 0xA4, 0xE4, 0xB1, 0xA9, 0x3D } }
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_IotClientUnitTestsTemplate,
0xEC625337, 0x5C7B, 0x4D7C, 0xB4, 0x72, 0x98, 0xA4, 0xE4, 0xB1, 0xA9, 0x3D );
#endif

// Command set guid for our commands (used with IOleCommandTarget)
// { 1fd266ea-d9fd-41df-8fb1-9d5264596356 }
#define guidIotClientUnitTestsTemplateCmdSet { 0x1FD266EA, 0xD9FD, 0x41DF, { 0x8F, 0xB1, 0x9D, 0x52, 0x64, 0x59, 0x63, 0x56 } }
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_IotClientUnitTestsTemplateCmdSet, 
0x1FD266EA, 0xD9FD, 0x41DF, 0x8F, 0xB1, 0x9D, 0x52, 0x64, 0x59, 0x63, 0x56 );
#endif

//Guid for the image list referenced in the VSCT file
// { 0b50c3c7-92cc-4ffd-b3ef-4b58a26e21fb }
#define guidImages { 0xB50C3C7, 0x92CC, 0x4FFD, { 0xB3, 0xEF, 0x4B, 0x58, 0xA2, 0x6E, 0x21, 0xFB } }
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_Images, 
0xB50C3C7, 0x92CC, 0x4FFD, 0xB3, 0xEF, 0x4B, 0x58, 0xA2, 0x6E, 0x21, 0xFB );
#endif


