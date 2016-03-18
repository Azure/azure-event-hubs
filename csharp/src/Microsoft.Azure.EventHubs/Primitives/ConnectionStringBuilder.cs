// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;

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
        private ConnectionStringBuilder(
            string namespaceName,
            string entityPath,
            string sharedAccessKeyName,
            string sharedAccessKey,
            TimeSpan operationTimeout,
            RetryPolicy retryPolicy)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Build a connection string consumable by <see cref="EventHubClient.Create(string)"/>
        /// </summary>
        /// <param name="namespaceName">Namespace name (the dns suffix, ex: .servicebus.windows.net, is not required)</param>
        /// <param name="entityPath">Entity path. For eventHubs case specify eventHub name.</param>
        /// <param name="sharedAccessKeyName">Shared Access Key name</param>
        /// <param name="sharedAccessKey">Shared Access Key</param>
        public ConnectionStringBuilder(string namespaceName, string entityPath, string sharedAccessKeyName, string sharedAccessKey)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// ConnectionString format:
        /// Endpoint=sb://namespace_DNS_Name;EntityPath=EVENT_HUB_NAME;SharedAccessKeyName=SHARED_ACCESS_KEY_NAME;SharedAccessKey=SHARED_ACCESS_KEY
        /// </summary>
        /// <param name="connectionString">ServiceBus ConnectionString</param>
        public ConnectionStringBuilder(string connectionString)
        {
            throw new NotImplementedException();
        }

        internal Uri Endpoint
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        /// <summary>
        /// Get the shared access policy key value from the connection string
        /// </summary>
        /// <value>Shared Access Signature key</value>
        string SasKey
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        /// <summary>
        /// Get the shared access policy owner name from the connection string
        /// </summary>
        public string SasKeyName
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        /// <summary>
        /// Get the entity path value from the connection string
        /// </summary>
        public string EntityPath
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        /// <summary>
        /// OperationTimeout is applied in erroneous situations to notify the caller about the relevant <see cref="ServiceBusException"/>
        /// </summary>
        public TimeSpan OperationTimeout
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        /// <summary>
        /// Get the retry policy instance that was created as part of this builder's creation.
        /// </summary>
        public RetryPolicy RetryPolicy
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        /// <summary>
        /// Returns an interoperable connection string that can be used to connect to ServiceBus Namespace
        /// </summary>
        /// <returns>the connection string</returns>
        public override string ToString()
        {
            throw new NotImplementedException();
        }
    }
}