// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using System.Threading.Tasks;

    public sealed class EventProcessorHost
    {
        readonly bool initializeLeaseManager;

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
        /// <param name="eventHubPath">The name of the EventHub.</param>
        /// <param name="consumerGroupName">The name of the consumer group within the Event Hub.</param>
        /// <param name="eventHubConnectionString">Connection string for the Event Hub to receive from.</param>
        /// <param name="storageConnectionString">Connection string to Azure Storage account used for leases and checkpointing.</param>
        /// <param name="leaseContainerName">Azure Storage container name for use by built-in lease and checkpoint manager.</param>
        public EventProcessorHost(
            string eventHubPath,
            string consumerGroupName,
            string eventHubConnectionString,
            string storageConnectionString,
            string leaseContainerName)
            : this(EventProcessorHost.CreateHostName(null),
                eventHubPath,
                consumerGroupName,
                eventHubConnectionString,
                storageConnectionString,
                leaseContainerName)
        {
        }

        /// <summary>
        /// Create a new host to process events from an Event Hub.
        /// 
        /// <para>This overload of the constructor uses the default, built-in lease and checkpoint managers.</para>
        /// </summary>
        /// <param name="hostName">A name for this event processor host. See method notes.</param>
        /// <param name="eventHubPath">The name of the EventHub.</param>
        /// <param name="consumerGroupName">The name of the consumer group within the Event Hub.</param>
        /// <param name="eventHubConnectionString">Connection string for the Event Hub to receive from.</param>
        /// <param name="storageConnectionString">Connection string to Azure Storage account used for leases and checkpointing.</param>
        /// <param name="leaseContainerName">Azure Storage container name for use by built-in lease and checkpoint manager.</param>
        public EventProcessorHost(
            string hostName,
            string eventHubPath,
            string consumerGroupName,
            string eventHubConnectionString,
            string storageConnectionString,
            string leaseContainerName)
            : this(hostName,
                eventHubPath,
                consumerGroupName,
                eventHubConnectionString,
                new AzureStorageCheckpointLeaseManager(storageConnectionString, leaseContainerName))
        {
            this.initializeLeaseManager = true;
        }

        /// <summary>
        /// Create a new host to process events from an Event Hub.
        /// 
        /// <para>This overload of the constructor allows maximum flexibility.
        /// This one allows the caller to specify the name of the processor host as well.
        /// The overload also allows the caller to provide their own lease and checkpoint managers to replace the built-in
        /// ones based on Azure Storage.</para>
        /// </summary>
        /// <param name="hostName">Name of the processor host. MUST BE UNIQUE. Strongly recommend including a Guid to ensure uniqueness.</param>
        /// <param name="eventHubPath">The name of the EventHub.</param>
        /// <param name="consumerGroupName">The name of the consumer group within the Event Hub.</param>
        /// <param name="eventHubConnectionString">Connection string for the Event Hub to receive from.</param>
        /// <param name="checkpointManager">Object implementing ICheckpointManager which handles partition checkpointing.</param>
        /// <param name="leaseManager">Object implementing ILeaseManager which handles leases for partitions.</param>
        public EventProcessorHost(
             string hostName,
             string eventHubPath,
             string consumerGroupName,
             string eventHubConnectionString,
             ICheckpointManager checkpointManager,
             ILeaseManager leaseManager)
        {
            if (string.IsNullOrEmpty(consumerGroupName))
            {
                throw new ArgumentNullException(nameof(consumerGroupName));
            }
            else if (checkpointManager == null || leaseManager == null)
            {
                throw new ArgumentNullException(checkpointManager == null ? nameof(checkpointManager) : nameof(leaseManager));
            }

            var csb = new EventHubsConnectionStringBuilder(eventHubConnectionString);
            if (string.IsNullOrEmpty(eventHubPath))
            {
                // Entity path is expected in the connection string if not provided with eventHubPath parameter.
                if (string.IsNullOrEmpty(csb.EntityPath))
                {
                    throw new ArgumentException(nameof(eventHubConnectionString),
                        "Provide EventHub entity path either in eventHubPath parameter or in eventHubConnectionString.");
                }
            }
            else
            {
                // Entity path should not conflict with connection string.
                if (!string.IsNullOrEmpty(csb.EntityPath) &&
                    string.Compare(csb.EntityPath, eventHubPath, StringComparison.OrdinalIgnoreCase) != 0)
                {
                    throw new ArgumentException(nameof(eventHubConnectionString),
                        "Provided EventHub path in eventHubPath parameter conflicts with the path in provided EventHubs connection string.");
                }

                csb.EntityPath = eventHubPath;
            }

            this.HostName = hostName;
            this.EventHubPath = csb.EntityPath;
            this.ConsumerGroupName = consumerGroupName;
            this.EventHubConnectionString = csb.ToString();
            this.CheckpointManager = checkpointManager;
            this.LeaseManager = leaseManager;
            this.Id = $"EventProcessorHost({hostName.Substring(0, Math.Min(hostName.Length, 20))}...)";
            this.PartitionManager = new PartitionManager(this);
            ProcessorEventSource.Log.EventProcessorHostCreated(this.Id, this.EventHubPath);
        }

        // Using this intermediate constructor to create single combined manager to be used as 
        // both lease manager and checkpoint manager.
        EventProcessorHost(
                string hostName,
                string eventHubPath,
                string consumerGroupName,
                string eventHubConnectionString,
                AzureStorageCheckpointLeaseManager combinedManager)
            : this(hostName,
                  eventHubPath,
                  consumerGroupName,
                  eventHubConnectionString,
                  combinedManager,
                  combinedManager)
        {
        }
        
        /// <summary>
        /// Returns processor host name.
        /// If the processor host name was automatically generated, this is the only way to get it.
        /// </summary>
        public string HostName { get; }

        public string EventHubPath { get; }

        public string ConsumerGroupName { get; }

        // All of these accessors are for internal use only.
        internal string EventHubConnectionString { get; private set; }

        internal ICheckpointManager CheckpointManager { get; }

        internal EventProcessorOptions EventProcessorOptions { get; private set; }

        internal ILeaseManager LeaseManager { get; private set; }

        internal IEventProcessorFactory ProcessorFactory { get; private set; }

        internal PartitionManager PartitionManager { get; private set; }

        internal string Id { get; }

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
            var epo = EventProcessorOptions.DefaultOptions;
            epo.ReceiveTimeout = TimeSpan.MinValue;
            return RegisterEventProcessorFactoryAsync(factory, epo);
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
        public async Task RegisterEventProcessorFactoryAsync(IEventProcessorFactory factory, EventProcessorOptions processorOptions)
        {
            if (factory == null || processorOptions == null)
            {
                throw new ArgumentNullException(factory == null ? nameof(factory) : nameof(processorOptions));
            }

            ProcessorEventSource.Log.EventProcessorHostOpenStart(this.Id, factory.GetType().ToString());
            try
            {
                // Override operation timeout by receive timeout?
                if (processorOptions.ReceiveTimeout > TimeSpan.MinValue)
                {
                    var cbs = new EventHubsConnectionStringBuilder(this.EventHubConnectionString);
                    cbs.OperationTimeout = processorOptions.ReceiveTimeout;
                    this.EventHubConnectionString = cbs.ToString();
                }

                if (this.initializeLeaseManager)
                {
                    ((AzureStorageCheckpointLeaseManager)this.LeaseManager).Initialize(this);
                }

                this.ProcessorFactory = factory;
                this.EventProcessorOptions = processorOptions;
                await this.PartitionManager.StartAsync();
            }
            catch (Exception e)
            {
                ProcessorEventSource.Log.EventProcessorHostOpenError(this.Id, e.ToString());
                throw;
            }
            finally
            {
                ProcessorEventSource.Log.EventProcessorHostOpenStop(this.Id);
            }
        }

        /// <summary>
        /// Stop processing events.  Does not return until the shutdown is complete.
        /// </summary>
        /// <returns></returns>
        public async Task UnregisterEventProcessorAsync() // throws InterruptedException, ExecutionException
        {
            ProcessorEventSource.Log.EventProcessorHostCloseStart(this.Id);    	
            try
            {
                await this.PartitionManager.StopAsync();
            }
            catch (Exception e)
            {
                // Log the failure but nothing really to do about it.
                ProcessorEventSource.Log.EventProcessorHostCloseError(this.Id, e.ToString());
                throw;
            }
            finally
            {
                ProcessorEventSource.Log.EventProcessorHostCloseStop(this.Id);
            }
        }

        /// <summary>
        /// Convenience method for generating unique host names, safe to pass to the EventProcessorHost constructors
        /// that take a hostName argument.
        ///  
        /// If a prefix is supplied, the constructed name begins with that string. If the prefix argument is null or
        /// an empty string, the constructed name begins with "host". Then a dash '-' and a unique ID are appended to
        /// create a unique name.
        /// </summary>
        /// <param name="prefix">String to use as the beginning of the name. If null or empty, a default is used.</param>
        /// <returns>A unique host name to pass to EventProcessorHost constructors.</returns>
        static string CreateHostName(string prefix)
        {
            if (string.IsNullOrEmpty(prefix))
            {
                prefix = "host";
            }

            return prefix + "-" + Guid.NewGuid().ToString();
        }
    }
}
