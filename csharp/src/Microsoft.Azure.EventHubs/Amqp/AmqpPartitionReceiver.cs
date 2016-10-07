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
        readonly ActiveClientLinkManager clientLinkManager;
        IPartitionReceiveHandler receiveHandler;
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
            this.clientLinkManager = new ActiveClientLinkManager((AmqpEventHubClient)this.EventHubClient);
        }

        string Path { get; }

        FaultTolerantAmqpObject<ReceivingAmqpLink> ReceiveLinkManager { get; }

        protected override Task OnCloseAsync()
        {
            // Close any ReceiveHandler (this is safe if there is none) and the ReceiveLinkManager in parallel.
            this.ReceiveHandlerClose();
            this.clientLinkManager.Close();
            return this.ReceiveLinkManager.CloseAsync();
        }

        protected override async Task<IList<EventData>> OnReceiveAsync(int maxMessageCount, TimeSpan waitTime)
        {
            bool shouldRetry = false;

            var timeoutHelper = new TimeoutHelper(waitTime, true);

            do
            {
                shouldRetry = false;

                try
                {
                    try
                    {
                        ReceivingAmqpLink receiveLink = await this.ReceiveLinkManager.GetOrCreateAsync(timeoutHelper.RemainingTime());
                        IEnumerable<AmqpMessage> amqpMessages = null;
                        bool hasMessages = await Task.Factory.FromAsync(
                            (c, s) => receiveLink.BeginReceiveMessages(maxMessageCount, timeoutHelper.RemainingTime(), c, s),
                            (a) => receiveLink.EndReceiveMessages(a, out amqpMessages),
                            this);

                        if (receiveLink.TerminalException != null)
                        {
                            throw receiveLink.TerminalException;
                        }

                        this.retryPolicy.ResetRetryCount(this.ClientId);

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
                    }
                    catch (AmqpException amqpException)
                    {
                        throw AmqpExceptionHelper.ToMessagingContract(amqpException.Error);
                    }
                }
                catch (Exception ex)
                {
                    // Evaluate retry condition?
                    this.retryPolicy.IncrementRetryCount(this.ClientId);
                    TimeSpan? retryInterval = this.retryPolicy.GetNextRetryInterval(this.ClientId, ex, timeoutHelper.RemainingTime());
                    if (retryInterval != null)
                    {
                        await Task.Delay(retryInterval.Value);
                        shouldRetry = true;
                    }
                    else
                    {
                        throw;
                    }
                }
            } while (shouldRetry);

            // No messages to deliver.
            return null;
        }

        protected override void OnSetReceiveHandler(IPartitionReceiveHandler newReceiveHandler)
        {
            lock (this.receivePumpLock)
            {
                if (this.receiveHandler != null)
                {
                    // Notify existing handler first (but don't wait).
                    this.receiveHandler.ProcessErrorAsync(new OperationCanceledException("New handler has registered for this receiver.")); // .Fork();
                }

                this.receiveHandler = newReceiveHandler;
                if (this.receiveHandler != null)
                {
                    // We have a new receiveHandler, ensure pump is running.
                    if (this.receivePumpTask == null)
                    {
                        this.receivePumpCancellationSource = new CancellationTokenSource();
                        this.receivePumpTask = this.ReceivePumpAsync(this.receivePumpCancellationSource.Token);
                    }
                }
                else
                {
                    // We have no receiveHandler, ensure pump is shut down.
                    if (this.receivePumpTask != null)
                    {
                        this.receivePumpCancellationSource.Cancel();
                        this.receivePumpCancellationSource.Dispose();
                        this.receivePumpCancellationSource = null;
                        this.receivePumpTask = null;
                    }
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
                var activeClientLink = new ActiveClientLink(
                    link,
                    this.EventHubClient.ConnectionSettings.Endpoint.AbsoluteUri, // audience
                    this.EventHubClient.ConnectionSettings.Endpoint.AbsoluteUri, // endpointUri
                    new string[] { ClaimConstants.Listen },
                    true,
                    expiresAt);

                this.clientLinkManager.SetActiveLink(activeClientLink);

                return link;
            }
            catch
            {
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

        async Task ReceivePumpAsync(CancellationToken cancellationToken)
        {
            // Loop until pump is shutdown or an error is hit.
            while (!cancellationToken.IsCancellationRequested)
            {
                IEnumerable<EventData> receivedEvents;

                try
                {
                    int batchSize;
                    lock (this.receivePumpLock)
                    {
                        if (this.receiveHandler == null)
                        {
                            // Pump has been shutdown, nothing more to do.
                            return;
                        }
                        else
                        {
                            batchSize = receiveHandler.MaxBatchSize;
                        }
                    }

                    receivedEvents = await this.ReceiveAsync(batchSize);
                }
                catch (Exception e)
                {
                    await this.ReceiveHandlerProcessErrorAsync(e);
                    break;
                }

                try
                {
                    await this.ReceiveHandlerProcessEventsAsync(receivedEvents);
                }
                catch (Exception userCodeError)
                {
                    await this.ReceiveHandlerProcessErrorAsync(userCodeError);
                    break;
                }
            }

            this.ReceiveHandlerClose();
        }

        // Encapsulates taking the receivePumpLock, checking this.receiveHandler for null,
        // calls this.receiveHandler.CloseAsync (starting this operation inside the receivePumpLock).
        void ReceiveHandlerClose()
        {
            lock (this.receivePumpLock)
            {
                if (this.receiveHandler != null)
                {
                    if (this.receivePumpTask != null)
                    {
                        this.receivePumpCancellationSource.Cancel();
                        this.receivePumpCancellationSource.Dispose();
                        this.receivePumpCancellationSource = null;
                        this.receivePumpTask = null;
                    }

                    this.receiveHandler = null;
                }
            }
        }

        // Encapsulates taking the receivePumpLock, checking this.receiveHandler for null,
        // calls this.receiveHandler.ProcessErrorAsync (starting this operation inside the receivePumpLock).
        Task ReceiveHandlerProcessErrorAsync(Exception error)
        {
            Task processErrorTask = null;
            lock (this.receivePumpLock)
            {
                if (this.receiveHandler != null)
                {
                    processErrorTask = this.receiveHandler.ProcessErrorAsync(error);
                }
            }

            return processErrorTask ?? Task.CompletedTask;
        }

        // Encapsulates taking the receivePumpLock, checking this.receiveHandler for null,
        // calls this.receiveHandler.ProcessErrorAsync (starting this operation inside the receivePumpLock).
        Task ReceiveHandlerProcessEventsAsync(IEnumerable<EventData> eventDatas)
        {
            Task processEventsTask = null;
            lock (this.receivePumpLock)
            {
                if (this.receiveHandler != null)
                {
                    processEventsTask = this.receiveHandler.ProcessEventsAsync(eventDatas);
                }
            }

            return processEventsTask ?? Task.CompletedTask;
        }
    }
}
