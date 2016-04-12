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
    using Microsoft.Azure.Amqp.Transport;

    sealed class AmqpEventHubClient : EventHubClient
    {
        const string CbsSaslMechanismName = "MSSBCBS";

        public AmqpEventHubClient(ServiceBusConnectionSettings connectionSettings)
            : base(connectionSettings)
        {
            this.ContainerId = Guid.NewGuid().ToString("N");
            this.AmqpVersion = new Version(1, 0, 0, 0);
            this.MaxFrameSize = AmqpConstants.DefaultMaxFrameSize;
            var tokenProvider = connectionSettings.CreateTokenProvider();
            this.CbsTokenProvider = new TokenProviderAdapter(tokenProvider, connectionSettings.OperationTimeout);
            this.ConnectionManager = new AmqpConnectionManager(this);
        }

        internal ICbsTokenProvider CbsTokenProvider { get; }

        internal AmqpConnectionManager ConnectionManager { get; }

        internal string ContainerId { get; }

        Version AmqpVersion { get; }

        uint MaxFrameSize { get; }

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

        public override async Task CloseAsync()
        {
            // Closing the Connection will also close all Links associated with it.
            await this.ConnectionManager.CloseAsync();
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

        /// <summary>
        /// This class is responsible for getting or creating the AmqpConnection for use.
        /// </summary>
        internal sealed class AmqpConnectionManager
        {
            readonly AmqpEventHubClient eventHubClient;
            Task<AmqpConnection> createConnectionTask;

            public AmqpConnectionManager(AmqpEventHubClient eventHubClient)
            {
                this.eventHubClient = eventHubClient;
            }

            object ThisLock { get; } = new object();

            internal async Task<AmqpConnection> GetConnectionAsync()
            {
                var localCreateConnectionTask = this.createConnectionTask;
                if (localCreateConnectionTask != null)
                {
                    var connection = await localCreateConnectionTask;
                    if (!connection.IsClosing())
                    {
                        return connection;
                    }
                    else
                    {
                        lock (this.ThisLock)
                        {
                            if (object.ReferenceEquals(localCreateConnectionTask, this.createConnectionTask))
                            {
                                this.createConnectionTask = null;
                            }
                        }
                    }
                }

                lock (this.ThisLock)
                {
                    if (this.createConnectionTask == null)
                    {
                        this.createConnectionTask = this.CreateConnectionAsync();
                    }

                    localCreateConnectionTask = this.createConnectionTask;
                }

                return await localCreateConnectionTask;
            }

            async Task<AmqpConnection> CreateConnectionAsync()
            {
                string hostName = this.eventHubClient.ConnectionSettings.Endpoint.Host;
                string networkHost = this.eventHubClient.ConnectionSettings.Endpoint.Host;
                int port = this.eventHubClient.ConnectionSettings.Endpoint.Port;

                var timeoutHelper = new TimeoutHelper(this.eventHubClient.ConnectionSettings.OperationTimeout);
                var amqpSettings = CreateAmqpSettings(
                    amqpVersion: this.eventHubClient.AmqpVersion,
                    useSslStreamSecurity: true,
                    hasTokenProvider: true);

                TransportSettings tpSettings = CreateTcpTransportSettings(
                    networkHost: networkHost,
                    hostName: hostName,
                    port: port,
                    useSslStreamSecurity: true);

                var initiator = new AmqpTransportInitiator(amqpSettings, tpSettings);
                var transport = await initiator.ConnectTaskAsync(timeoutHelper.RemainingTime());

                var connectionSettings = CreateAmqpConnectionSettings(this.eventHubClient.MaxFrameSize, this.eventHubClient.ContainerId, hostName);
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

            public async Task CloseAsync()
            {
                var localCreateTask = this.createConnectionTask;
                if (localCreateTask != null)
                {
                    var timeoutHelper = new TimeoutHelper(this.eventHubClient.ConnectionSettings.OperationTimeout, startTimeout: true);
                    var connection = await localCreateTask;
                    await connection.CloseAsync(timeoutHelper.RemainingTime());
                }
            }
        }

        /// <summary>
        /// Provides an adapter from TokenProvider to ICbsTokenProvider for AMQP CBS usage.
        /// </summary>
        sealed class TokenProviderAdapter : ICbsTokenProvider
        {
            readonly TokenProvider tokenProvider;
            readonly TimeSpan operationTimeout;

            public TokenProviderAdapter(TokenProvider tokenProvider, TimeSpan operationTimeout)
            {
                Fx.Assert(tokenProvider != null, "tokenProvider cannot be null");
                this.tokenProvider = tokenProvider;
                this.operationTimeout = operationTimeout;
            }

            public async Task<CbsToken> GetTokenAsync(Uri namespaceAddress, string appliesTo, string[] requiredClaims)
            {
                string claim = requiredClaims?.FirstOrDefault();
                var token = await this.tokenProvider.GetTokenAsync(appliesTo, claim, this.operationTimeout);
                return new CbsToken(token.TokenValue, CbsConstants.ServiceBusSasTokenType, token.ExpiresAtUtc);
            }
        }
    }
}