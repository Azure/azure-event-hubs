// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "version.h"

#define TOSTRING_(x) #x
#define TOSTRING(x) TOSTRING_(x)

#ifndef EHC_VERSION
#define EHC_VERSION unknown
#endif

const char* EventHubClient_GetVersionString(void)
{
    /* Codes_SRS_EVENTHUBCLIENT_05_003: [EventHubClient_GetVersionString shall return a pointer to a constant string which indicates the version of EventHubClient API.] */
    return TOSTRING(EHC_VERSION);
}
