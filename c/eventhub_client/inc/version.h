// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef EVENTHUBVERSION_H
#define EVENTHUBVERSION_H

#include "azure_c_shared_utility/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MOCKABLE_FUNCTION(, const char*, EventHubClient_GetVersionString);

#ifdef __cplusplus
}
#endif

#endif // EVENTHUBVERSION_H
