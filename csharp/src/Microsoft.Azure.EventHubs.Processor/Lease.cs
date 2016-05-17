// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using Newtonsoft.Json;

    public class Lease
    {
        protected Lease()
        {
        }

        protected Lease(string partitionId)
        {
            this.PartitionId = partitionId;
            this.Owner = string.Empty;
            this.Token = string.Empty;
        }

        protected Lease(Lease source)
        {
            this.PartitionId = source.PartitionId;
            this.Epoch = source.Epoch;
            this.Owner = source.Owner;
            this.Token = source.Token;
        }

        public string PartitionId { get; set; }

        public string Owner { get; set; }

        public string Token { get; set; }

        public long Epoch { get; set; }

        public virtual bool IsExpired() 
        {
    	    // this function is meaningless in the base class
    	    return false;
        }

        public virtual string GetStateDebug()
        {
            return "N/A";
        }

        internal long IncrementEpoch()
        {
            this.Epoch++;
            return this.Epoch;
        }
    }
}