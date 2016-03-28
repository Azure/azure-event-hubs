// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp;

    class AmqpPartitionReceiver : PartitionReceiver
    {
        readonly AmqpEventHubClient eventHubClient;
        readonly string consumerGroupName;
        readonly DateTime? dateTime;
        readonly long epoch;
        readonly bool isEpochReceiver;
        readonly bool offsetInclusive;
        readonly string partitionId;
        readonly string startingOffset;

        public AmqpPartitionReceiver(
            AmqpEventHubClient eventHubClient,
            string consumerGroupName,
            string partitionId,
            string startingOffset,
            bool offsetInclusive,
            DateTime? dateTime,
            long epoch,
            bool isEpochReceiver)
            : base(eventHubClient, consumerGroupName, partitionId, startingOffset, offsetInclusive, dateTime, epoch, isEpochReceiver)
        {
            this.eventHubClient = eventHubClient;
            this.consumerGroupName = consumerGroupName;
            this.partitionId = partitionId;
            this.startingOffset = startingOffset;
            this.offsetInclusive = offsetInclusive;
            this.dateTime = dateTime;
            this.epoch = epoch;
            this.isEpochReceiver = isEpochReceiver;
        }

        public override Task CloseAsync()
        {
            throw new NotImplementedException("TODO: Close AMQP Link here.");
        }

        protected override Task<IEnumerable<EventData>> OnReceiveAsync()
        {
            throw new NotImplementedException("TODO: Implement OnReceiveAsync.");
        }

        protected override void OnSetReceiveHandler(IPartitionReceiveHandler receiveHandler)
        {
            throw new NotImplementedException("TODO: Implement OnSetReceiveHandler.");
        }
    }
}
