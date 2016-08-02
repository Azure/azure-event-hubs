// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;

    public class LeaseLostException : Exception
    {
        readonly Lease lease;

        internal LeaseLostException(Lease lease, Exception innerException)
            : base(string.Empty, innerException)
        {
            if (lease == null)
            {
                throw new ArgumentNullException(nameof(lease));
            }

            this.lease = lease;
        }

        // We don't want to expose Lease to the public.
        public string PartitionId
        {
            get { return this.lease.PartitionId; }
        }
    }
}