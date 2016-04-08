// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
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
        readonly string containerId;

        public AmqpEventHubClient(ServiceBusConnectionSettings connectionSettings)
            : base(connectionSettings)
        {
            this.containerId = Guid.NewGuid().ToString("N");
            this.AmqpVersion = new Version(1, 0, 0, 0);
            this.UseSslStreamSecurity = true;
            this.MaxFrameSize = (int)AmqpConstants.DefaultMaxFrameSize;
            this.ConnectionManager = new AmqpConnectionManager(this);
        }

        internal AmqpConnectionManager ConnectionManager { get; }

        Version AmqpVersion { get; }

        int MaxFrameSize { get; }

        bool UseSslStreamSecurity { get; }

        internal override EventSender OnCreateEventSender(string partitionId)
        {
            return new AmqpEventSender(this, partitionId);
        }

        protected override PartitionReceiver OnCreateReceiver(string consumerGroupName, string partitionId, string startingOffset, bool offsetInclusive, DateTime? dateTime, long epoch, bool isEpochReceiver)
        {
            return new AmqpPartitionReceiver(
                this, consumerGroupName, partitionId, startingOffset, offsetInclusive, dateTime, epoch, isEpochReceiver);
        }

        public override async Task CloseAsync()
        {
            await this.ConnectionManager.CloseAsync();
        }

        static string GetDomainName(string hostName)
        {
            string domainName = hostName;
            int colonIndex = domainName.IndexOf(':');
            if (colonIndex > 0)
            {
                domainName = domainName.Substring(0, colonIndex);
            }

            return domainName;
        }

        internal static AmqpSettings CreateAmqpSettings(
            string sslHostName,
            bool useSslStreamSecurity,
            bool useWebSockets,
            bool sslStreamUpgrade,
            bool hasTokenProvider,
            NetworkCredential networkCredential,
            Version amqpVersion,
            RemoteCertificateValidationCallback certificateValidationCallback,
            bool forceTokenProvider = false)
        {
            var settings = new AmqpSettings();
            if (useSslStreamSecurity && !useWebSockets && sslStreamUpgrade)
            {
                TlsTransportSettings tlsSettings = new TlsTransportSettings();
                tlsSettings.CertificateValidationCallback = certificateValidationCallback;
                tlsSettings.TargetHost = sslHostName;

                TlsTransportProvider tlsProvider = new TlsTransportProvider(tlsSettings);
                tlsProvider.Versions.Add(new AmqpVersion(1, 0, 0));
                settings.TransportProviders.Add(tlsProvider);
            }

            if (hasTokenProvider || networkCredential != null)
            {
                SaslTransportProvider saslProvider = new SaslTransportProvider();
                saslProvider.Versions.Add(new AmqpVersion(1, 0, 0));
                settings.TransportProviders.Add(saslProvider);

                if (forceTokenProvider)
                {
                    saslProvider.AddHandler(new SaslAnonymousHandler(CbsSaslMechanismName));
                }
                else if (networkCredential != null)
                {
                    SaslPlainHandler plainHandler = new SaslPlainHandler();
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

            AmqpTransportProvider amqpProvider = new AmqpTransportProvider();
            amqpProvider.Versions.Add(new AmqpVersion(amqpVersion));
            settings.TransportProviders.Add(amqpProvider);

            return settings;
        }

        static TransportSettings CreateTcpTransportSettings(
            string networkHost,
            string hostName,
            int port,
            bool useSslStreamSecurity,
            bool sslStreamUpgrade,
            string sslHostName,
            X509Certificate2 certificate,
            RemoteCertificateValidationCallback validateCertificate)
        {
            TcpTransportSettings tcpSettings = new TcpTransportSettings();
            tcpSettings.Host = networkHost;
            tcpSettings.Port = port < 0 ? AmqpConstants.DefaultSecurePort : port;
            tcpSettings.ReceiveBufferSize = AmqpConstants.TransportBufferSize;
            tcpSettings.SendBufferSize = AmqpConstants.TransportBufferSize;

            TransportSettings tpSettings = tcpSettings;
            if (useSslStreamSecurity && !sslStreamUpgrade)
            {
                TlsTransportSettings tlsSettings = new TlsTransportSettings(tcpSettings);
                tlsSettings.TargetHost = sslHostName ?? GetDomainName(hostName);
                tlsSettings.Certificate = certificate;
                tlsSettings.CertificateValidationCallback = validateCertificate;
                tpSettings = tlsSettings;
            }

            return tpSettings;
        }

        static AmqpConnectionSettings CreateAmqpConnectionSettings(int maxFrameSize, string containerId, string hostName)
        {
            var connectionSettings = new AmqpConnectionSettings();
            connectionSettings.MaxFrameSize = checked((uint)maxFrameSize);
            connectionSettings.ContainerId = containerId;
            connectionSettings.HostName = hostName;
            return connectionSettings;
        }

        /// <summary>
        /// This class is responsible for getting or creating the AmqpConnection for use.
        /// </summary>
        sealed internal class AmqpConnectionManager
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
                string sslHostName = null;

                var timeoutHelper = new TimeoutHelper(this.eventHubClient.ConnectionSettings.OperationTimeout);
                var amqpSettings = CreateAmqpSettings(
                    sslHostName: sslHostName,
                    useSslStreamSecurity: this.eventHubClient.UseSslStreamSecurity,
                    useWebSockets: false,
                    sslStreamUpgrade: false,
                    hasTokenProvider: true,
                    networkCredential: null,
                    amqpVersion: this.eventHubClient.AmqpVersion,
                    certificateValidationCallback: null,
                    forceTokenProvider: false);

                TransportSettings tpSettings = CreateTcpTransportSettings(
                    networkHost: networkHost,
                    hostName: hostName,
                    port: port,
                    useSslStreamSecurity: this.eventHubClient.UseSslStreamSecurity,
                    sslStreamUpgrade: false,
                    sslHostName: sslHostName,
                    certificate: null,
                    validateCertificate: null);

                var initiator = new AmqpTransportInitiator(amqpSettings, tpSettings);
                var transport = await initiator.ConnectTaskAsync(timeoutHelper.RemainingTime());

                var connectionSettings = CreateAmqpConnectionSettings(this.eventHubClient.MaxFrameSize, this.eventHubClient.containerId, hostName);
                var connection = new AmqpConnection(transport, amqpSettings, connectionSettings);
                await connection.OpenAsync(timeoutHelper.RemainingTime());

                // Always create the CBS Link + Session as well (.ctor adds it to AmqpConnection.Extensions for easy access).
                var cbsLink = new AmqpCbsLink(connection);

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
    }
}