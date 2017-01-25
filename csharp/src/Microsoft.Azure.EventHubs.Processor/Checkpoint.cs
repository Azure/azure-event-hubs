// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;

    public class Checkpoint
    {       
        public Checkpoint(string partitionId)
            : this(partitionId, PartitionReceiver.StartOfStream, 0)
        {
        }

        public Checkpoint(string partitionId, string offset, long sequenceNumber)
        {
            this.PartitionId = partitionId;
            this.Offset = offset;
            this.SequenceNumber = sequenceNumber;
        }

        public Checkpoint(Checkpoint source)
        {
            this.PartitionId = source.PartitionId;
            this.Offset = source.Offset;
            this.SequenceNumber = source.SequenceNumber;
        }

        public string Offset { get; set; }

        public long SequenceNumber { get; set; }

        public string PartitionId { get; }
    }
}