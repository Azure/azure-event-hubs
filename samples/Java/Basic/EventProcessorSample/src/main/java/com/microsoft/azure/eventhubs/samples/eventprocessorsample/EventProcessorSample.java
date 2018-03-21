/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.eventhubs.samples.eventprocessorsample;

import com.microsoft.azure.eventhubs.ConnectionStringBuilder;
import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventprocessorhost.CloseReason;
import com.microsoft.azure.eventprocessorhost.EventProcessorHost;
import com.microsoft.azure.eventprocessorhost.EventProcessorOptions;
import com.microsoft.azure.eventprocessorhost.ExceptionReceivedEventArgs;
import com.microsoft.azure.eventprocessorhost.IEventProcessor;
import com.microsoft.azure.eventprocessorhost.PartitionContext;

import java.util.concurrent.ExecutionException;
import java.util.function.Consumer;

public class EventProcessorSample
{
    public static void main(String args[]) throws InterruptedException, ExecutionException
    {
    	// SETUP SETUP SETUP SETUP
    	// Fill these strings in with the information of the Event Hub you wish to use. The consumer group
    	// can probably be left as-is. You will also need the connection string for an Azure Storage account,
    	// which is used to persist the lease and checkpoint data for this Event Hub. The Storage container name
    	// indicates where the blobs used to implement leases and checkpoints will be placed within the Storage
    	// account. All instances of EventProcessorHost which will be consuming from the same Event Hub and consumer
    	// group must use the same Azure Storage account and container name.
    	String consumerGroupName = "$Default";
    	String namespaceName = "qstest1";
    	String eventHubName = "qshub2";
    	String sasKeyName = "RootManageSharedAccessKey";
    	String sasKey = "6Loqtm2EkuvqnQLuQxuyUy16hLmLPidIadKtjdR6ORo=";
    	String storageConnectionString = "DefaultEndpointsProtocol=https;AccountName=mystgaccttest;AccountKey=SXeg8wdUSux5iTcaXQ4kVoO0gOfD6hFmZeW7rFyrmUW8SwHrPDr43nOnAAZ8odXzjzeLSBjY89X3D8QVHReUwg==;EndpointSuffix=core.windows.net";
    	String storageContainerName = "containertest3";
    	String hostNamePrefix = "mystgaccttest";
    	
    	// To conveniently construct the Event Hub connection string from the raw information, use the ConnectionStringBuilder class.
    	ConnectionStringBuilder eventHubConnectionString = new ConnectionStringBuilder()
    			.setNamespaceName(namespaceName)
    			.setEventHubName(eventHubName)
    			.setSasKeyName(sasKeyName)
    			.setSasKey(sasKey);
    	
		// Create the instance of EventProcessorHost using the most basic constructor. This constructor uses Azure Storage for
		// persisting partition leases and checkpoints. The host name, which identifies the instance of EventProcessorHost, must be unique.
		// You can use a plain UUID, or use the createHostName utility method which appends a UUID to a supplied string.
		EventProcessorHost host = new EventProcessorHost(
				EventProcessorHost.createHostName(hostNamePrefix),
				eventHubName,
				consumerGroupName,
				eventHubConnectionString.toString(),
				storageConnectionString,
				storageContainerName);
		
		// Registering an event processor class with an instance of EventProcessorHost starts event processing. The host instance
		// obtains leases on some partitions of the Event Hub, possibly stealing some from other host instances, in a way that
		// converges on an even distribution of partitions across all host instances. For each leased partition, the host instance
		// creates an instance of the provided event processor class, then receives events from that partition and passes them to
		// the event processor instance.
		//
		// There are two error notification systems in EventProcessorHost. Notification of errors tied to a particular partition,
		// such as a receiver failing, are delivered to the event processor instance for that partition via the onError method.
		// Notification of errors not tied to a particular partition, such as initialization failures, are delivered to a general
		// notification handler that is specified via an EventProcessorOptions object. You are not required to provide such a
		// notification handler, but if you don't, then you may not know that certain errors have occurred.
		System.out.println("Registering host named " + host.getHostName());
		EventProcessorOptions options = new EventProcessorOptions();
		options.setExceptionNotification(new ErrorNotificationHandler());

		host.registerEventProcessor(EventProcessor.class, options)
		.whenComplete((unused, e) ->
		{
			// whenComplete passes the result of the previous stage through unchanged,
			// which makes it useful for logging a result without side effects.
			if (e != null)
			{
				System.out.println("Failure while registering: " + e.toString());
				if (e.getCause() != null)
				{
					System.out.println("Inner exception: " + e.getCause().toString());
				}
			}
		})
		.thenAccept((unused) ->
		{
			// This stage will only execute if registerEventProcessor succeeded.
			// If it completed exceptionally, this stage will be skipped.
			System.out.println("Press enter to stop.");
			try 
			{
				System.in.read();
			}
			catch (Exception e)
			{
				System.out.println("Keyboard read failed: " + e.toString());
			}
		})
		.thenCompose((unused) ->
		{
			// This stage will only execute if registerEventProcessor succeeded.
			//
            // Processing of events continues until unregisterEventProcessor is called. Unregistering shuts down the
            // receivers on all currently owned leases, shuts down the instances of the event processor class, and
            // releases the leases for other instances of EventProcessorHost to claim.
			return host.unregisterEventProcessor();
		})
		.exceptionally((e) ->
		{
			System.out.println("Failure while unregistering: " + e.toString());
			if (e.getCause() != null)
			{
				System.out.println("Inner exception: " + e.getCause().toString());
			}
			return null;
		})
		.get(); // Wait for everything to finish before exiting main!
		
        System.out.println("End of sample");
    }
    
    // The general notification handler is an object that derives from Consumer<> and takes an ExceptionReceivedEventArgs object
    // as an argument. The argument provides the details of the error: the exception that occurred and the action (what EventProcessorHost
    // was doing) during which the error occurred. The complete list of actions can be found in EventProcessorHostActionStrings.
    public static class ErrorNotificationHandler implements Consumer<ExceptionReceivedEventArgs>
    {
		@Override
		public void accept(ExceptionReceivedEventArgs t)
		{
			System.out.println("SAMPLE: Host " + t.getHostname() + " received general error notification during " + t.getAction() + ": " + t.getException().toString());
		}
    }

    public static class EventProcessor implements IEventProcessor
    {
    	private int checkpointBatchingCount = 0;

    	// OnOpen is called when a new event processor instance is created by the host. In a real implementation, this
    	// is the place to do initialization so that events can be processed when they arrive, such as opening a database
    	// connection.
    	@Override
        public void onOpen(PartitionContext context) throws Exception
        {
        	System.out.println("SAMPLE: Partition " + context.getPartitionId() + " is opening");
        }

        // OnClose is called when an event processor instance is being shut down. The reason argument indicates whether the shut down
        // is because another host has stolen the lease for this partition or due to error or host shutdown. In a real implementation,
        // this is the place to do cleanup for resources that were opened in onOpen.
    	@Override
        public void onClose(PartitionContext context, CloseReason reason) throws Exception
        {
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " is closing for reason " + reason.toString());
        }
    	
    	// onError is called when an error occurs in EventProcessorHost code that is tied to this partition, such as a receiver failure.
    	// It is NOT called for exceptions thrown out of onOpen/onClose/onEvents. EventProcessorHost is responsible for recovering from
    	// the error, if possible, or shutting the event processor down if not, in which case there will be a call to onClose. The
    	// notification provided to onError is primarily informational.
    	@Override
    	public void onError(PartitionContext context, Throwable error)
    	{
    		System.out.println("SAMPLE: Partition " + context.getPartitionId() + " onError: " + error.toString());
    	}

    	// onEvents is called when events are received on this partition of the Event Hub. The maximum number of events in a batch
    	// can be controlled via EventProcessorOptions. Also, if the "invoke processor after receive timeout" option is set to true,
    	// this method will be called with null when a receive timeout occurs.
    	@Override
        public void onEvents(PartitionContext context, Iterable<EventData> events) throws Exception
        {
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " got event batch");
            int eventCount = 0;
            for (EventData data : events)
            {
            	// It is important to have a try-catch around the processing of each event. Throwing out of onEvents deprives
            	// you of the chance to process any remaining events in the batch. 
            	try
            	{
	                System.out.println("SAMPLE (" + context.getPartitionId() + "," + data.getSystemProperties().getOffset() + "," +
	                		data.getSystemProperties().getSequenceNumber() + "): " + new String(data.getBytes(), "UTF8"));
	                eventCount++;
	                
	                // Checkpointing persists the current position in the event stream for this partition and means that the next
	                // time any host opens an event processor on this event hub+consumer group+partition combination, it will start
	                // receiving at the event after this one. Checkpointing is usually not a fast operation, so there is a tradeoff
	                // between checkpointing frequently (to minimize the number of events that will be reprocessed after a crash, or
	                // if the partition lease is stolen) and checkpointing infrequently (to reduce the impact on event processing
	                // performance). Checkpointing every five events is an arbitrary choice for this sample.
	                this.checkpointBatchingCount++;
	                if ((checkpointBatchingCount % 5) == 0)
	                {
	                	System.out.println("SAMPLE: Partition " + context.getPartitionId() + " checkpointing at " +
	               			data.getSystemProperties().getOffset() + "," + data.getSystemProperties().getSequenceNumber());
	                	// Checkpoints are created asynchronously. It is important to wait for the result of checkpointing
	                	// before exiting onEvents or before creating the next checkpoint, to detect errors and to ensure proper ordering.
	                	context.checkpoint(data).get();
	                }
            	}
            	catch (Exception e)
            	{
            		System.out.println("Processing failed for an event: " + e.toString());
            	}
            }
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " batch size was " + eventCount + " for host " + context.getOwner());
        }
    }
}



