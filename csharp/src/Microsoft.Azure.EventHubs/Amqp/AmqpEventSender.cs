// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp;

    class AmqpEventSender : EventSender
    {
        readonly string path;
        readonly string partitionId;

        public AmqpEventSender(AmqpEventHubClient eventHubClient, string partitionId)
        {
            this.partitionId = partitionId;
            if (!string.IsNullOrEmpty(partitionId))
            {
                this.path = $"{eventHubClient.EventHubName}/Partitions/{partitionId}";
            }
            else
            {
                this.path = eventHubClient.EventHubName;
            }
        }

        public override Task CloseAsync()
        {
            throw new NotImplementedException("TODO: Close the AMQP Link here.");
        }

        protected override async Task OnSendAsync(IEnumerable<EventData> eventDatas, string partitionKey)
        {
            AmqpMessage amqpMessage = EventDatasToAmqpMessage(eventDatas, partitionKey, true);

            throw new NotImplementedException("TODO: Get an AMQP Link and send the amqpMessage here.");
        }

        static AmqpMessage EventDatasToAmqpMessage(IEnumerable<EventData> eventDatas, string partitionKey, bool batchable)
        {
            throw new NotImplementedException("TODO: Serialize batch of EventData here.");
        }
    }
}
