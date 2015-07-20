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

#ifndef LOGGING_H
#define LOGGING_H
#include <stdio.h>
#include <time.h>

#define STRINGIFY(a) (#a)

#define LogUsage (void)printf

#if defined _WIN32_WCE && DEBUG
	#define LogInfo(...) (void)printf("Info: " __VA_ARGS__)
#else
    #define LogInfo(...) (void)printf("Info: " __VA_ARGS__)
#endif 

#if ((defined _WIN32_WCE) && _WIN32_WCE==0x0600)
    #define LogError(FORMAT, ...) { (void)printf("Error: File:%s Func:%s Line:%d " FORMAT, __FILE__, __FUNCDNAME__, __LINE__, __VA_ARGS__); }
#elif defined _WIN32_WCE && DEBUG
    #define LogError(FORMAT, ...) { time_t t = time(NULL); (void)printf("Error: Time:%.24s File:%s Func:%s Line:%d " FORMAT, ctime(&t), __FILE__, __FUNCDNAME__, __LINE__, __VA_ARGS__); }
#elif defined _MSC_VER
#define LogError(FORMAT, ...) { time_t t = time(NULL); (void)fprintf(stderr,"Error: Time:%.24s File:%s Func:%s Line:%d " FORMAT, ctime(&t), __FILE__, __FUNCDNAME__, __LINE__, __VA_ARGS__); }
#else
#define LogError(FORMAT, ...) { time_t t = time(NULL); (void)fprintf(stderr,"Error: Time:%.24s File:%s Func:%s Line:%d " FORMAT, ctime(&t), __FILE__, __func__, __LINE__, ##__VA_ARGS__); }
#endif

#endif /* LOGGING_H */
