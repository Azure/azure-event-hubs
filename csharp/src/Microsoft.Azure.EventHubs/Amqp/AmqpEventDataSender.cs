// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp;
    using Microsoft.Azure.Amqp.Framing;

    class AmqpEventDataSender : EventDataSender
    {
        int deliveryCount;

        internal AmqpEventDataSender(AmqpEventHubClient eventHubClient, string partitionId)
            : base(eventHubClient, partitionId)
        {
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
            var timeoutHelper = new TimeoutHelper(this.EventHubClient.ConnectionSettings.OperationTimeout, true);
            using (AmqpMessage amqpMessage = AmqpMessageConverter.EventDatasToAmqpMessage(eventDatas, partitionKey, true))
            {
                var amqpLink = await this.SendLinkManager.GetLinkAsync();

                if (amqpLink.Settings.MaxMessageSize.HasValue)
                {
                    ulong size = (ulong)amqpMessage.SerializedMessageSize;
                    if (size > amqpLink.Settings.MaxMessageSize.Value)
                    {
                        throw new NotImplementedException("MessageSizeExceededException: " + Resources.AmqpMessageSizeExceeded.FormatForUser(amqpMessage.DeliveryId.Value, size, amqpLink.Settings.MaxMessageSize.Value));
                        //throw Fx.Exception.AsError(new MessageSizeExceededException(
                        //    Resources.AmqpMessageSizeExceeded.FormatForUser(amqpMessage.DeliveryId.Value, size, amqpLink.Settings.MaxMessageSize.Value)));
                    }
                }

                Outcome outcome = await amqpLink.SendMessageAsync(amqpMessage, this.GetNextDeliveryTag(), AmqpConstants.NullBinary, timeoutHelper.RemainingTime());
                if (outcome.DescriptorCode != Accepted.Code)
                {
                    Rejected rejected = (Rejected)outcome;
                    throw Fx.Exception.AsError(AmqpExceptionHelper.ToMessagingContract(rejected.Error));
                }
            }
        }

        ArraySegment<byte> GetNextDeliveryTag()
        {
            int deliveryId = Interlocked.Increment(ref this.deliveryCount);
            return new ArraySegment<byte>(BitConverter.GetBytes(deliveryId));
        }

        /// <summary>
        /// This class is responsible for getting or [re]creating the SendingAmqpLink when asked for it.
        /// </summary>
        class AmqpSendLinkManager
        {
            readonly AmqpEventDataSender eventSender;
            readonly AmqpEventHubClient eventHubClient;
            volatile Task<SendingAmqpLink> createLinkTask;

            public AmqpSendLinkManager(AmqpEventDataSender eventSender)
            {
                this.eventSender = eventSender;
                this.eventHubClient = (AmqpEventHubClient)eventSender.EventHubClient;
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
                var connectionSettings = this.eventHubClient.ConnectionSettings;
                var timeoutHelper = new TimeoutHelper(connectionSettings.OperationTimeout, startTimeout: true);
                AmqpConnection connection = await this.eventHubClient.ConnectionManager.GetConnectionAsync();

                // Authenticate over CBS
                var cbsLink = connection.Extensions.Find<AmqpCbsLink>();

                ICbsTokenProvider cbsTokenProvider = this.eventHubClient.CbsTokenProvider;
                Uri address = new Uri(connectionSettings.Endpoint, this.eventSender.Path);
                string audience = address.AbsoluteUri;
                string resource = address.AbsoluteUri;
                var expiresAt = await cbsLink.SendTokenAsync(cbsTokenProvider, address, audience, resource, new[] { ClaimConstants.Send }, timeoutHelper.RemainingTime());

                AmqpSession session = null;
                try
                {
                    // Create our Session
                    var sessionSettings = new AmqpSessionSettings { Properties = new Fields() };
                    //sessionSettings.Properties[AmqpClientConstants.BatchFlushIntervalName] = (uint)connectionSettings.BatchFlushInterval.TotalMilliseconds;
                    session = connection.CreateSession(sessionSettings);
                    await session.OpenAsync(timeoutHelper.RemainingTime());

                    // Create our Link
                    var linkSettings = new AmqpLinkSettings();
                    linkSettings.AddProperty(AmqpClientConstants.TimeoutName, (uint)timeoutHelper.RemainingTime().TotalMilliseconds);
                    linkSettings.AddProperty(AmqpClientConstants.EntityTypeName, (int)MessagingEntityType.EventHub);
                    linkSettings.Role = false;
                    linkSettings.InitialDeliveryCount = 0;
                    linkSettings.Target = new Target { Address = address.AbsolutePath };
                    linkSettings.Source = new Source { Address = this.eventSender.ClientId };

                    var link = new SendingAmqpLink(linkSettings);
                    linkSettings.LinkName = $"{this.eventHubClient.ContainerId};{connection.Identifier}:{session.Identifier}:{link.Identifier}";
                    link.AttachTo(session);

                    await link.OpenAsync(timeoutHelper.RemainingTime());
                    return link;
                }
                catch (Exception)
                {
                    // Cleanup any session (and thus link) in case of exception.
                    session?.Abort();
                    throw;
                }
            }

            internal async Task CloseAsync()
            {
                var localCreateTask = this.createLinkTask;
                if (localCreateTask != null)
                {
                    var timeoutHelper = new TimeoutHelper(this.eventHubClient.ConnectionSettings.OperationTimeout, startTimeout: true);
                    var localSendLink = await localCreateTask;

                    // Note we close the session (which includes the link).
                    await localSendLink.Session.CloseAsync(timeoutHelper.RemainingTime());
                }
            }
        }
    }
}
