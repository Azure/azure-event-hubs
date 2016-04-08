// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp;
    using Microsoft.Azure.Amqp.Framing;

    class AmqpEventSender : EventSender
    {
        readonly AmqpEventHubClient eventHubClient;

        internal AmqpEventSender(AmqpEventHubClient eventHubClient, string partitionId)
            : base(partitionId)
        {
            this.eventHubClient = eventHubClient;
            if (!string.IsNullOrEmpty(partitionId))
            {
                this.Path = $"{eventHubClient.EventHubName}/Partitions/{partitionId}";
            }
            else
            {
                this.Path = eventHubClient.EventHubName;
            }

            this.SendLinkManager = new AmqpSendLinkManager(this);
        }

        string Path { get; }

        AmqpSendLinkManager SendLinkManager { get; }

        public override Task CloseAsync()
        {
            return this.SendLinkManager.CloseAsync();
        }

        protected override async Task OnSendAsync(IEnumerable<EventData> eventDatas, string partitionKey)
        {
            using (AmqpMessage amqpMessage = EventDatasToAmqpMessage(eventDatas, partitionKey, true))
            {
                var sendLink = await this.SendLinkManager.GetLinkAsync();

                throw new NotImplementedException("Send message on Aqmp Link");
            }
        }

        static AmqpMessage EventDatasToAmqpMessage(IEnumerable<EventData> eventDatas, string partitionKey, bool batchable)
        {
            //throw new NotImplementedException("TODO: Serialize batch of EventData here.");
            return null;
        }

        /// <summary>
        /// This class is responsible for getting or [re]creating the SendingAmqpLink when asked for it.
        /// </summary>
        class AmqpSendLinkManager
        {
            readonly AmqpEventHubClient eventHubClient;
            Task<SendingAmqpLink> createLinkTask;

            public AmqpSendLinkManager(AmqpEventSender amqpEventSender)
            {
                this.eventHubClient = amqpEventSender.eventHubClient;
            }

            object ThisLock { get; } = new object();

            internal async Task<SendingAmqpLink> GetLinkAsync()
            {
                var localCreateLinkTask = this.createLinkTask;
                if (localCreateLinkTask != null)
                {
                    var sendLink = await localCreateLinkTask;
                    if (!sendLink.IsClosing())
                    {
                        return sendLink;
                    }
                    else
                    {
                        lock (this.ThisLock)
                        {
                            if (object.ReferenceEquals(localCreateLinkTask, this.createLinkTask))
                            {
                                this.createLinkTask = null;
                            }
                        }
                    }
                }

                lock (this.ThisLock)
                {
                    if (this.createLinkTask == null)
                    {
                        this.createLinkTask = this.CreateLinkAsync();
                    }

                    localCreateLinkTask = this.createLinkTask;
                }

                return await localCreateLinkTask;
            }

            async Task<SendingAmqpLink> CreateLinkAsync()
            {
                var timeoutHelper = new TimeoutHelper(this.eventHubClient.ConnectionSettings.OperationTimeout, startTimeout: true);
                AmqpConnection connection = await this.eventHubClient.ConnectionManager.GetConnectionAsync();

                // Authenticate over CBS
                var cbsLink = connection.Extensions.Find<AmqpCbsLink>();

                throw new NotImplementedException("Authenticate over CBS here!");
                ICbsTokenProvider cbsTokenProvider = null;
                Uri address = null;
                string audience = null;
                string resource = null;
                var expiresAt = await cbsLink.SendTokenAsync(cbsTokenProvider, address, audience, resource, new[] { ClaimConstants.Send }, timeoutHelper.RemainingTime());

                // Create our Session
                AmqpSessionSettings sessionSettings = new AmqpSessionSettings { Properties = new Fields() };
                //sessionSettings.Properties[AmqpClientConstants.BatchFlushIntervalName] = (uint)batchFlushInterval.TotalMilliseconds;
                var session = connection.CreateSession(sessionSettings);
                await session.OpenAsync(timeoutHelper.RemainingTime());

                // Create our Link
                throw new NotImplementedException("CREATE LINK HERE.");
            }

            internal async Task CloseAsync()
            {
                var localCreateTask = this.createLinkTask;
                if (localCreateTask != null)
                {
                    var timeoutHelper = new TimeoutHelper(this.eventHubClient.ConnectionSettings.OperationTimeout, startTimeout: true);

                    var localSendLink = await localCreateTask;
                    await localSendLink.CloseAsync(timeoutHelper.RemainingTime());
                    
                    throw new NotImplementedException("TODO: Close the Session as well?");
                }
            }
        }
    }
}
