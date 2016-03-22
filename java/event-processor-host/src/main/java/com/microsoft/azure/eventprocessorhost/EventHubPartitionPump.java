/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.io.IOException;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;

import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.PartitionReceiveHandler;
import com.microsoft.azure.eventhubs.PartitionReceiver;
import com.microsoft.azure.servicebus.ReceiverDisconnectedException;
import com.microsoft.azure.servicebus.ServiceBusException;

class EventHubPartitionPump extends PartitionPump
{
    private CompletableFuture<?> internalOperationFuture = null;
    
	private EventHubClient eventHubClient = null;
	private PartitionReceiver partitionReceiver = null;
    private InternalReceiveHandler internalReceiveHandler = null;

    //
    // The base initialize() is fine as-is, no need to override.
    //

    @Override
    public void specializedStartPump()
    {
        try
        {
			openClients();
		}
        catch (Exception e)
        {
        	if ((e instanceof ExecutionException) && (e.getCause() instanceof ReceiverDisconnectedException))
        	{
        		// DUMMY This is probably due to a receiver with a higher epoch
        		// Is there a way to be sure without checking the exception text?
        		this.host.logWithHostAndPartition(this.partitionContext, "Receiver disconnected on create, bad epoch?", e);
        		// DUMMY ENDS
        	}
        	else
        	{
				// DUMMY figure out the retry policy here
				this.host.logWithHostAndPartition(this.partitionContext, "Failure creating client or receiver", e);
				// DUMMY ENDS
        	}
			this.pumpStatus = PartitionPumpStatus.PP_OPENFAILED;
		}

        if (this.pumpStatus == PartitionPumpStatus.PP_OPENING)
        {
            this.internalReceiveHandler = new InternalReceiveHandler();
            // onOpen is called from the base class and must have returned in order for execution to reach here, 
            // meaning it is safe to set the handler and start calling onEvents.
            // Set the status to running before setting the handler, so the handler can never race and see the status != running.
            this.pumpStatus = PartitionPumpStatus.PP_RUNNING;
            this.partitionReceiver.setReceiveHandler(this.internalReceiveHandler);
        }
        
        if (this.pumpStatus == PartitionPumpStatus.PP_OPENFAILED)
        {
        	this.pumpStatus = PartitionPumpStatus.PP_CLOSING;
        	cleanUpClients();
        	this.pumpStatus = PartitionPumpStatus.PP_CLOSED;
        }
    }
    
    private void openClients() throws ServiceBusException, IOException, InterruptedException, ExecutionException
    {
    	// Create new client/receiver
    	this.host.logWithHostAndPartition(this.partitionContext, "Opening EH client");
		this.internalOperationFuture = EventHubClient.createFromConnectionString(this.host.getEventHubConnectionString());
		this.eventHubClient = (EventHubClient) this.internalOperationFuture.get();
		this.internalOperationFuture = null;
		
    	String startingOffset = this.partitionContext.getStartingOffset();
    	long epoch = this.lease.getEpoch();
    	this.host.logWithHostAndPartition(this.partitionContext, "Opening EH receiver with epoch " + epoch + " at offset " + startingOffset);
		this.internalOperationFuture = this.eventHubClient.createEpochReceiver(this.partitionContext.getConsumerGroupName(), this.partitionContext.getPartitionId(), startingOffset, epoch);
		this.lease.setEpoch(epoch);
		this.partitionReceiver = (PartitionReceiver) this.internalOperationFuture.get();
		this.internalOperationFuture = null;
		
        this.host.logWithHostAndPartition(this.partitionContext, "EH client and receiver creation finished");
    }
    
    private void cleanUpClients() // swallows all exceptions
    {
        if (this.partitionReceiver != null)
        {
    		// Taking the lock means that there is no onEvents call in progress.
        	synchronized (this.processingSynchronizer)
        	{
        		// Disconnect the processor from the receiver we're about to close.
        		// Fortunately this is idempotent -- setting the handler to null when it's already been
        		// nulled by code elsewhere is harmless!
        		this.partitionReceiver.setReceiveHandler(null);
        	}
        	
        	this.host.logWithHostAndPartition(this.partitionContext, "Closing EH receiver");
        	this.partitionReceiver.close();
        	this.partitionReceiver = null;
        }
        
        if (this.eventHubClient != null)
        {
        	this.host.logWithHostAndPartition(this.partitionContext, "Closing EH client");
        	this.eventHubClient.close();
        	this.eventHubClient = null;
        }
    }

    @Override
    public void specializedShutdown(CloseReason reason)
    {
    	// If an open operation is stuck, this lets us shut down anyway.
    	CompletableFuture<?> captured = this.internalOperationFuture;
    	if (captured != null)
    	{
    		captured.cancel(true);
    	}
    	
    	if (this.partitionReceiver != null)
    	{
    		// Disconnect any processor from the receiver so the processor won't get
    		// any more calls. But a call could be in progress right now. 
    		this.partitionReceiver.setReceiveHandler(null);
    		
            // Close the EH clients. Errors are swallowed, nothing we could do about them anyway.
            cleanUpClients();
    	}
    }
    
    
    private class InternalReceiveHandler extends PartitionReceiveHandler
    {
		@Override
		public void onReceive(Iterable<EventData> events)
		{
        	// This method is called on the thread that the Java EH client uses to run the pump.
        	// There is one pump per EventHubClient. Since each PartitionPump creates a new EventHubClient,
        	// using that thread to call onEvents does no harm. Even if onEvents is slow, the pump will
        	// get control back each time onEvents returns, and be able to receive a new batch of messages
        	// with which to make the next onEvents call. The pump gains nothing by running faster than onEvents.
			
			EventHubPartitionPump.this.onEvents(events);
		}

		@Override
		public void onError(Throwable error)
		{
			if (error == null)
			{
				error = new Throwable("No error info supplied by EventHub client");
			}
			EventHubPartitionPump.this.host.logWithHostAndPartition(EventHubPartitionPump.this.partitionContext, "EventHub client error: " + error.toString());
			if (error instanceof Exception)
			{
				EventHubPartitionPump.this.host.logWithHostAndPartition(EventHubPartitionPump.this.partitionContext, "EventHub client error continued", (Exception)error);
			}
			EventHubPartitionPump.this.pumpStatus = PartitionPumpStatus.PP_ERRORED;
		}

		@Override
		public void onClose(Throwable error)
		{
			if (error == null)
			{
				error = new Throwable("normal shutdown"); // Is this true?
			}
			EventHubPartitionPump.this.host.logWithHostAndPartition(EventHubPartitionPump.this.partitionContext, "EventHub client closed: " + error.toString());
			if (error instanceof Exception)
			{
				EventHubPartitionPump.this.host.logWithHostAndPartition(EventHubPartitionPump.this.partitionContext, "EventHub client closed continued", (Exception)error);
			}
			EventHubPartitionPump.this.pumpStatus = PartitionPumpStatus.PP_ERRORED;
		}
    }
}