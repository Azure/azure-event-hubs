/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.servicebus.ConnectionStringBuilder;
import com.microsoft.azure.storage.StorageException;

import java.net.URISyntaxException;
import java.security.InvalidKeyException;
import java.util.UUID;
import java.util.concurrent.*;
import java.util.logging.Logger;
import java.util.logging.Level;


public final class EventProcessorHost
{
    private final String hostName;
    private final String namespaceName;
    private final String eventHubPath;
    private final String consumerGroupName;
    private String eventHubConnectionString;

    private ICheckpointManager checkpointManager;
    private ILeaseManager leaseManager;
    private boolean initializeLeaseManager = false; 
    private PartitionManager partitionManager;
    private Future<?> partitionManagerFuture = null;
    private IEventProcessorFactory<?> processorFactory;
    private EventProcessorOptions processorOptions;

    // Thread pool is shared among all instances of EventProcessorHost
    // weOwnExecutor exists to support user-supplied thread pools if we add that feature later.
    // weOwnExecutor is a boxed Boolean so it can be used to synchronize access to these variables.
    // executorRefCount is required because the last host must shut down the thread pool if we own it.
    private static ExecutorService executorService = Executors.newCachedThreadPool();
    private static int executorRefCount = 0;
    private static Boolean weOwnExecutor = true;
    
    public final static String EVENTPROCESSORHOST_TRACE = "eventprocessorhost.trace";
	private static final Logger TRACE_LOGGER = Logger.getLogger(EventProcessorHost.EVENTPROCESSORHOST_TRACE);
    
    
    // DUMMY STARTS
    public static void setDummyPartitionCount(int count)
    {
    	PartitionManager.dummyPartitionCount = count;
    	TRACE_LOGGER.setLevel(Level.FINEST);
    }
    // DUMMY ENDS

    /**
     * Create a new host to process events from an Event Hub.
     * 
     * <p>
     * Since Event Hubs are frequently used for scale-out, high-traffic scenarios, generally there will
     * be only one host per process, and the processes will be run on separate machines. However, it is
     * supported to run multiple hosts on one machine, or even within one process, if throughput is not
     * a concern.
     * <p>
     * This overload of the constructor uses the default, built-in lease and checkpoint managers. The
     * Azure Storage account specified by the storageConnectionString parameter is used by the built-in
     * managers to record leases and checkpoints.
     * 
     * @param namespaceName				The name of the Service Bus namespace in which the Event Hub exists.
     * @param eventHubPath				The path of the Event Hub.
     * @param sharedAccessKeyName		The name of the shared access key to use for authn/authz.
     * @param sharedAccessKey			The shared access key (base64 encoded)
     * @param consumerGroupName			The name of the consumer group within the Event Hub.
     * @param storageConnectionString	Connection string to Azure Storage account used for leases and checkpointing.
     */
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
        this.initializeLeaseManager = true;
    }

    // Because Java won't let you do ANYTHING before calling another constructor. In particular, you can't
    // new up an object and pass it as two parameters of the other constructor.
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

    /**
     * Create a new host to process events from an Event Hub.
     * 
     * This overload of the constructor allows the caller to provide their own lease and checkpoint
     * managers. The first parameters are the same as other overloads.
     * 
     * @param checkpointManager	Object implementing ICheckpointManager which handles partition checkpointing.
     * @param leaseManager		Object implementing ILeaseManager which handles leases for partitions.
     */
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

    /**
     * Create a new host to process events from an Event Hub.
     * 
     * This overload of the constructor allows maximum flexibility. In addition to all the parameters from
     * other overloads, this one allows the caller to specify the name of the processor host. The other overloads
     * automatically generate a unique processor host name. Unless there is a need to include some other
     * information, such as machine name, in the processor host name, it is best to stick to those. 
     * 
     * @param hostName	Name of the processor host. MUST BE UNIQUE. Strongly recommend including a UUID to ensure uniqueness. 
     */
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
        
        logWithHost(Level.INFO, "New EventProcessorHost created");
    }

    /**
     * Returns processor host name.
     * 
     * If the processor host name was automatically generated, this is the only way to get it.
     * 
     * @return	the processor host name
     */
    public String getHostName() { return this.hostName; }

    /**
     * Returns the Event Hub connection string assembled by the processor host.
     * 
     * The connection string is assembled from info provider by the caller to the constructor
     * using ConnectionStringBuilder, so it's not clear that there's any value to making this
     * string accessible.
     * 
     * @return	Event Hub connection string.
     */
    public String getEventHubConnectionString() { return this.eventHubConnectionString; }
    
    /**
     * FOR TESTING USE ONLY
     * 
     */
    public <T extends PartitionPump> void setPumpClass(Class<T> pumpClass)
    {
    	this.partitionManager.setPumpClass(pumpClass);
    }

    /**
     * FOR TESTING USE ONLY
     * 
     */
    public static ExecutorService getExecutorService()
    {
    	return EventProcessorHost.executorService;
    }
    
    // All of these accessors are for internal use only.
    ICheckpointManager getCheckpointManager() { return this.checkpointManager; }
    ILeaseManager getLeaseManager() { return this.leaseManager; }
    PartitionManager getPartitionManager() { return this.partitionManager; }
    IEventProcessorFactory<?> getProcessorFactory() { return this.processorFactory; }
    String getEventHubPath() { return this.eventHubPath; }
    String getConsumerGroupName() { return this.consumerGroupName; }
    
    /**
     * Register class for event processor and start processing.
     *
     * <p>
     * This overload uses the default event processor factory, which simply creates new instances of
     * the registered event processor class, and uses all the default options.
     * <pre>
     * class EventProcessor implements IEventProcessor { ... }
     * EventProcessorHost host = new EventProcessorHost(...);
     * Future<?> foo = host.registerEventProcessor(EventProcessor.class);
     * </pre>
     *  
     * @param eventProcessorType	Class that implements IEventProcessor.
     * @return						Future that does not complete until the processor host shuts down.
     */
    public <T extends IEventProcessor> Future<?> registerEventProcessor(Class<T> eventProcessorType)
    {
        DefaultEventProcessorFactory<T> defaultFactory = new DefaultEventProcessorFactory<T>();
        defaultFactory.setEventProcessorClass(eventProcessorType);
        return registerEventProcessorFactory(defaultFactory, EventProcessorOptions.getDefaultOptions());
    }

    /**
     * Register class for event processor and start processing.
     * 
     * This overload uses the default event processor factory, which simply creates new instances of
     * the registered event processor class, but takes user-specified options.
     *  
     * @param eventProcessorType	Class that implements IEventProcessor.
     * @param processorOptions		Options for the processor host and event processor(s).
     * @return						Future that does not complete until the processor host shuts down.
     */
    public <T extends IEventProcessor> Future<?> registerEventProcessor(Class<T> eventProcessorType, EventProcessorOptions processorOptions)
    {
        DefaultEventProcessorFactory<T> defaultFactory = new DefaultEventProcessorFactory<T>();
        defaultFactory.setEventProcessorClass(eventProcessorType);
        return registerEventProcessorFactory(defaultFactory, processorOptions);
    }

    /**
     * Register user-supplied event processor factory and start processing.
     * 
     * <p>
     * If creating a new event processor requires more work than just new'ing an objects, the user must
     * create an object that implements IEventProcessorFactory and pass it to this method, instead of calling
     * registerEventProcessor.
     * <p>
     * This overload uses default options for the processor host and event processor(s).
     * 
     * @param factory	User-supplied event processor factory object.
     * @return			Future that does not complete until the processor host shuts down.
     */
    public Future<?> registerEventProcessorFactory(IEventProcessorFactory<?> factory)
    {
        return registerEventProcessorFactory(factory, EventProcessorOptions.getDefaultOptions());
    }

    /**
     * Register user-supplied event processor factory and start processing.
     * 
     * This overload takes user-specified options.
     * 
     * @param factory			User-supplied event processor factory object.			
     * @param processorOptions	Options for the processor host and event processor(s).
     * @return					Future that does not complete until the processor host shuts down.
     */
    public Future<?> registerEventProcessorFactory(IEventProcessorFactory<?> factory, EventProcessorOptions processorOptions)
    {
        if (this.initializeLeaseManager)
        {
            try
            {
				((AzureStorageCheckpointLeaseManager)leaseManager).initialize(this);
			}
            catch (InvalidKeyException | URISyntaxException | StorageException e)
            {
            	this.logWithHost(Level.SEVERE, "Failure initializing Storage lease manager", e);
            	throw new RuntimeException("Failure initializing Storage lease manager", e);
			}
        }
        
        logWithHost(Level.INFO, "Starting event processing");
        this.processorFactory = factory;
        this.processorOptions = processorOptions;
        this.partitionManagerFuture = EventProcessorHost.executorService.submit(this.partitionManager);
        return this.partitionManagerFuture;
    }

    /**
     * Stop processing events.
     * 
     * Returns while the shutdown is still in progress. The returned Future is the same as the one returned by
     * registerEventProcessor/registerEventProcessorFactory. If the caller cares, it can be used to check whether
     * shutdown is complete.
     * 
     * @return	Future that does not complete until the processor host shuts down.
     */
    public void unregisterEventProcessor()
    {
    	logWithHost(Level.INFO, "Stopping event processing");
    	
        this.partitionManager.stopPartitions();
        try
        {
			this.partitionManagerFuture.get();
	        if (EventProcessorHost.weOwnExecutor)
	        {
	        	// If there are multiple EventProcessorHosts in one process, only await the shutdown on the last one.
	        	// Otherwise the first one will block forever here.
	        	// This could race with stopExecutor() but that is harmless: it is legal to call awaitTermination()
	        	// at any time, whether executorServer.shutdown() has been called yet or not.
	        	if (EventProcessorHost.executorRefCount <= 0)
	        	{
	        		EventProcessorHost.executorService.awaitTermination(10, TimeUnit.MINUTES);
	        	}
	        }
		}
        catch (InterruptedException | ExecutionException e)
        {
        	// Log the failure but nothing really to do about it.
        	logWithHost(Level.SEVERE, "Failure shutting down", e);
		}
    }
    
    // PartitionManager calls this after all shutdown tasks have been submitted to the ExecutorService.
    void stopExecutor()
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

    
    //
    // Centralized logging.
    //
    
    void log(Level logLevel, String logMessage)
    {
  		EventProcessorHost.TRACE_LOGGER.log(logLevel, logMessage);
    	//System.out.println(logLevel.toString() + ": " + logMessage);
    }
    
    void logWithHost(Level logLevel, String logMessage)
    {
    	log(logLevel, "host " + this.hostName + ": " + logMessage);
    }
    
    void logWithHost(Level logLevel, String logMessage, Exception e)
    {
    	log(logLevel, "host " + this.hostName + ": " + logMessage);
    	logWithHost(logLevel, "Caught " + e.toString());
    	StackTraceElement[] stack = e.getStackTrace();
    	for (int i = 0; i < stack.length; i++)
    	{
    		logWithHost(logLevel, stack[i].toString());
    	}
    	Throwable cause = e.getCause();
    	if ((cause != null) && (cause instanceof Exception))
    	{
    		Exception inner = (Exception)cause;
    		logWithHost(logLevel, "Inner exception " + inner.toString());
    		stack = inner.getStackTrace();
        	for (int i = 0; i < stack.length; i++)
        	{
        		logWithHost(logLevel, stack[i].toString());
        	}
    	}
    }
    
    void logWithHostAndPartition(Level logLevel, String partitionId, String logMessage)
    {
    	logWithHost(logLevel, "partition " + partitionId + ": " + logMessage);
    }
    
    void logWithHostAndPartition(Level logLevel, String partitionId, String logMessage, Exception e)
    {
    	logWithHostAndPartition(logLevel, partitionId, logMessage);
    	logWithHostAndPartition(logLevel, partitionId, "Caught " + e.toString());
    	StackTraceElement[] stack = e.getStackTrace();
    	for (int i = 0; i < stack.length; i++)
    	{
    		logWithHostAndPartition(logLevel, partitionId, stack[i].toString());
    	}
    	Throwable cause = e.getCause();
    	if ((cause != null) && (cause instanceof Exception))
    	{
    		Exception inner = (Exception)cause;
    		logWithHostAndPartition(logLevel, partitionId, "Inner exception " + inner.toString());
    		stack = inner.getStackTrace();
        	for (int i = 0; i < stack.length; i++)
        	{
        		logWithHostAndPartition(logLevel, partitionId, stack[i].toString());
        	}
    	}
    }
    
    void logWithHostAndPartition(Level logLevel, PartitionContext context, String logMessage)
    {
    	logWithHostAndPartition(logLevel, context.getPartitionId(), logMessage);
    }
    
    void logWithHostAndPartition(Level logLevel, PartitionContext context, String logMessage, Exception e)
    {
    	logWithHostAndPartition(logLevel, context.getPartitionId(), logMessage, e);
    }
}
