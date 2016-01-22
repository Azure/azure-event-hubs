package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.eventhubs.EventData;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Set;
import java.util.concurrent.Callable;
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
                System.out.println("Exception getting leases " + e.toString());
                e.printStackTrace();
                // DUMMY ENDS
            }

            // Remove any pumps for which we have lost the lease.
            ArrayList<String> pumpsToRemove = new ArrayList<String>();
            Set<String> savedPumpsKeySet = this.pumps.keySet();
        	// DUMMY STARTS
        	for (String partitionId : savedPumpsKeySet)
        	{
        		System.out.println("Host " + this.host.getHostName() + " has pump for partition " + partitionId);
        	}
        	// DUMMY ENDS
            for (String partitionId : savedPumpsKeySet)
            {
            	System.out.println("Host " + this.host.getHostName() + " checking pump " + partitionId); // DUMMY
                if (!this.leases.containsKey(partitionId))
                {
                    PartitionPump partitionPump = this.pumps.get(partitionId);
                    if (!partitionPump.getStatus().isDone())
                    {
                    	System.out.println("######## Host " + this.host.getHostName() + " would close " + partitionId);
                        partitionPump.forceClose(CloseReason.LeaseLost);
                        System.out.println("ForceClose returned"); // DUMMY
                        pumpsToRemove.add(partitionId);
                    }
                }
                else
                {
                	System.out.println("this.leases contains " + partitionId + " and pump exists for host " + this.host.getHostName()); // DUMMY
                }
            }
            System.out.println("End of pumps check for host " + this.host.getHostName()); // DUMMY
            // Avoid concurrent modification exception by doing the removes as a separate step
            for (String removee : pumpsToRemove)
            {
            	System.out.println("######## Host " + this.host.getHostName() + " removing " + removee); // DUMMY
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
                    System.out.println("Failure starting pump on partition " + partitionId + ": " + e.toString());
                    e.printStackTrace();
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
                System.out.println("Sleep was interrupted " + e.toString());
                e.printStackTrace();
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
                System.out.println("Waiting for pump shutdown on " + partitionId); // DUMMY
                this.pumps.get(partitionId).getStatus().get();
                System.out.println("Got for pump shutdown on " + partitionId); // DUMMY
            }
            catch (Exception e)
            {
                // DUMMY STARTS
                System.out.println("Failure waiting for shutdown on " + partitionId);
                e.printStackTrace();
                // DUMMY ENDS
            }
        }

        System.out.println("Master pump loop exiting"); // DUMMY
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
        private Lease lease;
        private Boolean keepGoing = true;
        private Boolean alreadyForceClosed = false;

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
            try
            {
                System.out.println("Forcing close on partition " + this.partitionContext.getLease().getPartitionId()); // DUMMY
                this.keepGoing = false;
                this.alreadyForceClosed = true;
                this.host.getExecutorService().submit(new ForceCloseCallable(this.processor, this.partitionContext, reason));
            }
            catch (Exception e)
            {
                // DUMMY STARTS
                System.out.println("Failed closing processor " + e.toString());
                e.printStackTrace();
                // DUMMY ENDS
            }
        }

        public void shutdown()
        {
            this.keepGoing = false;
        }

        private static int eventNumber = 0; // DUMMY

        public void run()
        {
            try
            {
                this.processor.onOpen(this.partitionContext);
            }
            catch (Exception e)
            {
                // DUMMY STARTS
                System.out.println("Failed opening processor " + e.toString());
                e.printStackTrace();
                // DUMMY ENDS
            }

            while (this.keepGoing)
            {
                // Receive loop goes here
                // DUMMY STARTS
                EventData dummyEvent = new EventData(("event " + PartitionPump.eventNumber++ + " on partition " + this.lease.getPartitionId() + " by host " + this.host.getHostName()).getBytes());
                ArrayList<EventData> dummyList = new ArrayList<EventData>();
                dummyList.add(dummyEvent);
                try
                {
                    this.processor.onEvents(this.partitionContext, dummyList);
                }
                catch (Exception e)
                {
                    // What do we even do here?
                    System.out.println("Got exception from onEvents " + e.toString());
                    e.printStackTrace();
                }
                try
                {
                    Thread.sleep(2000);
                }
                catch (InterruptedException e)
                {
                    // If sleep is interrupted, don't care
                }
                // DUMMY ENDS
            }

            if (!this.alreadyForceClosed)
            {
                try
                {
                    this.processor.onClose(this.partitionContext, CloseReason.Shutdown);
                }
                catch (Exception e)
                {
                    // DUMMY STARTS
                    System.out.println("Failed closing processor " + e.toString());
                    e.printStackTrace();
                    // DUMMY ENDS
                }
            }

            // DUMMY STARTS
            System.out.println("Pump exiting for " + this.lease.getPartitionId());
            // DUMMY ENDS
        }
        
        private class ForceCloseCallable implements Callable<Void>
        {
        	private PartitionContext context;
        	private CloseReason reason;
        	private IEventProcessor processor;
        	
        	public ForceCloseCallable(IEventProcessor processor, PartitionContext context, CloseReason reason)
        	{
        		this.processor = processor;
        		this.context = context;
        		this.reason = reason;
        	}
        	
        	public Void call()
        	{
                try
                {
					this.processor.onClose(this.context, this.reason);
				}
                catch (Exception e)
                {
					// DUMMY what to do here?
					e.printStackTrace();
				}
        		return null;
        	}
        }
    }
}
