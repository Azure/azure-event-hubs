/*
 * LICENSE GOES HERE TOO
 */

package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.PartitionReceiveHandler;
import com.microsoft.azure.eventhubs.PartitionReceiver;
import com.microsoft.azure.servicebus.ReceiverDisconnectedException;
import com.microsoft.azure.servicebus.ServiceBusException;

import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.Callable;


class Pump
{
    private EventProcessorHost host;

    private ConcurrentHashMap<String, LeaseAndPump> pumpStates;
    
    private Class<?> pumpClass = EventHubPartitionPump.class;
    
    private class LeaseAndPump implements Callable<Void>
    {
    	private Lease lease;
    	private PartitionPump pump;
    	
    	public LeaseAndPump(Lease lease, PartitionPump pump)
    	{
    		this.lease = lease;
    		this.pump = pump;
    	}
    	
    	public void setLease(Lease newLease)
    	{
    		this.lease = newLease;
    	}
    	
    	public PartitionPump getPump()
    	{
    		return this.pump;
    	}
    	
		@Override
		public Void call() throws Exception
		{
			this.pump.startPump();
			return null;
		}
    }

    public Pump(EventProcessorHost host)
    {
        this.host = host;

        this.pumpStates = new ConcurrentHashMap<String, LeaseAndPump>();
    }
    
    public <T extends PartitionPump> void setPumpClass(Class<T> pumpClass)
    {
    	this.pumpClass = pumpClass;
    }

    public void addPump(String partitionId, Lease lease) throws Exception
    {
    	LeaseAndPump capturedState = this.pumpStates.get(partitionId);
    	if (capturedState != null)
    	{
    		// There already is a pump. Just replace the lease.
    		this.host.logWithHostAndPartition(partitionId, "updating lease for pump");
    		capturedState.setLease(lease);
    	}
    	else
    	{
    		// Create a new pump.
    		PartitionPump newPartitionPump = (PartitionPump)this.pumpClass.newInstance();
    		newPartitionPump.initialize(this.host, lease);
    		LeaseAndPump newPump = new LeaseAndPump(lease, newPartitionPump);
    		EventProcessorHost.getExecutorService().submit(newPump);
            this.pumpStates.put(partitionId, newPump); // do the put after start, if the start fails then put doesn't happen
    		this.host.logWithHostAndPartition(partitionId, "created new pump");
    	}
    }
    
    public Future<?> removePump(String partitionId, final CloseReason reason)
    {
    	Future<?> retval = null;
    	LeaseAndPump capturedState = this.pumpStates.get(partitionId);
    	if (capturedState != null)
    	{
			this.host.logWithHostAndPartition(partitionId, "closing pump for reason " + reason.toString());
    		if (!capturedState.getPump().isClosing())
    		{
    			retval = EventProcessorHost.getExecutorService().submit(() -> capturedState.getPump().shutdown(reason));
    		}
    		// else, pump is already closing/closed, don't need to try to shut it down again
    		
    		this.host.logWithHostAndPartition(partitionId, "removing pump");
    		this.pumpStates.remove(partitionId);
    	}
    	else
    	{
    		this.host.logWithHostAndPartition(partitionId, "no pump found to remove for partition " + partitionId);
    	}
    	return retval;
    }
    
    public Iterable<Future<?>> removeAllPumps(CloseReason reason)
    {
    	ArrayList<Future<?>> futures = new ArrayList<Future<?>>();
    	for (String partitionId : this.pumpStates.keySet())
    	{
    		futures.add(removePump(partitionId, reason));
    	}
    	return futures;
    }
    
    public boolean hasPump(String partitionId)
    {
    	return this.pumpStates.containsKey(partitionId);
    }


    private class EventHubPartitionPump extends PartitionPump
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
        	String startingOffset = "0"; // DUMMY should get from checkpoint manager

        	// Create new client/receiver
        	this.host.logWithHostAndPartition(this.partitionContext, "Opening EH client");
			this.internalOperationFuture = EventHubClient.createFromConnectionString(this.host.getEventHubConnectionString());
			this.eventHubClient = (EventHubClient) this.internalOperationFuture.get();
			this.internalOperationFuture = null;
			
        	long epoch = this.lease.getEpoch() + 1;
        	this.host.logWithHostAndPartition(this.partitionContext, "Opening EH receiver with epoch " + epoch + " at offset " + startingOffset);
			this.internalOperationFuture = this.eventHubClient.createEpochReceiver(this.partitionContext.getConsumerGroupName(), this.partitionContext.getPartitionId(), startingOffset, epoch);
			this.lease.setEpoch(epoch); // TODO need to update lease!
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
				// TODO Auto-generated method stub
			}

			@Override
			public void onClose(Throwable error)
			{
				// TODO Auto-generated method stub
			}
        }
    }
}
