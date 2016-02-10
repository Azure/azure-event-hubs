package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.PartitionReceiveHandler;
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
                    if (partitionPump.isRunning())
                    {
                    	this.host.logWithHostAndPartition(partitionId, "closing pump");
                        partitionPump.forceClose(CloseReason.LeaseLost);
                    }
                    // else pump is already shut down
                    pumpsToRemove.add(partitionId);
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
                        if (!this.pumps.get(partitionId).isRunning())
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

        this.host.logWithHost("Master pump loop exiting"); // DUMMY
    }

    private void startSinglePump(Lease lease) throws Exception
    {
        PartitionPump partitionPump = new PartitionPump(this.host, lease);
        partitionPump.startPump();
        this.pumps.put(lease.getPartitionId(), partitionPump); // do the put after start, if the start fails then put doesn't happen
    }

    private class PartitionPump
    {
        private EventProcessorHost host;
        private IEventProcessor processor;
        private PartitionContext partitionContext;
        
        private CompletableFuture<?> internalOperationFuture = null;
        private Lease lease;
        private Boolean pumpRunning = false;
        private CloseReason reason = CloseReason.Shutdown; // default to shutdown
        
    	private EventHubClient eventHubClient = null;
    	private PartitionReceiver partitionReceiver = null;
        private InternalReceiveHandler internalReceiveHandler = null;

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

            //this.future = this.host.getExecutorService().submit(this);

            try
            {
				openClients();
			}
            catch (ServiceBusException | IOException | InterruptedException e)
            {
				// DUMMY figure out the retry policy here
				this.host.logWithHostAndPartition(this.partitionContext, "Failure creating client or receiver", e);
				throw e;
				// DUMMY ENDS
			}
            catch (ExecutionException e)
            {
            	if (e.getCause() instanceof ReceiverDisconnectedException)
            	{
            		// DUMMY This is probably due to a receiver with a higher epoch
            		// Is there a way to be sure without checking the exception text?
            		this.host.logWithHostAndPartition(this.partitionContext, "Receiver disconnected on create", e);
            		// DUMMY ENDS
            	}
            	else
            	{
    				// DUMMY figure out the retry policy here
    				this.host.logWithHostAndPartition(this.partitionContext, "Failure creating client or receiver", e);
    				// DUMMY ENDS
            	}
        		throw e;
            }

        	try
            {
                this.processor.onOpen(this.partitionContext);
            }
            catch (Exception e)
            {
            	// If the processor won't open, only thing we can do here is pass the buck.
            	// Null it out so we don't try to operate on it further.
            	this.host.logWithHostAndPartition(this.partitionContext, "Failed opening processor", e);
            	this.processor = null;
            	throw e;
            }
            
            this.internalReceiveHandler = new InternalReceiveHandler();
            this.partitionReceiver.setReceiveHandler(this.internalReceiveHandler);
            
            this.pumpRunning = true;
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
                	this.host.logWithHostAndPartition(this.partitionContext, "Failed closing old receiver", e);
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

        public Boolean isRunning()
        {
        	return this.pumpRunning;
        }

        public void forceClose(CloseReason reason)
        {
            this.host.logWithHostAndPartition(this.partitionContext, "Forcing close"); // DUMMY
            this.reason = reason;
            shutdown();
        }

        public void shutdown()
        {
        	// If an operation is stuck, this lets us shut down anyway.
        	CompletableFuture<?> captured = this.internalOperationFuture;
        	if (captured != null)
        	{
        		captured.cancel(true);
        	}

            if (this.processor != null)
            {
                try
                {
            		// Taking the lock means that there is no onEvents call in progress.
                	synchronized(this.internalReceiveHandler)
                	{
                		// Disconnect the processor from the receiver so the processor won't get
                		// called unexpectedly after it is closed.
                		this.partitionReceiver.setReceiveHandler(null);
                		// Close the processor.
                		this.processor.onClose(this.partitionContext, this.reason);
                    }
                }
                catch (Exception e)
                {
                    // DUMMY STARTS Is there anything we should do here except log it?
                	this.host.logWithHostAndPartition(this.partitionContext, "Failed closing processor", e);
                    // DUMMY ENDS
                }
            }
            
            cleanUpClients();
            
            this.pumpRunning = false;
        }
        
        
        private class InternalReceiveHandler extends PartitionReceiveHandler
        {
			@Override
			public void onReceive(Iterable<EventData> events)
			{
                try
                {
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
