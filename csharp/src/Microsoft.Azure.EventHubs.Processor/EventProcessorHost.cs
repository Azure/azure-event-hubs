// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using System.Diagnostics.Tracing;
    using System.Threading;
    using System.Threading.Tasks;

    public sealed class EventProcessorHost
    {
        readonly string namespaceName;
        readonly string sharedAccessKeyName;
        readonly string sharedAccessKey;
        readonly bool initializeLeaseManager;
        Task partitionManagerFuture;

        /// <summary>
        /// Create a new host to process events from an Event Hub.
        /// 
        /// <para>Since Event Hubs are frequently used for scale-out, high-traffic scenarios, generally there will
        /// be only one host per process, and the processes will be run on separate machines. However, it is
        /// supported to run multiple hosts on one machine, or even within one process, if throughput is not
        /// a concern.</para>
        ///
        /// This overload of the constructor uses the default, built-in lease and checkpoint managers. The
        /// Azure Storage account specified by the storageConnectionString parameter is used by the built-in
        /// managers to record leases and checkpoints.
        /// </summary>
        /// <param name="namespaceName">The name of the Service Bus namespace in which the Event Hub exists.</param>
        /// <param name="eventHubPath">The path of the Event Hub.</param>
        /// <param name="sharedAccessKeyName">The name of the shared access key to use for authn/authz.</param>
        /// <param name="sharedAccessKey">The shared access key (base64 encoded)</param>
        /// <param name="consumerGroupName">The name of the consumer group within the Event Hub.</param>
        /// <param name="storageConnectionString">Connection string to Azure Storage account used for leases and checkpointing.</param>
        public EventProcessorHost(
            string namespaceName,
            string eventHubPath,
            string sharedAccessKeyName,
            string sharedAccessKey,
            string consumerGroupName,
            string storageConnectionString)
            : this(namespaceName,
                eventHubPath,
                sharedAccessKeyName,
                sharedAccessKey,
                consumerGroupName,
                new AzureStorageCheckpointLeaseManager(storageConnectionString))
        {
            this.initializeLeaseManager = true;
        }

        /// <summary>
        /// Create a new host to process events from an Event Hub.
        /// 
        /// <para>This overload of the constructor uses the default, built-in lease and checkpoint managers, but
        /// uses a non-default storage container name. The first parameters are the same as the other overloads.</para>
        /// </summary>
        /// <param name="namespaceName">The name of the Service Bus namespace in which the Event Hub exists.</param>
        /// <param name="eventHubPath">The path of the Event Hub.</param>
        /// <param name="sharedAccessKeyName">The name of the shared access key to use for authn/authz.</param>
        /// <param name="sharedAccessKey">The shared access key (base64 encoded)</param>
        /// <param name="consumerGroupName">The name of the consumer group within the Event Hub.</param>
        /// <param name="storageConnectionString">Connection string to Azure Storage account used for leases and checkpointing.</param>
        /// <param name="storageContainerName">Azure Storage container name in which all leases and checkpointing will occur.</param>
        public EventProcessorHost(
            string namespaceName,
            string eventHubPath,
            string sharedAccessKeyName,
            string sharedAccessKey,
            string consumerGroupName,
            string storageConnectionString,
            string storageContainerName)
            : this(namespaceName,
                eventHubPath, 
                sharedAccessKeyName,
                sharedAccessKey,
                consumerGroupName,
                new AzureStorageCheckpointLeaseManager(storageConnectionString, storageContainerName))
        {
            this.initializeLeaseManager = true;
        }
    
        // Because Java won't let you do ANYTHING before calling another constructor. In particular, you can't
        // new up an object and pass it as two parameters of the other constructor.
        EventProcessorHost(
            string namespaceName,
            string eventHubPath,
            string sharedAccessKeyName,
            string sharedAccessKey,
            string consumerGroupName,
            AzureStorageCheckpointLeaseManager combinedManager)
            : this(namespaceName, eventHubPath, sharedAccessKeyName, sharedAccessKey, consumerGroupName, combinedManager, combinedManager)
        {
        }

        /// <summary>
        /// Create a new host to process events from an Event Hub.
        /// 
        /// <para>This overload of the constructor allows the caller to provide their own lease and checkpoint
        /// managers. The first parameters are the same as other overloads.</para>
        /// </summary>
        /// <param name="namespaceName">The name of the Service Bus namespace in which the Event Hub exists.</param>
        /// <param name="eventHubPath">The path of the Event Hub.</param>
        /// <param name="sharedAccessKeyName">The name of the shared access key to use for authn/authz.</param>
        /// <param name="sharedAccessKey">The shared access key (base64 encoded)</param>
        /// <param name="consumerGroupName">The name of the consumer group within the Event Hub.</param>
        /// <param name="checkpointManager">Object implementing ICheckpointManager which handles partition checkpointing.</param>
        /// <param name="leaseManager">Object implementing ILeaseManager which handles leases for partitions.</param>
        public EventProcessorHost(
            string namespaceName,
            string eventHubPath,
            string sharedAccessKeyName,
            string sharedAccessKey,
            string consumerGroupName,
            ICheckpointManager checkpointManager,
            ILeaseManager leaseManager)
            : this(
                "netcorehost-" + Guid.NewGuid(),
                namespaceName,
                eventHubPath,
                sharedAccessKeyName,
                sharedAccessKey,
                consumerGroupName,
                checkpointManager,
                leaseManager)
        {
        }

        /// <summary>
        /// Create a new host to process events from an Event Hub.
        /// 
        /// <para>This overload of the constructor allows maximum flexibility. In addition to all the parameters from
        /// other overloads, this one allows the caller to specify the name of the processor host. The other overloads
        /// automatically generate a unique processor host name. Unless there is a need to include some other
        /// information, such as machine name, in the processor host name, it is best to stick to those.</para>
        /// </summary>
        /// <param name="hostName">Name of the processor host. MUST BE UNIQUE. Strongly recommend including a Guid to ensure uniqueness.</param>
        /// <param name="namespaceName">The name of the Service Bus namespace in which the Event Hub exists.</param>
        /// <param name="eventHubPath">The path of the Event Hub.</param>
        /// <param name="sharedAccessKeyName">The name of the shared access key to use for authn/authz.</param>
        /// <param name="sharedAccessKey">The shared access key (base64 encoded)</param>
        /// <param name="consumerGroupName">The name of the consumer group within the Event Hub.</param>
        /// <param name="checkpointManager">Object implementing ICheckpointManager which handles partition checkpointing.</param>
        /// <param name="leaseManager">Object implementing ILeaseManager which handles leases for partitions.</param>
        public EventProcessorHost(
             string hostName,
             string namespaceName,
             string eventHubPath,
             string sharedAccessKeyName,
             string sharedAccessKey,
             string consumerGroupName,
             ICheckpointManager checkpointManager,
             ILeaseManager leaseManager)
        {   	
            this.HostName = hostName;
            this.namespaceName = namespaceName;
            this.EventHubPath = eventHubPath;
            this.sharedAccessKeyName = sharedAccessKeyName;
            this.sharedAccessKey = sharedAccessKey;
            this.ConsumerGroupName = consumerGroupName;
            this.CheckpointManager = checkpointManager;
            this.LeaseManager = leaseManager;
            this.PartitionManager = new PartitionManager(this);        
            this.LogWithHost(EventLevel.Informational, "New EventProcessorHost created");
        }

        /// <summary>
        /// Returns processor host name.
        /// If the processor host name was automatically generated, this is the only way to get it.
        /// </summary>
        public string HostName { get; }

        /// <summary>
        /// Returns the Event Hub connection string assembled by the processor host.
        /// <para>The connection string is assembled from info provider by the caller to the constructor
        /// using ConnectionStringBuilder, so it's not clear that there's any value to making this
        /// string accessible.</para>
        /// </summary>
        internal string EventHubConnectionString { get; private set; }

        public string EventHubPath { get; }

        public string ConsumerGroupName { get; }

        // All of these accessors are for internal use only.
        internal ICheckpointManager CheckpointManager { get; }

        internal EventProcessorOptions EventProcessorOptions { get; private set; }

        internal ILeaseManager LeaseManager { get; private set; }

        internal IEventProcessorFactory ProcessorFactory { get; private set; }

        internal PartitionManager PartitionManager { get; private set; }

        /// <summary>
        /// This registers <see cref="IEventProcessor"/> implementation with the host using <see cref="DefaultEventProcessorFactory{T}"/>.  
        /// This also starts the host and causes it to start participating in the partition distribution process.
        /// </summary>
        /// <typeparam name="T">Implementation of your application specific <see cref="IEventProcessor"/>.</typeparam>
        /// <returns>A task to indicate EventProcessorHost instance is started.</returns>
        public Task RegisterEventProcessorAsync<T>() where T : IEventProcessor, new()
        {
            return RegisterEventProcessorAsync<T>(EventProcessorOptions.DefaultOptions);
        }

        /// <summary>
        /// This registers <see cref="IEventProcessor"/> implementation with the host using <see cref="DefaultEventProcessorFactory{T}"/>.  
        /// This also starts the host and causes it to start participating in the partition distribution process.
        /// </summary>
        /// <typeparam name="T">Implementation of your application specific <see cref="IEventProcessor"/>.</typeparam>
        /// <param name="processorOptions"><see cref="EventProcessorOptions"/> to control various aspects of message pump created when ownership 
        /// is acquired for a particular partition of EventHub.</param>
        /// <returns>A task to indicate EventProcessorHost instance is started.</returns>
        public Task RegisterEventProcessorAsync<T>(EventProcessorOptions processorOptions) where T : IEventProcessor, new()
        {
            IEventProcessorFactory f = new DefaultEventProcessorFactory<T>();
            return RegisterEventProcessorFactoryAsync(f, processorOptions);
        }

        /// <summary>
        /// This registers <see cref="IEventProcessorFactory"/> implementation with the host which is used to create an instance of 
        /// <see cref="IEventProcessor"/> when it takes ownership of a partition.  This also starts the host and causes it to start participating 
        /// in the partition distribution process.
        /// </summary>
        /// <param name="factory">Instance of <see cref="IEventProcessorFactory"/> implementation.</param>
        /// <returns>A task to indicate EventProcessorHost instance is started.</returns>
        public Task RegisterEventProcessorFactoryAsync(IEventProcessorFactory factory)
        {
            return RegisterEventProcessorFactoryAsync(factory, EventProcessorOptions.DefaultOptions);
        }

        /// <summary>
        /// This registers <see cref="IEventProcessorFactory"/> implementation with the host which is used to create an instance of 
        /// <see cref="IEventProcessor"/> when it takes ownership of a partition.  This also starts the host and causes it to start participating 
        /// in the partition distribution process.
        /// </summary>
        /// <param name="factory">Instance of <see cref="IEventProcessorFactory"/> implementation.</param>
        /// <param name="processorOptions"><see cref="EventProcessorOptions"/> to control various aspects of message pump created when ownership 
        /// is acquired for a particular partition of EventHub.</param>
        /// <returns>A task to indicate EventProcessorHost instance is started.</returns>
        public Task RegisterEventProcessorFactoryAsync(IEventProcessorFactory factory, EventProcessorOptions processorOptions)
        {
    	    // TODO: set the timeout
            this.EventHubConnectionString = new ServiceBusConnectionSettings(
                this.namespaceName,
                this.EventHubPath,
                this.sharedAccessKeyName,
                this.sharedAccessKey
                /*, processorOptions.ReceiveTimeOut, RetryPolicy.Default*/).ToString();

            if (this.initializeLeaseManager)
            {
                try
                {
				    ((AzureStorageCheckpointLeaseManager)this.LeaseManager).Initialize(this);
			    }
                catch (Exception e) //when (e is InvalidKeyException || e is URISyntaxException || e is StorageException)
                {
            	    this.LogWithHost(EventLevel.Error, "Failure initializing Storage lease manager", e);
            	    throw new EventProcessorRuntimeException(e.Message, "Initializing Storage lease manager", e);
			    }
            }
        
            this.LogWithHost(EventLevel.Informational, "Starting event processing");
            this.ProcessorFactory = factory;
            this.EventProcessorOptions = processorOptions;
            this.partitionManagerFuture = this.PartitionManager.RunAsync();        
            return this.partitionManagerFuture;
        }

        /// <summary>
        /// Stop processing events.  Does not return until the shutdown is complete.
        /// </summary>
        /// <returns></returns>
        public async Task UnregisterEventProcessorAsync() // throws InterruptedException, ExecutionException
        {
    	    this.LogWithHost(EventLevel.Informational, "Stopping event processing");
    	
            this.PartitionManager.StopPartitions();
            try
            {
                await this.partitionManagerFuture;
		    }
            catch (Exception e) // when (e is InterruptedException || e is ExecutionException)
            {
        	    // Log the failure but nothing really to do about it.
        	    this.LogWithHost(EventLevel.Error, "Failure shutting down", e);
        	    throw;
		    }
        }


        //
        // Centralized logging.
        //

        internal void Log(EventLevel logLevel, string logMessage)
        {
            // TODO: EventProcessorHost.TRACE_LOGGER.log(logLevel, logMessage);
            Console.WriteLine(logLevel + ": " + logMessage);
        }

        internal void LogWithHost(EventLevel logLevel, string logMessage)
        {
            Log(logLevel, "host " + this.HostName + ": " + logMessage);
        }

        internal void LogWithHost(EventLevel logLevel, string logMessage, Exception e)
        {
            LogWithHost(logLevel, logMessage + ". Caught " + e);
        }

        internal void LogWithHostAndPartition(EventLevel logLevel, string partitionId, string logMessage)
        {
            LogWithHost(logLevel, "partition " + partitionId + ": " + logMessage);
        }

        internal void LogWithHostAndPartition(EventLevel logLevel, string partitionId, string logMessage, Exception e)
        {
            LogWithHostAndPartition(logLevel, partitionId, logMessage + ". Caught " + e);
        }

        internal void LogWithHostAndPartition(EventLevel logLevel, PartitionContext context, string logMessage)
        {
            LogWithHostAndPartition(logLevel, context.PartitionId, logMessage);
        }

        internal void LogWithHostAndPartition(EventLevel logLevel, PartitionContext context, string logMessage, Exception e)
        {
            LogWithHostAndPartition(logLevel, context.PartitionId, logMessage, e);
        }
    }
}
