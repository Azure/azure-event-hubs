package com.microsoft.azure.eventprocessorhost;

import java.util.UUID;


public final class EventProcessorHost
{
    private final String hostName;
    private final String eventHubPath;
    private final String consumerGroupName;
    private final String eventHubConnectionString;

    private ICheckpointManager checkpointManager;
    private ILeaseManager leaseManager;
    private IEventProcessorFactory processorFactory;
    private EventProcessorOptions processorOptions;
    private EventPump pump;

    public EventProcessorHost(
            final String eventHubPath,
            final String consumerGroupName,
            final String eventHubConnectionString,
            ICheckpointManager checkpointManager,
            ILeaseManager leaseManager)
    {
        this(String.format("javahost-%1$", UUID.randomUUID()), eventHubPath, consumerGroupName,
                eventHubConnectionString, checkpointManager, leaseManager);
    }

    public EventProcessorHost(
            final String hostName,
            final String eventHubPath,
            final String consumerGroupName,
            final String eventHubConnectionString,
            ICheckpointManager checkpointManager,
            ILeaseManager leaseManager)
    {
        this.hostName = hostName;
        this.eventHubPath = eventHubPath;
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
        this.pump = new EventPump();
        // TODO START PROCESSING -- Create an Executor to run the pump or whatever
    }

    public void unregisterEventProcessor()
    {
        // TODO SIGNAL this.pump TO STOP PROCESSING AND WAIT
    }


    private class EventPump implements Runnable
    {
        public void run()
        {

        }
    }
}
