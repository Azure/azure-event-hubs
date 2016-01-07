package com.microsoft.azure.eventprocessorhost;

import java.util.UUID;


public final class EventProcessorHost
{
    private final String hostName;
    private final String eventHubConnectionString;
    private final String consumerGroupName;

    private ICheckpointManager checkpointManager;
    private ILeaseManager leaseManager;
    private IEventProcessorFactory processorFactory;
    private EventProcessorOptions processorOptions;

    public EventProcessorHost(
            final String eventHubConnectionString,
            final String consumerGroupName,
            ICheckpointManager checkpointManager,
            ILeaseManager leaseManager)
    {
        this(String.format("javahost-%1$", UUID.randomUUID()), consumerGroupName,
                eventHubConnectionString, checkpointManager, leaseManager);
    }

    public EventProcessorHost(
            final String hostName,
            final String eventHubConnectionString,
            final String consumerGroupName,
            ICheckpointManager checkpointManager,
            ILeaseManager leaseManager)
    {
        this.hostName = hostName;
        this.consumerGroupName = consumerGroupName;
        this.eventHubConnectionString = eventHubConnectionString;
        this.checkpointManager = checkpointManager;
        this.leaseManager = leaseManager;
    }

    public String getHostName()
    {
        return this.hostName;
    }

    // Caller will need to know the actual type of the object, but should be able to cast
    // back to it and not have to save a reference.
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
