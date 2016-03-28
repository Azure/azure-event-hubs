// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;

    public class AmqpEventHubClient : EventHubClient
    {
        public AmqpEventHubClient(ConnectionStringBuilder connectionStringBuilder)
            : base(connectionStringBuilder)
        {
        }

        internal override EventSender OnCreateEventSender(string partitionId)
        {
            return new AmqpEventSender(this, partitionId);
        }

        protected override PartitionReceiver OnCreateReceiver(string consumerGroupName, string partitionId, string startingOffset, bool offsetInclusive, DateTime? dateTime, long epoch, bool isEpochReceiver)
        {
            return new AmqpPartitionReceiver(
                this, consumerGroupName, partitionId, startingOffset, offsetInclusive, dateTime, epoch, isEpochReceiver);
        }
    }
}