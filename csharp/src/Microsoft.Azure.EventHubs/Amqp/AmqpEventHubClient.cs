// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Amqp
{
    using System;
    using System.Linq;
    using System.Net;
    using System.Net.Security;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Sasl;
    using Microsoft.Azure.Amqp;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;
    using Microsoft.Azure.EventHubs.Amqp.Management;

    sealed class AmqpEventHubClient : EventHubClient
    {
        const string CbsSaslMechanismName = "MSSBCBS";
        AmqpServiceClient<IAmqpEntityManagement> managementServiceClient; // serviceClient that handles management calls

        public AmqpEventHubClient(EventHubsConnectionSettings connectionSettings)
            : base(connectionSettings)
        {
            this.ContainerId = Guid.NewGuid().ToString("N");
            this.AmqpVersion = new Version(1, 0, 0, 0);
            this.MaxFrameSize = AmqpConstants.DefaultMaxFrameSize;
            this.TokenProvider = connectionSettings.CreateTokenProvider();
            this.CbsTokenProvider = new TokenProviderAdapter(this);
            this.ConnectionManager = new FaultTolerantAmqpObject<AmqpConnection>(this.CreateConnectionAsync, this.CloseConnection);
        }

        internal ICbsTokenProvider CbsTokenProvider { get; }

        internal FaultTolerantAmqpObject<AmqpConnection> ConnectionManager { get; }

        internal string ContainerId { get; }

        Version AmqpVersion { get; }

        uint MaxFrameSize { get; }

        TokenProvider TokenProvider { get; }

        internal override EventDataSender OnCreateEventSender(string partitionId)
        {
            return new AmqpEventDataSender(this, partitionId);
        }

        protected override PartitionReceiver OnCreateReceiver(
            string consumerGroupName, string partitionId, string startOffset, bool offsetInclusive, DateTime? startTime, long? epoch)
        {
            return new AmqpPartitionReceiver(
                this, consumerGroupName, partitionId, startOffset, offsetInclusive, startTime, epoch);
        }

        protected override Task OnCloseAsync()
        {
            // Closing the Connection will also close all Links associated with it.
            return this.ConnectionManager.CloseAsync();
        }

        internal async Task<ActiveClientRequestResponseLink> OpenRequestResponseLinkAsync(
            string type, string address, MessagingEntityType? entityType, string[] requiredClaims, TimeSpan timeout)
        {
            var timeoutHelper = new TimeoutHelper(timeout, true);
            AmqpSession session = null;
            try
            {
                // Don't need to get token for namespace scope operations, included in request
                bool isNamespaceScope = address.Equals(AmqpClientConstants.ManagementAddress, StringComparison.OrdinalIgnoreCase);

                var connection = await this.ConnectionManager.GetOrCreateAsync(timeoutHelper.RemainingTime());

                var sessionSettings = new AmqpSessionSettings { Properties = new Fields() };
                //sessionSettings.Properties[AmqpClientConstants.BatchFlushIntervalName] = (uint)batchFlushInterval.TotalMilliseconds;
                session = connection.CreateSession(sessionSettings);

                await session.OpenAsync(timeoutHelper.RemainingTime());

                var linkSettings = new AmqpLinkSettings();
                linkSettings.AddProperty(AmqpClientConstants.TimeoutName, (uint)timeoutHelper.RemainingTime().TotalMilliseconds);
                if (entityType != null)
                {
                    linkSettings.AddProperty(AmqpClientConstants.EntityTypeName, (int)entityType.Value);
                }

                // Create the link
                var link = new RequestResponseAmqpLink(type, session, address, linkSettings.Properties);

                bool isClientToken = false;
                var authorizationValidToUtc = DateTime.MaxValue;

                if (!isNamespaceScope)
                {
                    // TODO: Get Entity level token here
                }

                await link.OpenAsync(timeoutHelper.RemainingTime());

                // Redirected scenario requires entityPath as the audience, otherwise we 
                // should always use the full EndpointUri as audience.
                return new ActiveClientRequestResponseLink(
                    link,
                    this.ConnectionSettings.Endpoint.AbsoluteUri, // audience
                    this.ConnectionSettings.Endpoint.AbsoluteUri, // endpointUri
                    requiredClaims,
                    isClientToken,
                    authorizationValidToUtc);
            }
            catch (Exception)
            {
                if (session != null)
                {
                    // Aborting the session will cleanup the link as well.
                    session.Abort();
                }

                throw;
            }
        }

        protected override async Task<EventHubRuntimeInformation> OnGetRuntimeInformationAsync()
        {
            try
            {
                var timeoutHelper = new TimeoutHelper(this.ConnectionSettings.OperationTimeout);
                string serviceClientAddress = //this.ConnectionSettings.Endpoint.IsIoTDeviceUri() ?
                    //string.Concat(this.ConnectionSettings.EntityPath, AmqpClientConstants.ManagementAddressSegment) :
                    AmqpClientConstants.ManagementAddress;

                string entityType = AmqpClientConstants.ManagementEventHubEntityTypeValue;
                SecurityToken token = await this.TokenProvider.GetTokenAsync(this.ConnectionSettings.Endpoint.AbsoluteUri, ClaimConstants.Manage, timeoutHelper.RemainingTime());

                var serviceClient = this.GetManagementServiceClient(serviceClientAddress);
                var eventHubRuntimeInformation = await serviceClient.Channel.GetRuntimeInfoAsync<EventHubRuntimeInformation>(
                    entityType, this.ConnectionSettings.EntityPath, null, token.TokenValue.ToString(), this.ConnectionSettings.OperationTimeout);

                return eventHubRuntimeInformation;
            }
            catch (AggregateException aggregateException) when (aggregateException.InnerExceptions.Count == 1)
            {
                // The AmqpServiceClient for some reason wraps errors with an unnecessary AggregateException, unwrap here.
                throw aggregateException.InnerException;
            }
        }

        internal AmqpServiceClient<IAmqpEntityManagement> GetManagementServiceClient(string address)
        {
            if (this.managementServiceClient == null)
            {
                lock (ThisLock)
                {
                    if (this.managementServiceClient == null)
                    {
                        this.managementServiceClient = new AmqpServiceClient<IAmqpEntityManagement>(this, address);
                    }

                    Fx.Assert(string.Equals(this.managementServiceClient.Address, address, StringComparison.OrdinalIgnoreCase), "The address should match the address of managementServiceClient");
                }
            }

            return this.managementServiceClient;
        }

        internal static AmqpSettings CreateAmqpSettings(
            Version amqpVersion,
            bool useSslStreamSecurity,
            bool hasTokenProvider,
            string sslHostName = null,
            bool useWebSockets = false,
            bool sslStreamUpgrade = false,
            NetworkCredential networkCredential = null,
            RemoteCertificateValidationCallback certificateValidationCallback = null,
            bool forceTokenProvider = true)
        {
            var settings = new AmqpSettings();
            if (useSslStreamSecurity && !useWebSockets && sslStreamUpgrade)
            {
                var tlsSettings = new TlsTransportSettings();
                tlsSettings.CertificateValidationCallback = certificateValidationCallback;
                tlsSettings.TargetHost = sslHostName;

                var tlsProvider = new TlsTransportProvider(tlsSettings);
                tlsProvider.Versions.Add(new AmqpVersion(amqpVersion));
                settings.TransportProviders.Add(tlsProvider);
            }

            if (hasTokenProvider || networkCredential != null)
            {
                var saslProvider = new SaslTransportProvider();
                saslProvider.Versions.Add(new AmqpVersion(amqpVersion));
                settings.TransportProviders.Add(saslProvider);

                if (forceTokenProvider)
                {
                    saslProvider.AddHandler(new SaslAnonymousHandler(CbsSaslMechanismName));
                }
                else if (networkCredential != null)
                {
                    var plainHandler = new SaslPlainHandler();
                    plainHandler.AuthenticationIdentity = networkCredential.UserName;
                    plainHandler.Password = networkCredential.Password;
                    saslProvider.AddHandler(plainHandler);
                }
                else
                {
                    // old client behavior: keep it for validation only
                    saslProvider.AddHandler(new SaslExternalHandler());
                }
            }

            var amqpProvider = new AmqpTransportProvider();
            amqpProvider.Versions.Add(new AmqpVersion(amqpVersion));
            settings.TransportProviders.Add(amqpProvider);

            return settings;
        }

        static TransportSettings CreateTcpTransportSettings(
            string networkHost,
            string hostName,
            int port,
            bool useSslStreamSecurity,
            bool sslStreamUpgrade = false,
            string sslHostName = null,
            X509Certificate2 certificate = null,
            RemoteCertificateValidationCallback certificateValidationCallback = null)
        {
            TcpTransportSettings tcpSettings = new TcpTransportSettings
            {
                Host = networkHost,
                Port = port < 0 ? AmqpConstants.DefaultSecurePort : port,
                ReceiveBufferSize = AmqpConstants.TransportBufferSize,
                SendBufferSize = AmqpConstants.TransportBufferSize
            };

            TransportSettings tpSettings = tcpSettings;
            if (useSslStreamSecurity && !sslStreamUpgrade)
            {
                TlsTransportSettings tlsSettings = new TlsTransportSettings(tcpSettings)
                {
                    TargetHost = sslHostName ?? hostName,
                    Certificate = certificate,
                    CertificateValidationCallback = certificateValidationCallback
                };
                tpSettings = tlsSettings;
            }

            return tpSettings;
        }

        static AmqpConnectionSettings CreateAmqpConnectionSettings(uint maxFrameSize, string containerId, string hostName)
        {
            var connectionSettings = new AmqpConnectionSettings
            {
                MaxFrameSize = maxFrameSize,
                ContainerId = containerId,
                HostName = hostName
            };
            return connectionSettings;
        }

        async Task<AmqpConnection> CreateConnectionAsync(TimeSpan timeout)
        {
            string hostName = this.ConnectionSettings.Endpoint.Host;
            string networkHost = this.ConnectionSettings.Endpoint.Host;
            int port = this.ConnectionSettings.Endpoint.Port;

            var timeoutHelper = new TimeoutHelper(timeout);
            var amqpSettings = CreateAmqpSettings(
                amqpVersion: this.AmqpVersion,
                useSslStreamSecurity: true,
                hasTokenProvider: true);

            TransportSettings tpSettings = CreateTcpTransportSettings(
                networkHost: networkHost,
                hostName: hostName,
                port: port,
                useSslStreamSecurity: true);

            var initiator = new AmqpTransportInitiator(amqpSettings, tpSettings);
            var transport = await initiator.ConnectTaskAsync(timeoutHelper.RemainingTime());

            var connectionSettings = CreateAmqpConnectionSettings(this.MaxFrameSize, this.ContainerId, hostName);
            var connection = new AmqpConnection(transport, amqpSettings, connectionSettings);
            await connection.OpenAsync(timeoutHelper.RemainingTime());

            // Always create the CBS Link + Session
            var cbsLink = new AmqpCbsLink(connection);
            if (connection.Extensions.Find<AmqpCbsLink>() == null)
            {
                connection.Extensions.Add(cbsLink);
            }

            return connection;
        }

        void CloseConnection(AmqpConnection connection)
        {
            connection.SafeClose();
        }

        /// <summary>
        /// Provides an adapter from TokenProvider to ICbsTokenProvider for AMQP CBS usage.
        /// </summary>
        sealed class TokenProviderAdapter : ICbsTokenProvider
        {
            readonly AmqpEventHubClient eventHubClient;

            public TokenProviderAdapter(AmqpEventHubClient eventHubClient)
            {
                Fx.Assert(eventHubClient != null, "tokenProvider cannot be null");
                this.eventHubClient = eventHubClient;
            }

            public async Task<CbsToken> GetTokenAsync(Uri namespaceAddress, string appliesTo, string[] requiredClaims)
            {
                string claim = requiredClaims?.FirstOrDefault();
                var tokenProvider = this.eventHubClient.TokenProvider;
                var timeout = this.eventHubClient.ConnectionSettings.OperationTimeout;
                var token = await tokenProvider.GetTokenAsync(appliesTo, claim, timeout);
                return new CbsToken(token.TokenValue, CbsConstants.ServiceBusSasTokenType, token.ExpiresAtUtc);
            }
        }
    }
}