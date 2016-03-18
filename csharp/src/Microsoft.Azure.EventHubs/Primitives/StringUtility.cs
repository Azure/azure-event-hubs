// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;

    static class StringUtility
    {
        public static string GetRandomString()
        {
            return Guid.NewGuid().ToString().Substring(0, 6);
        }
    }
}
