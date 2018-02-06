// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Producer
{
    /// <summary>
    /// A mechanism to identify the mock transactions that are generated on the client side.
    /// This is not used downstream. But is used more for an easy (or visual) comparison of
    /// anomalous data that the downstream analytics produces.
    /// </summary>
    internal enum TransactionType
    {
        Regular,
        Suspect,
    }
}

