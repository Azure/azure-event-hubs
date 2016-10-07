// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;

    abstract class EventDataSender : ClientEntity
    {
        protected EventDataSender(EventHubClient eventHubClient, string partitionId)
            : base(nameof(EventDataSender) + StringUtility.GetRandomString())
        {
            this.EventHubClient = eventHubClient;
            this.PartitionId = partitionId;
            this.retryPolicy = eventHubClient.ConnectionSettings.RetryPolicy;
            this.retryPolicy.ResetRetryCount(this.ClientId);
        }

        protected EventHubClient EventHubClient { get; }

        protected string PartitionId { get; }

        public Task SendAsync(IEnumerable<EventData> eventDatas, string partitionKey)
        {
            return this.OnSendAsync(eventDatas, partitionKey);
        }

        protected abstract Task OnSendAsync(IEnumerable<EventData> eventDatas, string partitionKey);

        internal static int ValidateEvents(IEnumerable<EventData> eventDatas, string partitionId, string partitionKey)
        {
            int count;
            if (eventDatas == null || (count = eventDatas.Count()) == 0)
            {
                throw Fx.Exception.Argument(nameof(eventDatas), Resources.EventDataListIsNullOrEmpty);
            }
            else if (partitionId != null && partitionKey != null)
            {
                throw Fx.Exception.Argument(nameof(partitionKey), Resources.PartitionInvalidPartitionKey.FormatForUser(partitionKey, partitionId));
            }

            return count;
        }
    }
}