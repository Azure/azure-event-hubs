﻿// Copyright (c) Microsoft. All rights reserved.
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

            this.SendLinkManager = new FaultTolerantAmqpObject<SendingAmqpLink>(this.CreateLinkAsync, this.CloseSession);
        }

        string Path { get; }

        FaultTolerantAmqpObject<SendingAmqpLink> SendLinkManager { get; }

        public override Task CloseAsync()
        {
            return this.SendLinkManager.CloseAsync();
        }

        protected override async Task OnSendAsync(IEnumerable<EventData> eventDatas, string partitionKey)
        {
            var timeoutHelper = new TimeoutHelper(this.EventHubClient.ConnectionSettings.OperationTimeout, true);
            using (AmqpMessage amqpMessage = AmqpMessageConverter.EventDatasToAmqpMessage(eventDatas, partitionKey, true))
            {
                var amqpLink = await this.SendLinkManager.GetOrCreateAsync(timeoutHelper.RemainingTime());
                if (amqpLink.Settings.MaxMessageSize.HasValue)
                {
                    ulong size = (ulong)amqpMessage.SerializedMessageSize;
                    if (size > amqpLink.Settings.MaxMessageSize.Value)
                    {
                        // TODO: Add MessageSizeExceededException
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

        async Task<SendingAmqpLink> CreateLinkAsync(TimeSpan timeout)
        {
            var amqpEventHubClient = ((AmqpEventHubClient)this.EventHubClient);
            var connectionSettings = amqpEventHubClient.ConnectionSettings;
            var timeoutHelper = new TimeoutHelper(connectionSettings.OperationTimeout);
            AmqpConnection connection = await amqpEventHubClient.ConnectionManager.GetOrCreateAsync(timeoutHelper.RemainingTime());

            // Authenticate over CBS
            var cbsLink = connection.Extensions.Find<AmqpCbsLink>();

            ICbsTokenProvider cbsTokenProvider = amqpEventHubClient.CbsTokenProvider;
            Uri address = new Uri(connectionSettings.Endpoint, this.Path);
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
                linkSettings.Source = new Source { Address = this.ClientId };

                var link = new SendingAmqpLink(linkSettings);
                linkSettings.LinkName = $"{amqpEventHubClient.ContainerId};{connection.Identifier}:{session.Identifier}:{link.Identifier}";
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

        void CloseSession(SendingAmqpLink link)
        {
            // Note we close the session (which includes the link).
            link.Session.SafeClose();
        }
    }
}
