// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "version.h"

const char* EventHubClient_GetVersionString(void)
{
    /* Codes_SRS_EVENTHUBCLIENT_05_003: [EventHubClient_GetVersionString shall return a pointer to a constant string which indicates the version of EventHubClient API.] */
    return EVENT_HUB_SDK_VERSION;
}
