// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Producer
{
    internal sealed class Transaction
    {
        public TransactionData Data { get; set; }

        public TransactionType Type { get; set; }
    }
}

