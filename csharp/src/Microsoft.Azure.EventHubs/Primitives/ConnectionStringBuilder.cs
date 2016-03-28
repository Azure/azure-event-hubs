// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Text;

    /// <summary>
    /// ConnectionStringBuilder can be used to construct a connection string which can establish communication with ServiceBus entities.
    /// It can also be used to perform basic validation on an existing connection string.
    /// <para/>
    /// A connection string is basically a string consisted of key-value pair separated by ";". 
    /// Basic format is "&lt;key&gt;=&lt;value&gt;[;&lt;key&gt;=&lt;value&gt;]" where supported key name are as follow:
    /// <para/> Endpoint - the URL that contains the servicebus namespace
    /// <para/> EntityPath - the path to the service bus entity (queue/topic/eventhub/subscription/consumergroup/partition)
    /// <para/> SharedAccessKeyName - the key name to the corresponding shared access policy rule for the namespace, or entity.
    /// <para/> SharedAccessKey - the key for the corresponding shared access policy rule of the namespace or entity.
    /// </summary>
    /// <example>
    /// Sample code:
    /// <code>
    /// ConnectionStringBuilder connectionStringBuilder = new ConnectionStringBuilder(
    ///     "ServiceBusNamespaceName", 
    ///     "ServiceBusEntityName", // eventHub, queue, or topic name 
    ///     "SharedAccessSignatureKeyName", 
    ///     "SharedAccessSignatureKey");
    ///  string connectionString = connectionStringBuilder.toString();
    /// </code>
    /// </example>
    public class ConnectionStringBuilder
    {
        static readonly TimeSpan DefaultOperationTimeout = TimeSpan.FromMinutes(1);
        static readonly string EndpointFormat = "amqps://{0}.servicebus.windows.net";
        static readonly string EndpointConfigName = "Endpoint";
        static readonly string SharedAccessKeyNameConfigName = "SharedAccessKeyName";
        static readonly string SharedAccessKeyConfigName = "SharedAccessKey";
        static readonly string EntityPathConfigName = "EntityPath";
        const char KeyValueSeparator = '=';
        const char KeyValuePairDelimiter = ';';

        /// <summary>
        /// Build a connection string consumable by <see cref="EventHubClient.Create(string)"/>
        /// </summary>
        /// <param name="namespaceName">Namespace name (the dns suffix, ex: .servicebus.windows.net, is not required)</param>
        /// <param name="entityPath">Entity path. For eventHubs case specify eventHub name.</param>
        /// <param name="sharedAccessKeyName">Shared Access Key name</param>
        /// <param name="sharedAccessKey">Shared Access Key</param>
        public ConnectionStringBuilder(string namespaceName, string entityPath, string sharedAccessKeyName, string sharedAccessKey)
            : this(namespaceName, entityPath, sharedAccessKeyName, sharedAccessKey, DefaultOperationTimeout, RetryPolicy.Default)
        {
        }

        ConnectionStringBuilder(
            string namespaceName,
            string entityPath,
            string sharedAccessKeyName,
            string sharedAccessKey,
            TimeSpan operationTimeout,
            RetryPolicy retryPolicy)
        {
            if (string.IsNullOrWhiteSpace(namespaceName) || string.IsNullOrWhiteSpace(entityPath))
            {
                throw ExceptionUtility.ArgumentNullOrWhiteSpace(string.IsNullOrWhiteSpace(namespaceName) ? nameof(namespaceName) : nameof(entityPath));
            }
            else if (string.IsNullOrWhiteSpace(sharedAccessKeyName) || string.IsNullOrWhiteSpace(sharedAccessKey))
            {
                throw ExceptionUtility.ArgumentNullOrWhiteSpace(string.IsNullOrWhiteSpace(sharedAccessKeyName) ? nameof(sharedAccessKeyName) : nameof(sharedAccessKey));
            }
            else if (retryPolicy == null)
            {
                throw ExceptionUtility.ArgumentNull(nameof(retryPolicy));
            }

            this.Endpoint = new Uri(EndpointFormat.FormatInvariant(namespaceName));
            this.EntityPath = entityPath;
            this.SasKey = sharedAccessKey;
            this.SasKeyName = sharedAccessKeyName;
            this.OperationTimeout = operationTimeout;
            this.RetryPolicy = retryPolicy;
        }

        /// <summary>
        /// ConnectionString format:
        /// Endpoint=sb://namespace_DNS_Name;EntityPath=EVENT_HUB_NAME;SharedAccessKeyName=SHARED_ACCESS_KEY_NAME;SharedAccessKey=SHARED_ACCESS_KEY
        /// </summary>
        /// <param name="connectionString">ServiceBus ConnectionString</param>
        public ConnectionStringBuilder(string connectionString)
        {
            if (string.IsNullOrWhiteSpace(connectionString))
            {
                throw ExceptionUtility.ArgumentNullOrWhiteSpace(nameof(connectionString));
            }

            this.ParseConnectionString(connectionString);
        }

        public Uri Endpoint { get; set; }

        /// <summary>
        /// Get the shared access policy key value from the connection string
        /// </summary>
        /// <value>Shared Access Signature key</value>
        public string SasKey { get; set; }

        /// <summary>
        /// Get the shared access policy owner name from the connection string
        /// </summary>
        public string SasKeyName { get; set; }

        /// <summary>
        /// Get the entity path value from the connection string
        /// </summary>
        public string EntityPath { get; set; }

        /// <summary>
        /// OperationTimeout is applied in erroneous situations to notify the caller about the relevant <see cref="ServiceBusException"/>
        /// </summary>
        public TimeSpan OperationTimeout { get; set; }

        /// <summary>
        /// Get the retry policy instance that was created as part of this builder's creation.
        /// </summary>
        public RetryPolicy RetryPolicy { get; set; }

        /// <summary>
        /// Returns an interoperable connection string that can be used to connect to ServiceBus Namespace
        /// </summary>
        /// <returns>the connection string</returns>
        public override string ToString()
        {
            var connectionStringBuilder = new StringBuilder();
            if (this.Endpoint != null)
            {
                connectionStringBuilder.Append($"{EndpointConfigName}{KeyValueSeparator}{this.Endpoint}{KeyValuePairDelimiter}");
            }

            if (!string.IsNullOrWhiteSpace(this.EntityPath))
            {
                connectionStringBuilder.Append($"{EntityPathConfigName}{KeyValueSeparator}{this.EntityPath}{KeyValuePairDelimiter}");
            }

            if (!string.IsNullOrWhiteSpace(this.SasKeyName))
            {
                connectionStringBuilder.Append($"{SharedAccessKeyNameConfigName}{KeyValueSeparator}{this.SasKeyName}{KeyValuePairDelimiter}");
            }

            if (!string.IsNullOrWhiteSpace(this.SasKey))
            {
                connectionStringBuilder.Append($"{SharedAccessKeyConfigName}{KeyValueSeparator}{this.SasKey}");
            }

            return connectionStringBuilder.ToString();
        }

        internal EventHubClient CreateEventHubClient()
        {
            // In the future to support other protocols add that logic here.
            return new AmqpEventHubClient(this);
        }

        void ParseConnectionString(string connectionString)
        {
            // First split based on ';'
            string[] keyValuePairs = connectionString.Split(new[] { KeyValuePairDelimiter }, StringSplitOptions.RemoveEmptyEntries);
            foreach (var keyValuePair in keyValuePairs)
            {
                // Now split based on the _first_ '='
                string[] keyAndValue = keyValuePair.Split(new[] { KeyValueSeparator }, 2);
                string key = keyAndValue[0];
                if (keyAndValue.Length != 2)
                {
                    throw ExceptionUtility.Argument(nameof(connectionString), $"Value for the connection string parameter name '{key}' was not found.");
                }

                string value = keyAndValue[1];
                if (key.Equals(EndpointConfigName, StringComparison.OrdinalIgnoreCase))
                {
                    this.Endpoint = new Uri(value);
                }
                else if (key.Equals(EntityPathConfigName, StringComparison.OrdinalIgnoreCase))
                {
                    this.EntityPath = value;
                }
                else if (key.Equals(SharedAccessKeyNameConfigName, StringComparison.OrdinalIgnoreCase))
                {
                    this.SasKeyName = value;
                }
                else if (key.Equals(SharedAccessKeyConfigName, StringComparison.OrdinalIgnoreCase))
                {
                    this.SasKey = value;
                }
                else
                {
                    throw ExceptionUtility.Argument(nameof(connectionString), $"Illegal connection string parameter name '{key}'");
                }
            }
        }
    }
}