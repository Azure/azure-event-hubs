// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    class AmqpPartitionReceiver : PartitionReceiver
    {
        readonly object receivePumpLock;
        CancellationTokenSource receivePumpCancellationSource;
        Task receivePumpTask;

        public AmqpPartitionReceiver(
            AmqpEventHubClient eventHubClient,
            string consumerGroupName,
            string partitionId,
            string startOffset,
            bool offsetInclusive,
            DateTime? startTime,
            long? epoch)
            : base(eventHubClient, consumerGroupName, partitionId, startOffset, offsetInclusive, startTime, epoch)
        {
            string entityPath = eventHubClient.ConnectionSettings.EntityPath;
            this.Path = $"{entityPath}/ConsumerGroups/{consumerGroupName}/Partitions/{partitionId}";
            this.ReceiveLinkManager = new FaultTolerantAmqpObject<ReceivingAmqpLink>(this.CreateLinkAsync, this.CloseSession);
            this.receivePumpLock = new object();
        }

        string Path { get; }

        FaultTolerantAmqpObject<ReceivingAmqpLink> ReceiveLinkManager { get; }

        protected override async Task OnCloseAsync()
        {
            Task localReceivePumpTask;
            CancellationTokenSource localReceivePumpCancellationSource;
            lock (this.receivePumpLock)
            {
                localReceivePumpTask = this.receivePumpTask;
                localReceivePumpCancellationSource = this.receivePumpCancellationSource;
                this.receivePumpTask = null;
                this.receivePumpCancellationSource = null;
            }

            if (localReceivePumpTask != null)
            {
                localReceivePumpCancellationSource.Cancel();
                await localReceivePumpTask;
                localReceivePumpCancellationSource.Dispose();
            }

            await this.ReceiveLinkManager.CloseAsync();
        }

        protected override async Task<IList<EventData>> OnReceiveAsync()
        {
            var timeoutHelper = new TimeoutHelper(this.EventHubClient.ConnectionSettings.OperationTimeout, true);
            ReceivingAmqpLink receiveLink = await this.ReceiveLinkManager.GetOrCreateAsync(timeoutHelper.RemainingTime());
            IEnumerable<AmqpMessage> amqpMessages = null;
            bool hasMessages = await Task.Factory.FromAsync(
                (c, s) => receiveLink.BeginReceiveMessages(this.PrefetchCount, timeoutHelper.RemainingTime(), c, s),
                (a) => receiveLink.EndReceiveMessages(a, out amqpMessages),
                this);

            if (hasMessages && amqpMessages != null)
            {
                IList<EventData> eventDatas = null;
                foreach (var amqpMessage in amqpMessages)
                {
                    if (eventDatas == null)
                    {
                        eventDatas = new List<EventData>();
                    }

                    receiveLink.DisposeDelivery(amqpMessage, true, AmqpConstants.AcceptedOutcome);
                    eventDatas.Add(AmqpMessageConverter.AmqpMessageToEventData(amqpMessage));
                }

                return eventDatas;
            }
            else
            {
                return null;
            }
        }

        protected override void OnSetReceiveHandler(IPartitionReceiveHandler receiveHandler)
        {
            lock (this.receivePumpLock)
            {
                if (this.receivePumpTask != null)
                {
                    // Shutdown previously running pump.
                    Fx.Assert(this.receivePumpCancellationSource != null, $"{nameof(receivePumpCancellationSource)} and {nameof(receivePumpTask)} must be set together!");
                    this.receivePumpCancellationSource.Cancel();
                    this.receivePumpTask.Wait(this.EventHubClient.ConnectionSettings.OperationTimeout);

                    this.receivePumpCancellationSource.Dispose();
                    this.receivePumpCancellationSource = null;
                    this.receivePumpTask = null;
                }

                if (receiveHandler != null)
                {
                    this.receivePumpCancellationSource = new CancellationTokenSource();
                    this.receivePumpTask = this.ReceivePumpAsync(receiveHandler, this.receivePumpCancellationSource.Token);
                }
            }
        }

        async Task<ReceivingAmqpLink> CreateLinkAsync(TimeSpan timeout)
        {
            var amqpEventHubClient = ((AmqpEventHubClient)this.EventHubClient);
            var connectionSettings = this.EventHubClient.ConnectionSettings;
            var timeoutHelper = new TimeoutHelper(connectionSettings.OperationTimeout);
            AmqpConnection connection = await amqpEventHubClient.ConnectionManager.GetOrCreateAsync(timeoutHelper.RemainingTime());

            // Authenticate over CBS
            var cbsLink = connection.Extensions.Find<AmqpCbsLink>();

            ICbsTokenProvider cbsTokenProvider = amqpEventHubClient.CbsTokenProvider;
            Uri address = new Uri(connectionSettings.Endpoint, this.Path);
            string audience = address.AbsoluteUri;
            string resource = address.AbsoluteUri;
            var expiresAt = await cbsLink.SendTokenAsync(cbsTokenProvider, address, audience, resource, new[] { ClaimConstants.Listen }, timeoutHelper.RemainingTime());

            AmqpSession session = null;
            try
            {
                // Create our Session
                var sessionSettings = new AmqpSessionSettings { Properties = new Fields() };
                session = connection.CreateSession(sessionSettings);
                await session.OpenAsync(timeoutHelper.RemainingTime());

                FilterSet filterMap = null;
                var filters = this.CreateFilters();
                if (filters != null && filters.Count > 0)
                {
                    filterMap = new FilterSet();
                    foreach (var filter in filters)
                    {
                        filterMap.Add(filter.DescriptorName, filter);
                    }
                }

                // Create our Link
                var linkSettings = new AmqpLinkSettings();
                linkSettings.Role = true;
                linkSettings.TotalLinkCredit = (uint)this.PrefetchCount;
                linkSettings.AutoSendFlow = this.PrefetchCount > 0;
                linkSettings.AddProperty(AmqpClientConstants.EntityTypeName, (int)MessagingEntityType.ConsumerGroup);
                linkSettings.Source = new Source { Address = address.AbsolutePath, FilterSet = filterMap };
                linkSettings.Target = new Target { Address = this.ClientId };
                linkSettings.SettleType = SettleMode.SettleOnSend;

                if (this.Epoch.HasValue)
                {
                    linkSettings.AddProperty(AmqpClientConstants.AttachEpoch, this.Epoch.Value);
                }

                var link = new ReceivingAmqpLink(linkSettings);
                linkSettings.LinkName = $"{amqpEventHubClient.ContainerId};{connection.Identifier}:{session.Identifier}:{link.Identifier}";
                link.AttachTo(session);

                await link.OpenAsync(timeoutHelper.RemainingTime());
                return link;
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                // Cleanup any session (and thus link) in case of exception.
                session?.Abort();
                throw;
            }
        }

        void CloseSession(ReceivingAmqpLink link)
        {
            link.Session.SafeClose();
        }

        IList<AmqpDescribed> CreateFilters()
        {
            if (string.IsNullOrWhiteSpace(this.StartOffset) && !this.StartTime.HasValue)
            {
                return null;
            }

            List<AmqpDescribed> filterMap = new List<AmqpDescribed>();
            if (!string.IsNullOrWhiteSpace(this.StartOffset) || this.StartTime.HasValue)
            {
                // In the case of DateTime, we want to be amqp-compliant so 
                // we should transmit the DateTime in a amqp-timestamp format,
                // which is defined as "64-bit two's-complement integer representing milliseconds since the unix epoch"
                // ref: http://docs.oasis-open.org/amqp/core/v1.0/amqp-core-complete-v1.0.pdf
                string sqlExpression = !string.IsNullOrWhiteSpace(this.StartOffset) ?
                    this.OffsetInclusive ?
                        string.Format(CultureInfo.InvariantCulture, AmqpClientConstants.FilterInclusiveOffsetFormatString, this.StartOffset) :
                        string.Format(CultureInfo.InvariantCulture, AmqpClientConstants.FilterOffsetFormatString, this.StartOffset) :
                    string.Format(CultureInfo.InvariantCulture, AmqpClientConstants.FilterReceivedAtFormatString, TimeStampEncodingGetMilliseconds(this.StartTime.Value));
                filterMap.Add(new AmqpSelectorFilter(sqlExpression));
            }

            return filterMap;
        }

        // This is equivalent to Microsoft.Azure.Amqp's internal API TimeStampEncoding.GetMilliseconds
        static long TimeStampEncodingGetMilliseconds(DateTime value)
        {
            DateTime utcValue = value.ToUniversalTime();
            double millisecs = (utcValue - AmqpConstants.StartOfEpoch).TotalMilliseconds;
            return (long)millisecs;
        }

        async Task ReceivePumpAsync(IPartitionReceiveHandler receiveHandler, CancellationToken cancellationToken)
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                IEnumerable<EventData> receivedEvents = null;

                try
                {
                    receivedEvents = await this.ReceiveAsync();
                }
                catch (Exception e) // when (e is InterruptedException || e is ExecutionException || e is TimeoutException)
                {
                    ServiceBusException serviceBusException = e as ServiceBusException;
                    if (serviceBusException != null && serviceBusException.IsTransient)
                    {
                        try
                        {
                            await receiveHandler.ProcessErrorAsync(e);
                            continue;
                        }
                        catch (Exception userCodeError)
                        {
                            await receiveHandler.CloseAsync(userCodeError);
                            return;
                        }
                    }
					else
					{
                        await receiveHandler.CloseAsync(e);
                        return;
                    }
                }

                try
                {
                    await receiveHandler.ProcessEventsAsync(receivedEvents);
                }
                catch (Exception userCodeError)
                {
                    await receiveHandler.CloseAsync(userCodeError);
                    return;
                }
            }

            // Shutting down gracefully
            await receiveHandler.CloseAsync(null);
        }
    }
}
