// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;

    public sealed class MessagingEntityNotFoundException : EventHubsException
    {
        public MessagingEntityNotFoundException(string message)
            : base(false, message, null)
        {
        }
    }
}
