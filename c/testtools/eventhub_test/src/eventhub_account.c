/*
Microsoft Azure IoT Device Libraries

Copyright (c) Microsoft Corporation
All rights reserved. 

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the Software), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stdlib.h>
#include "eventhub_account.h"
#include "iot_logging.h"

#define MAX_PARTITION_SIZE      16

const char* EventHubAccount_GetConnectionString(void)
{
    const char* envVar = getenv("EVENTHUB_CONNECTION_STRING");
    if (envVar == NULL)
    {
        LogError("Failed: EVENTHUB_CONNECTION_STRING is NULL\r\n");
    }
    return envVar;
}

const char* EventHubAccount_GetName(void)
{
    const char* envVar = getenv("EVENTHUB_NAME");
    if (envVar == NULL)
    {
        LogError("Failed: EVENTHUB_NAME is NULL\r\n");
    }
    return envVar;
}

int EventHubAccount_PartitionCount(void)
{
    int nPartitionCount; 
    char* envVar = getenv("EVENTHUB_PARTITION_COUNT"); 
    if (envVar == NULL) 
    { 
        LogInfo("Warning: EVENTHUB_PARTITION_COUNT is NULL using value of %d\r\n", MAX_PARTITION_SIZE);
        nPartitionCount = MAX_PARTITION_SIZE; 
    } 
    else 
    { 
        nPartitionCount = atoi(envVar); 
    } 
    return nPartitionCount; 
}