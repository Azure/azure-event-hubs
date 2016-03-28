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
        public EventSender()
            : base(nameof(EventSender) + StringUtility.GetRandomString())
        {
        }

        public async Task SendAsync(IEnumerable<EventData> eventDatas, string partitionKey)
        {
            EventHubsEventSource.Log.EventSendStart(eventDatas.Count(), partitionKey);
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
    }
}