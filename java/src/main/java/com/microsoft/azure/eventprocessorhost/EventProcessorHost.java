package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.servicebus.ConnectionStringBuilder;

import java.util.ArrayList;
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
    private IEventProcessorFactory processorFactory;
    private EventProcessorOptions processorOptions;

    private PumpManager pumpManager = null;

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

    public <T extends IEventProcessor> Future<Void> registerEventProcessor(Class<T> eventProcessorType)
    {
        return registerEventProcessorFactory(new DefaultEventProcessorFactory<T>(), EventProcessorOptions.getDefaultOptions());
    }

    public <T extends IEventProcessor> Future<Void> registerEventProcessor(Class<T> eventProcessorType, EventProcessorOptions processorOptions)
    {
        return registerEventProcessorFactory(new DefaultEventProcessorFactory<T>(), processorOptions);
    }

    public Future<Void> registerEventProcessorFactory(IEventProcessorFactory factory)
    {
        return registerEventProcessorFactory(factory, EventProcessorOptions.getDefaultOptions());
    }

    public Future<Void> registerEventProcessorFactory(IEventProcessorFactory factory, EventProcessorOptions processorOptions)
    {
        this.processorFactory = factory;
        this.processorOptions = processorOptions;
        return this.executorService.submit(new PumpCallable());
    }

    public Future<Void> unregisterEventProcessor()
    {
        return this.pumpManager.stopPump();
    }


    private class PumpCallable implements Callable<Void>
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
                PumpManager pumpManager = new PumpManager();
                EventProcessorHost.this.pumpManager = pumpManager;
                pumpManager.setupInitialLeases();
                pumpManager.startManagingPump();
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

    private class PumpManager implements Callable<Void>
    {
        private HashMap<String, Lease> leases;
        private Future<Void> pumpManagerThreadFuture;

        public PumpManager()
        {
            this.leases = new HashMap<String, Lease>();
        }

        public void setupInitialLeases() throws Exception
        {
            if (!EventProcessorHost.this.leaseManager.leaseStoreExists().get())
            {
                if (!EventProcessorHost.this.leaseManager.createLeaseStoreIfNotExists().get())
                {
                    // DUMMY STARTS
                    throw new Exception("couldn't create lease store");
                    // DUMMY ENDS
                }

                // Determine how many partitions there are, create leases for them, and acquire those leases
                // DUMMY STARTS
                ArrayList<String> partitionIds = new ArrayList<String>();
                partitionIds.add("0");
                partitionIds.add("1");
                partitionIds.add("2");
                partitionIds.add("3");
                for (String id : partitionIds)
                {
                    EventProcessorHost.this.leaseManager.createLeaseIfNotExists(id).get();
                    Lease gotLease = EventProcessorHost.this.leaseManager.acquireLease(id).get();
                    if (gotLease != null)
                    {
                        this.leases.put(id, gotLease);
                    }
                }
                // DUMMY ENDS
            }
            else
            {
                // Get all the leases

                // If any are expired, take those

                // TODO grab more if needed for load balancing
            }
        }

        public void startManagingPump()
        {
            this.pumpManagerThreadFuture = EventProcessorHost.this.executorService.submit(this);
        }

        public Future<Void> stopPump()
        {
            // TODO set a flag or something
            return this.pumpManagerThreadFuture;
        }

        public Void call()
        {
            // Start a pump for each successfully acquired lease
            // DUMMY STARTS
            for (String partitionId : this.leases.keySet())
            {
                Lease lease = this.leases.get(partitionId);
                //IEventProcessor processorForLease = EventProcessorHost.this.processorFactory.createEventProcessor()
            }
            // DUMMY ENDS

            return null;
        }
    }
}
