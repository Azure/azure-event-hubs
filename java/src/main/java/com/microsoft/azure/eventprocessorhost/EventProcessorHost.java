package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.servicebus.ConnectionStringBuilder;

import java.util.HashMap;
import java.util.UUID;
import java.util.concurrent.*;


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
    private PartitionManager partitionManager;
    private IEventProcessorFactory processorFactory;
    private EventProcessorOptions processorOptions;

    private Pump pump = null;

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

        this.partitionManager = new PartitionManager(this);

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
    public ExecutorService getExecutorService() { return this.executorService; }
    public PartitionManager getPartitionManager() { return this.partitionManager; }

    public <T extends IEventProcessor> Future<Void> registerEventProcessor(Class<T> eventProcessorType)
    {
        DefaultEventProcessorFactory<T> defaultFactory = new DefaultEventProcessorFactory<T>();
        defaultFactory.setEventProcessorClass(eventProcessorType);
        return registerEventProcessorFactory(defaultFactory, EventProcessorOptions.getDefaultOptions());
    }

    public <T extends IEventProcessor> Future<Void> registerEventProcessor(Class<T> eventProcessorType, EventProcessorOptions processorOptions)
    {
        DefaultEventProcessorFactory<T> defaultFactory = new DefaultEventProcessorFactory<T>();
        defaultFactory.setEventProcessorClass(eventProcessorType);
        return registerEventProcessorFactory(defaultFactory, processorOptions);
    }

    public Future<Void> registerEventProcessorFactory(IEventProcessorFactory factory)
    {
        return registerEventProcessorFactory(factory, EventProcessorOptions.getDefaultOptions());
    }

    public Future<Void> registerEventProcessorFactory(IEventProcessorFactory factory, EventProcessorOptions processorOptions)
    {
        this.processorFactory = factory;
        this.processorOptions = processorOptions;
        return this.executorService.submit(new PumpStartupCallable());
    }

    public Future<?> unregisterEventProcessor()
    {
        return this.pump.requestPumpStop();
    }


    private class PumpStartupCallable implements Callable<Void>
    {
        // This method is running in its own thread and can block during startup without causing trouble.
        // Before exiting, it starts the pump manager.
        // When this method returns, that signals the Future returned from Register* and indicates to the user that
        // EPH startup is sufficiently complete and the pump is running. User is not required to care about this,
        // but the info is available if desired.
        public Void call()
        {
            try
            {
                Pump pump = new Pump(EventProcessorHost.this);
                EventProcessorHost.this.pump = pump;
                pump.doStartupTasks();
                pump.doPump();
            }
            catch (Exception e)
            {
                // DUMMY STARTS
                System.out.println("Exception from pump " + e.toString());
                // DUMMY ENDS
            }
            return null;
        }
    }
}
