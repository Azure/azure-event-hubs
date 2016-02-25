/*
 * LICENSE GOES HERE
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


class Pump
{
    private EventProcessorHost host;

    private ConcurrentHashMap<String, LeaseAndPump> pumpStates;
    
    private class LeaseAndPump
    {
    	public Lease lease;
    	public PartitionPump pump;
    }

    public Pump(EventProcessorHost host)
    {
        this.host = host;

        this.pumpStates = new ConcurrentHashMap<String, LeaseAndPump>();
    }

    public void addPump(String partitionId, Lease lease) throws Exception
    {
    	LeaseAndPump capturedState = this.pumpStates.get(partitionId);
    	if (capturedState != null)
    	{
    		// There already is a pump. Just replace the lease.
    		this.host.logWithHostAndPartition(partitionId, "updating lease for pump");
    		capturedState.lease = lease;
    	}
    	else
    	{
    		// Create a new pump.
    		final LeaseAndPump newPump = new LeaseAndPump(); // final so that the lambda below is happy
    		newPump.lease = lease;
    		newPump.pump = new PartitionPump(this.host, lease);
    		EventProcessorHost.getExecutorService().submit(() -> newPump.pump.startPump());
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
    		if (!capturedState.pump.isClosing())
    		{
    			retval = EventProcessorHost.getExecutorService().submit(() -> capturedState.pump.shutdown(reason));
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


    public enum PartitionPumpStatus { uninitialized, opening, openfailed, running, closing, closed };

    private class PartitionPump
    {
        private EventProcessorHost host;
        private IEventProcessor processor;
        private PartitionContext partitionContext;
        
        private CompletableFuture<?> internalOperationFuture = null;
        private Lease lease;
        private PartitionPumpStatus pumpStatus = PartitionPumpStatus.uninitialized;
        
    	private EventHubClient eventHubClient = null;
    	private PartitionReceiver partitionReceiver = null;
        private InternalReceiveHandler internalReceiveHandler = null;

        public PartitionPump(EventProcessorHost host, Lease lease)
        {
            this.host = host;

            this.lease = lease;
        }

        public void startPump()
        {
        	this.pumpStatus = PartitionPumpStatus.opening;
        	
            this.partitionContext = new PartitionContext(this.host.getCheckpointManager(), this.lease.getPartitionId());
            this.partitionContext.setEventHubPath(this.host.getEventHubPath());
            this.partitionContext.setConsumerGroupName(this.host.getConsumerGroupName());
            this.partitionContext.setLease(this.lease);

            if (this.pumpStatus == PartitionPumpStatus.opening)
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
					this.pumpStatus = PartitionPumpStatus.openfailed;
				}
            }

            if (this.pumpStatus == PartitionPumpStatus.opening)
            {
	        	try
	            {
					this.processor = this.host.getProcessorFactory().createEventProcessor(this.partitionContext);
	                this.processor.onOpen(this.partitionContext);
	                this.internalReceiveHandler = new InternalReceiveHandler();
	                // Handler is set after onOpen completes, meaning onEvents will never fire while onOpen is still executing.
	                this.partitionReceiver.setReceiveHandler(this.internalReceiveHandler);
	                this.pumpStatus = PartitionPumpStatus.running;
	            }
	            catch (Exception e)
	            {
	            	// If the processor won't create or open, only thing we can do here is pass the buck.
	            	// Null it out so we don't try to operate on it further.
	            	this.host.logWithHostAndPartition(this.partitionContext, "Failed creating or opening processor", e);
	            	this.processor = null;
	            	
	                // Close the EH clients. If they fail, there's nothing we could do about that anyway.
	                cleanUpClients();
	                
	            	this.pumpStatus = PartitionPumpStatus.openfailed;
	            }
            }
            
            if (this.pumpStatus == PartitionPumpStatus.openfailed)
            {
            	this.pumpStatus = PartitionPumpStatus.closed; // all cleanup is already done
            	Pump.this.removePump(this.partitionContext.getPartitionId(), CloseReason.Shutdown);
            }
        }
        
        private void openClients() throws ServiceBusException, IOException, InterruptedException, ExecutionException
        {
        	String startingOffset = "0"; // DUMMY should get from checkpoint manager

        	// Create new client/receiver
        	this.host.logWithHostAndPartition(this.partitionContext, "Opening EH client");
			this.internalOperationFuture = EventHubClient.createFromConnectionString(this.host.getEventHubConnectionString(), true);
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
            	synchronized (this.internalReceiveHandler)
            	{
            		// Disconnect the processor from the receiver we're about to close.
            		// Fortunately this is idempotent -- setting the handler to null when it's already been
            		// nulled by code elsewhere is harmless!
            		this.partitionReceiver.setReceiveHandler(null);
            	}
            	
            	this.host.logWithHostAndPartition(this.partitionContext, "Closing EH receiver");
            	try
            	{
					this.partitionReceiver.close();
				}
            	catch (ServiceBusException e)
            	{
            		// Nothing we can do about this except log it.
                	this.host.logWithHostAndPartition(this.partitionContext, "Failed closing EH receiver", e);
				}
            	this.partitionReceiver = null;
            }
            
            if (this.eventHubClient != null)
            {
            	this.host.logWithHostAndPartition(this.partitionContext, "Closing EH client");
            	this.eventHubClient.close();
            	this.eventHubClient = null;
            }
        }

        public PartitionPumpStatus getPumpStatus()
        {
        	return this.pumpStatus;
        }
        
        public Boolean isClosing()
        {
        	return ((this.pumpStatus == PartitionPumpStatus.closing) || (this.pumpStatus == PartitionPumpStatus.closed));
        }

        public void shutdown(CloseReason reason)
        {
        	this.pumpStatus = PartitionPumpStatus.closing;
            this.host.logWithHostAndPartition(this.partitionContext, "pump shutdown for reason " + reason.toString()); // DUMMY

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

            if (this.processor != null)
            {
                try
                {
                	synchronized(this.internalReceiveHandler)
                	{
                		// When we take the lock, any existing onEvents call has finished.
                    	// Because the client has been closed, there will not be any more
                    	// calls to onEvents in the future. Therefore we can safely call onClose.
                		this.processor.onClose(this.partitionContext, reason);
                    }
                }
                catch (Exception e)
                {
                    // DUMMY STARTS Is there anything we should do here except log it?
                	this.host.logWithHostAndPartition(this.partitionContext, "Failure closing processor", e);
                    // DUMMY ENDS
                }
            }
            
            this.pumpStatus = PartitionPumpStatus.closed;
        }
        
        
        private class InternalReceiveHandler extends PartitionReceiveHandler
        {
			@Override
			public void onReceive(Iterable<EventData> events)
			{
                try
                {
                	// This method is called on the thread that the Java EH client uses to run the pump.
                	// There is one pump per EventHubClient. Since each PartitionPump creates a new EventHubClient,
                	// using that thread to call onEvents does no harm. Even if onEvents is slow, the pump will
                	// get control back each time onEvents returns, and be able to receive a new batch of messages
                	// with which to make the next onEvents call. The pump gains nothing by running faster than onEvents.
                	
                	// Synchronize on the handler object to serialize calls to the processor.
                	// The handler is not installed until after onOpen returns, so there is no conflict there.
                	// There could be a conflict between this and onClose, however. All calls to onClose are
                	// protected by synchronizing on the handler object too.
                	synchronized(this)
                	{
                		PartitionPump.this.processor.onEvents(PartitionPump.this.partitionContext, events);
                	}
                }
                catch (Exception e)
                {
                    // DUMMY STARTS
                	// What do we even do here?
                	PartitionPump.this.host.logWithHostAndPartition(PartitionPump.this.partitionContext, "Got exception from onEvents", e);
                    // DUMMY ENDS
                }
			}

			@Override
			public void onError(Exception exception)
			{
				// TODO Auto-generated method stub
				
			}
        }
    }
}
