// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;

    static class ExceptionUtility
    {        
        public static ArgumentException Argument(string paramName, string message)
        {
            return new ArgumentException(message, paramName);
        }

        public static Exception ArgumentNull(string paramName)
        {
            return new ArgumentNullException(paramName);
        }

        public static ArgumentException ArgumentNullOrWhiteSpace(string paramName)
        {
            return Argument(paramName, Resources.ArgumentNullOrWhiteSpace.FormatForUser(paramName));
        }
    }
}
