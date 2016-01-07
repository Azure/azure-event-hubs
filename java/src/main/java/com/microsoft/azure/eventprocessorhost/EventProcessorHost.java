package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.servicebus.ConnectionStringBuilder;

import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;


public final class EventProcessorHost
{
    private final String hostName;
    private final String namespaceName;
    private final String eventHubPath;
    private final String consumerGroupName;
    private String eventHubConnectionString;

    private ExecutorService executorService;

    private ICheckpointManager checkpointManager;
    private ILeaseManager leaseManager;
    private IEventProcessorFactory processorFactory;
    private EventProcessorOptions processorOptions;


    public EventProcessorHost(
            final String namespaceName,
            final String eventHubPath,
            final String sharedAccessKeyName,
            final String sharedAccessKey,
            final String consumerGroupName,
            final String storageConnectionString)
    {
        this(namespaceName, eventHubPath, sharedAccessKeyName, sharedAccessKey, consumerGroupName,
                new AzureStorageCheckpointLeaseManager(storageConnectionString));
    }

    private EventProcessorHost(
            final String namespaceName,
            final String eventHubPath,
            final String sharedAccessKeyName,
            final String sharedAccessKey,
            final String consumerGroupName,
            final AzureStorageCheckpointLeaseManager combinedManager)
    {
        this(namespaceName, eventHubPath, sharedAccessKeyName, sharedAccessKey, consumerGroupName,
                combinedManager, combinedManager);
    }

    public EventProcessorHost(
            final String namespaceName,
            final String eventHubPath,
            final String sharedAccessKeyName,
            final String sharedAccessKey,
            final String consumerGroupName,
            ICheckpointManager checkpointManager,
            ILeaseManager leaseManager)
    {
        this(String.format("javahost-%1$", UUID.randomUUID()), namespaceName, eventHubPath, sharedAccessKeyName,
                sharedAccessKey, consumerGroupName, checkpointManager, leaseManager);
    }

    public EventProcessorHost(
            final String hostName,
            final String namespaceName,
            final String eventHubPath,
            final String sharedAccessKeyName,
            final String sharedAccessKey,
            final String consumerGroupName,
            ICheckpointManager checkpointManager,
            ILeaseManager leaseManager)
    {
        this(hostName, namespaceName, eventHubPath, sharedAccessKeyName, sharedAccessKey, consumerGroupName,
                checkpointManager, leaseManager, Executors.newCachedThreadPool());
    }

    public EventProcessorHost(
            final String hostName,
            final String namespaceName,
            final String eventHubPath,
            final String sharedAccessKeyName,
            final String sharedAccessKey,
            final String consumerGroupName,
            ICheckpointManager checkpointManager,
            ILeaseManager leaseManager,
            ExecutorService executorService)
    {
        this.hostName = hostName;
        this.namespaceName = namespaceName;
        this.eventHubPath = eventHubPath;
        this.consumerGroupName = consumerGroupName;
        this.checkpointManager = checkpointManager;
        this.leaseManager = leaseManager;
        this.executorService = executorService;

        this.eventHubConnectionString = new ConnectionStringBuilder(this.namespaceName, this.eventHubPath,
                sharedAccessKeyName, sharedAccessKey).toString();

        ((IManagerBase)this.checkpointManager).setConsumerGroupName(this.consumerGroupName);
        ((IManagerBase)this.checkpointManager).setEventHubPath(this.eventHubPath);
        ((IManagerBase)this.checkpointManager).setExecutorService(this.executorService);
        if (((IManagerBase)this.checkpointManager).isCombinedManager() == false)
        {
            ((IManagerBase)this.leaseManager).setConsumerGroupName(this.consumerGroupName);
            ((IManagerBase)this.leaseManager).setEventHubPath(this.eventHubPath);
            ((IManagerBase)this.leaseManager).setExecutorService(this.executorService);
        }
    }

    public String getHostName()
    {
        return this.hostName;
    }

    public ICheckpointManager getCheckpointManager()
    {
        return this.checkpointManager;
    }
    public ILeaseManager getLeaseManager() { return this.leaseManager; }

    public void registerEventProcessor()
    {
        registerEventProcessor(EventProcessorOptions.getDefaultOptions());
    }

    public void registerEventProcessor(EventProcessorOptions processorOptions)
    {
        registerEventProcessorFactory(new DefaultEventProcessorFactory(), processorOptions);
    }

    public void registerEventProcessorFactory(IEventProcessorFactory factory)
    {
        registerEventProcessorFactory(factory, EventProcessorOptions.getDefaultOptions());
    }

    public void registerEventProcessorFactory(IEventProcessorFactory factory, EventProcessorOptions processorOptions)
    {
        this.processorFactory = factory;
        this.processorOptions = processorOptions;
        // TODO START PROCESSING
    }

    public void unregisterEventProcessor()
    {
        // TODO SIGNAL this.pump TO STOP PROCESSING AND WAIT
        // TODO MAKE THIS ASYNC AGAIN
    }
}
