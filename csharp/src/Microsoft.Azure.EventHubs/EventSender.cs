// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Linq;
    using System.Threading.Tasks;

    abstract class EventSender : ClientEntity
    {
        protected EventSender(string partitionId)
            : base(nameof(EventSender) + StringUtility.GetRandomString())
        {
            this.PartitionId = partitionId;
        }

        protected string PartitionId { get; }

        public async Task SendAsync(IEnumerable<EventData> eventDatas, string partitionKey)
        {
            int count = this.ValidateEvents(eventDatas, partitionKey);
            EventHubsEventSource.Log.EventSendStart(count, partitionKey);
            try
            {
                await this.OnSendAsync(eventDatas, partitionKey);
            }
            catch (Exception exception)
            {
                EventHubsEventSource.Log.EventSendException(exception.ToString());
                throw;
            }
            finally
            {
                EventHubsEventSource.Log.EventSendStop();
            }
        }

        protected abstract Task OnSendAsync(IEnumerable<EventData> eventDatas, string partitionKey);

        int ValidateEvents(IEnumerable<EventData> eventDatas, string partitionKey)
        {
            int count;
            if (eventDatas == null || (count = eventDatas.Count()) == 0)
            {
                throw Fx.Exception.Argument(nameof(eventDatas), Resources.EventDataListIsNullOrEmpty);
            }
            else if (this.PartitionId != null && partitionKey != null)
            {
                throw Fx.Exception.Argument(nameof(partitionKey), Resources.PartitionInvalidPartitionKey.FormatForUser(partitionKey, this.PartitionId));
            }

            return count;
        }
    }
}