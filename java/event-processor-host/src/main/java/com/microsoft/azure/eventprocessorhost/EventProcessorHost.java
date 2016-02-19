package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.servicebus.ConnectionStringBuilder;

import java.util.UUID;
import java.util.concurrent.*;


public final class EventProcessorHost
{
    private final String hostName;
    private final String namespaceName;
    private final String eventHubPath;
    private final String consumerGroupName;
    private String eventHubConnectionString;

    private ICheckpointManager checkpointManager;
    private ILeaseManager leaseManager;
    private PartitionManager partitionManager;
    private Future<?> partitionManagerFuture = null;
    private IEventProcessorFactory<?> processorFactory;
    private EventProcessorOptions processorOptions;

    // Thread pool is shared among all instances of EventProcessorHost
    // weOwnExecutor exists to support user-supplied thread pools if we add that feature later.
    // executorRefCount is required because the last host must shut down the thread pool if we own it.
    private static ExecutorService executorService = Executors.newCachedThreadPool();
    private static int executorRefCount = 0;
    private static Boolean weOwnExecutor = true;

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
        this("javahost-" + UUID.randomUUID().toString(), namespaceName, eventHubPath, sharedAccessKeyName,
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
        this.hostName = hostName;
        this.namespaceName = namespaceName;
        this.eventHubPath = eventHubPath;
        this.consumerGroupName = consumerGroupName;
        this.checkpointManager = checkpointManager;
        this.leaseManager = leaseManager;
        if (EventProcessorHost.weOwnExecutor)
        {
	        synchronized(EventProcessorHost.weOwnExecutor)
	        {
	        	EventProcessorHost.executorRefCount++;
	        }
        }

        this.eventHubConnectionString = new ConnectionStringBuilder(this.namespaceName, this.eventHubPath,
                sharedAccessKeyName, sharedAccessKey).toString();

        this.partitionManager = new PartitionManager(this);

        if (leaseManager instanceof AzureStorageCheckpointLeaseManager)
        {
            ((AzureStorageCheckpointLeaseManager)leaseManager).setHost(this);
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
    public PartitionManager getPartitionManager() { return this.partitionManager; }
    public IEventProcessorFactory<?> getProcessorFactory() { return this.processorFactory; }
    public String getEventHubPath() { return this.eventHubPath; }
    public String getNamespaceName() { return this.namespaceName; }
    public String getConsumerGroupName() { return this.consumerGroupName; }
    public String getEventHubConnectionString() { return this.eventHubConnectionString; }

    public static ExecutorService getExecutorService() { return EventProcessorHost.executorService; }
    
    public <T extends IEventProcessor> Future<?> registerEventProcessor(Class<T> eventProcessorType)
    {
        DefaultEventProcessorFactory<T> defaultFactory = new DefaultEventProcessorFactory<T>();
        defaultFactory.setEventProcessorClass(eventProcessorType);
        return registerEventProcessorFactory(defaultFactory, EventProcessorOptions.getDefaultOptions());
    }

    public <T extends IEventProcessor> Future<?> registerEventProcessor(Class<T> eventProcessorType, EventProcessorOptions processorOptions)
    {
        DefaultEventProcessorFactory<T> defaultFactory = new DefaultEventProcessorFactory<T>();
        defaultFactory.setEventProcessorClass(eventProcessorType);
        return registerEventProcessorFactory(defaultFactory, processorOptions);
    }

    public Future<?> registerEventProcessorFactory(IEventProcessorFactory<?> factory)
    {
        return registerEventProcessorFactory(factory, EventProcessorOptions.getDefaultOptions());
    }

    public Future<?> registerEventProcessorFactory(IEventProcessorFactory<?> factory, EventProcessorOptions processorOptions)
    {
        this.processorFactory = factory;
        this.processorOptions = processorOptions;
        this.partitionManagerFuture = EventProcessorHost.executorService.submit(this.partitionManager);
        return this.partitionManagerFuture;
    }

    public Future<?> unregisterEventProcessor()
    {
        this.partitionManager.stopPartitions();
        return this.partitionManagerFuture;
    }
    
    public void stopExecutor()
    {
        if (EventProcessorHost.weOwnExecutor)
        {
        	synchronized(EventProcessorHost.weOwnExecutor)
        	{
        		EventProcessorHost.executorRefCount--;
        		if (EventProcessorHost.executorRefCount <= 0)
        		{
        			// It is OK to call shutdown() here even though threads are still running.
        			// Shutdown() causes the executor to stop accepting new tasks, but existing tasks will
        			// run to completion. The pool will terminate when all existing tasks finish.
        			// By this point all new tasks generated by the shutdown have been submitted.
        			EventProcessorHost.executorService.shutdown();
        		}
        	}
        }
    }
    
    void log(String logMessage)
    {
    	// DUMMY STARTS
    	System.out.println(logMessage);
    	// DUMMY ENDS
    }
    
    void logWithHost(String logMessage)
    {
    	log("host " + this.hostName + ": " + logMessage);
    }
    
    void logWithHost(String logMessage, Exception e)
    {
    	log("host " + this.hostName + ": " + logMessage);
    	logWithHost("Caught " + e.toString());
    	StackTraceElement[] stack = e.getStackTrace();
    	for (int i = 0; i < stack.length; i++)
    	{
    		logWithHost(stack[i].toString());
    	}
    }
    
    void logWithHostAndPartition(String partitionId, String logMessage)
    {
    	logWithHost("partition " + partitionId + ": " + logMessage);
    }
    
    void logWithHostAndPartition(String partitionId, String logMessage, Exception e)
    {
    	logWithHostAndPartition(partitionId, logMessage);
    	logWithHostAndPartition(partitionId, "Caught " + e.toString());
    	StackTraceElement[] stack = e.getStackTrace();
    	for (int i = 0; i < stack.length; i++)
    	{
    		logWithHostAndPartition(partitionId, stack[i].toString());
    	}
    }
    
    void logWithHostAndPartition(PartitionContext context, String logMessage)
    {
    	logWithHostAndPartition(context.getPartitionId(), logMessage);
    }
    
    void logWithHostAndPartition(PartitionContext context, String logMessage, Exception e)
    {
    	logWithHostAndPartition(context.getPartitionId(), logMessage, e);
    }
}
