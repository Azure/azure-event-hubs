// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Producer
{
    using System;

    internal sealed class TransactionData
    {
        public string CreditCardId { get; set; } // like a credit card no. just simpler to use a Guid for our sample than a 16 digit no!

        public int Amount { get; set; } // simplified to int instead of double for now

        public string Location { get; set; }

        public DateTimeOffset Timestamp { get; set; }
    }
}

