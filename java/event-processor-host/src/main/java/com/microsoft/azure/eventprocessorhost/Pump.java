package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.PartitionReceiver;
import com.microsoft.azure.servicebus.ReceiverDisconnectedException;
import com.microsoft.azure.servicebus.ServiceBusException;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.CancellationException;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;


public class Pump implements Runnable
{
    private EventProcessorHost host;

    private HashMap<String, Lease> leases;
    private HashMap<String, PartitionPump> pumps;
    private Future<?> pumpFuture;
    private Boolean keepGoing = true;

    public Pump(EventProcessorHost host)
    {
        this.host = host;

        this.leases = new HashMap<String, Lease>();
        this.pumps = new HashMap<String, PartitionPump>();
    }

    public void doPump()
    {
        this.pumpFuture = host.getExecutorService().submit(this);
    }

    public Future<?> requestPumpStop()
    {
        this.keepGoing = false;
        return this.pumpFuture;
    }

    public void run()
    {
        while (keepGoing)
        {
        	// Re-get leases
            try
            {
                this.leases = host.getPartitionManager().getSomeLeases();
            }
            catch (Exception e)
            {
                // DUMMY STARTS
                this.host.logWithHost("Exception getting leases", e);
                // DUMMY ENDS
            }

            // Remove any pumps for which we have lost the lease.
            ArrayList<String> pumpsToRemove = new ArrayList<String>();
            Set<String> savedPumpsKeySet = this.pumps.keySet();
        	// DUMMY STARTS
        	for (String partitionId : savedPumpsKeySet)
        	{
        		this.host.logWithHostAndPartition(partitionId, "has pump");
        	}
        	// DUMMY ENDS
            for (String partitionId : savedPumpsKeySet)
            {
            	this.host.logWithHostAndPartition(partitionId, "checking pump"); // DUMMY
                if (!this.leases.containsKey(partitionId))
                {
                    PartitionPump partitionPump = this.pumps.get(partitionId);
                    if (!partitionPump.getStatus().isDone())
                    {
                    	this.host.logWithHostAndPartition(partitionId, "closing pump");
                        partitionPump.forceClose(CloseReason.LeaseLost);
                        pumpsToRemove.add(partitionId);
                    }
                }
                else
                {
                	this.host.logWithHostAndPartition(partitionId, "pump valid"); // DUMMY
                }
            }
            // Avoid concurrent modification exception by doing the removes as a separate step
            for (String removee : pumpsToRemove)
            {
            	this.host.logWithHostAndPartition(removee, "removing pump"); // DUMMY
            	this.pumps.remove(removee);
            }

            // Check status of pumps for leases, start/restart where needed.
            for (String partitionId : this.leases.keySet())
            {
                try
                {
                    if (this.pumps.containsKey(partitionId))
                    {
                        if (this.pumps.get(partitionId).getStatus().isDone())
                        {
                        	this.host.logWithHostAndPartition(partitionId, "Restarting pump");
                            this.pumps.remove(partitionId);
                            startSinglePump(this.leases.get(partitionId));
                        }
                        // else
                        // we have the lease and we have a working pump, nothing to do
                    }
                    else
                    {
                        startSinglePump(this.leases.get(partitionId));
                    }
                }
                catch (Exception e)
                {
                    // DUMMY STARTS
                    this.host.logWithHostAndPartition(partitionId, "Failure starting pump", e);
                    // DUMMY ENDS
                }
            }

            // Sleep
            try
            {
                Thread.sleep(10000); // ten seconds for now
            }
            catch (InterruptedException e)
            {
                // DUMMY STARTS
                this.host.logWithHost("Sleep was interrupted", e);
                // DUMMY ENDS
            }
        }

        for (String partitionId : this.pumps.keySet())
        {
            this.pumps.get(partitionId).shutdown();
        }
        for (String partitionId : this.pumps.keySet())
        {
            try
            {
                this.host.logWithHostAndPartition(partitionId, "Waiting for pump shutdown"); // DUMMY
                this.pumps.get(partitionId).getStatus().get();
                this.host.logWithHostAndPartition(partitionId, "Pump shutdown complete"); // DUMMY
            }
            catch (ExecutionException e)
            {
            	if (e.getCause() instanceof CancellationException)
            	{
            		this.host.logWithHostAndPartition(partitionId, "Pump shutdown complete after operation cancelled"); // DUMMY
            	}
            	else
            	{
                    // DUMMY STARTS
                    this.host.logWithHostAndPartition(partitionId, "Failure in pump shutdown", e); // DUMMY
                    // DUMMY ENDS
            	}
            }
            catch (Exception e)
            {
                // DUMMY STARTS
                this.host.logWithHostAndPartition(partitionId, "Failure in pump shutdown", e); // DUMMY
                // DUMMY ENDS
            }
        }

        this.host.logWithHost("Master pump loop exiting"); // DUMMY
    }

    private void startSinglePump(Lease lease) throws Exception
    {
        PartitionPump partitionPump = new PartitionPump(this.host, lease);
        partitionPump.startPump();
        this.pumps.put(lease.getPartitionId(), partitionPump); // do the put after start, if the start fails then put doesn't happen
    }

    private static class PartitionPump implements Runnable
    {
        private EventProcessorHost host;
        private IEventProcessor processor;
        private PartitionContext partitionContext;
        
        private Future<?> future;
        private CompletableFuture<?> receiveFuture = null;
        private Lease lease;
        private Boolean keepGoing = true;
        private CloseReason reason = CloseReason.Shutdown; // default to shutdown

        public PartitionPump(EventProcessorHost host, Lease lease)
        {
            this.host = host;

            this.lease = lease;
        }

        public void startPump() throws Exception
        {
            this.partitionContext = new PartitionContext(this.host.getCheckpointManager(), this.lease.getPartitionId());
            this.partitionContext.setEventHubPath(this.host.getEventHubPath());
            this.partitionContext.setConsumerGroupName(this.host.getConsumerGroupName());
            this.partitionContext.setLease(this.lease);
            this.processor = this.host.getProcessorFactory().createEventProcessor(this.partitionContext);

            this.future = this.host.getExecutorService().submit(this);
        }

        public Future<?> getStatus()
        {
            return this.future;
        }

        public void forceClose(CloseReason reason)
        {
            this.host.logWithHostAndPartition(this.partitionContext, "Forcing close"); // DUMMY
            this.reason = reason;
            shutdown();
        }

        public void shutdown()
        {
        	CompletableFuture<?> captured = this.receiveFuture;
        	if (captured != null)
        	{
        		captured.cancel(true);
        	}
            this.keepGoing = false;
        }

        private static Boolean serialize = true;
        
        // DUMMY STARTS
        // workaround for threading issues in underlying client
        private static EventHubClient serializedClientOpen(PartitionPump thisPump) throws ServiceBusException, IOException, InterruptedException, ExecutionException
        {
        	EventHubClient ehClient = null;
        	synchronized (PartitionPump.serialize)
        	{
				thisPump.receiveFuture = EventHubClient.createFromConnectionString(thisPump.host.getEventHubConnectionString(), true);
				ehClient = (EventHubClient) thisPump.receiveFuture.get();
				thisPump.receiveFuture = null;
        	}
			return ehClient;
        }
        // DUMMY ENDS

        public void run()
        {
        	Boolean openSucceeded = false;
        	
        	EventHubClient ehClient = null;
        	PartitionReceiver ehReceiver = null;
        	Lease lease = this.partitionContext.getLease();
        	
        	// DUMMY STARTS
        	String startingOffset = "0"; // should get from checkpoint manager
        	// DUMMY ENDS
        	
            try
            {
            	this.host.logWithHostAndPartition(this.partitionContext, "Opening EH client");
            	if (PartitionPump.serialize)
            	{
            		ehClient = serializedClientOpen(this);
            	}
            	else
            	{
					this.receiveFuture = EventHubClient.createFromConnectionString(this.host.getEventHubConnectionString(), true);
					ehClient = (EventHubClient) this.receiveFuture.get();
					this.receiveFuture = null;
            	}
            	long epoch = lease.getEpoch() + 1;
            	this.host.logWithHostAndPartition(this.partitionContext, "Opening EH receiver with epoch " + epoch + " at offset " + startingOffset);
				this.receiveFuture = ehClient.createEpochReceiver(this.partitionContext.getConsumerGroupName(), lease.getPartitionId(), startingOffset, epoch);
				lease.setEpoch(epoch); // TODO need to update lease!
				ehReceiver = (PartitionReceiver) this.receiveFuture.get();
				this.receiveFuture = null;
			}
            catch (InterruptedException | ExecutionException | ServiceBusException | IOException e)
            {
				// DUMMY STARTS
            	this.host.logWithHostAndPartition(this.partitionContext, "Failed creating EH client or receiver", e);
				// DUMMY ENDS
				this.keepGoing = false;
			}
            this.host.logWithHostAndPartition(this.partitionContext, "EH client and receiver creation finished");

            if (this.keepGoing)
            {
	        	try
	            {
	                this.processor.onOpen(this.partitionContext);
	                openSucceeded = true;
	            }
	            catch (Exception e)
	            {
	                // DUMMY STARTS
	            	this.host.logWithHostAndPartition(this.partitionContext, "Failed opening processor", e);
	                // DUMMY ENDS
	                this.keepGoing = false;
	            }
            }

            while (this.keepGoing)
            {
            	Iterable<EventData> receivedEvents = null;
            	
                try
                {
                	this.receiveFuture = ehReceiver.receive();
					receivedEvents = (Iterable<EventData>) this.receiveFuture.get();
					this.receiveFuture = null;
                }
                catch (CancellationException e)
                {
                	this.host.logWithHostAndPartition(this.partitionContext, "Receive cancelled, shutting down pump");
                	this.keepGoing = false;
                }
                catch (Exception e)
                {
                	if ((e instanceof ExecutionException) && (e.getCause() instanceof ReceiverDisconnectedException))
                	{
                    	this.host.logWithHostAndPartition(this.partitionContext, "Receiver has been disconnected, shutting down pump");
                    	this.reason = CloseReason.LeaseLost;
                	}
                	else
                	{
	                    // DUMMY STARTS
	                	this.host.logWithHostAndPartition(this.partitionContext, "Got exception from receive", e);
	                    // DUMMY ENDS
                	}
                    this.keepGoing = false;
                }
                
                if (!keepGoing)
                {
                	break;
                }
                
                try
                {
                    this.processor.onEvents(this.partitionContext, receivedEvents);
                }
                catch (Exception e)
                {
                    // What do we even do here? DUMMY STARTS
                	this.host.logWithHostAndPartition(this.partitionContext, "Got exception from onEvents", e);
                    // DUMMY ENDS
                    this.keepGoing = false;
                }
            }

            if (openSucceeded)
            {
                try
                {
                    this.processor.onClose(this.partitionContext, this.reason);
                }
                catch (Exception e)
                {
                    // DUMMY STARTS
                	this.host.logWithHostAndPartition(this.partitionContext, "Failed closing processor", e);
                    // DUMMY ENDS
                }
            }
            
            if (ehReceiver != null)
            {
            	this.host.logWithHostAndPartition(this.partitionContext, "Closing EH receiver");
            	ehReceiver.close();
            	ehReceiver = null;
            }
            
            if (ehClient != null)
            {
            	this.host.logWithHostAndPartition(this.partitionContext, "Closing EH client");
            	ehClient.close();
            	ehClient = null;
            }

            // DUMMY STARTS
            this.host.logWithHostAndPartition(this.partitionContext, "Pump exiting");
            // DUMMY ENDS
        }
    }
}
