package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.eventhubs.EventData;

import java.awt.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.ExecutorService;
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
                // DUMMY ENDS
            }

            // Remove any pumps for which we have lost the lease.
            for (String partitionId : this.pumps.keySet())
            {
                if (!this.leases.containsKey(partitionId))
                {
                    PartitionPump partitionPump = this.pumps.get(partitionId);
                    if (!partitionPump.getStatus().isDone())
                    {
                        partitionPump.forceClose(CloseReason.LeaseLost);
                        this.pumps.remove(partitionId);
                    }
                }
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
                            startSinglePump(partitionId);
                        }
                        // else
                        // we have the lease and we have a working pump, nothing to do
                    }
                    else
                    {
                        startSinglePump(partitionId);
                    }
                }
                catch (Exception e)
                {
                    // DUMMY STARTS
                    System.out.println("Failure starting pump on partition " + partitionId + ": " + e.toString());
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
                this.pumps.get(partitionId).getStatus().get();
            }
            catch (Exception e)
            {
                // DUMMY STARTS
                System.out.println("Failure waiting for shutdown on " + partitionId);
                // DUMMY ENDS
            }
        }
    }

    private void startSinglePump(String partitionId) throws Exception
    {
        PartitionPump partitionPump = new PartitionPump(this.host, partitionId);
        partitionPump.startPump();
        this.pumps.put(partitionId, partitionPump); // do the put after start, if the start fails then put doesn't happen
    }

    private class PartitionPump implements Runnable
    {
        private EventProcessorHost host;
        private IEventProcessor processor;
        private PartitionContext partitionContext;

        private Future<?> future;
        private String partitionId;
        private Boolean keepGoing = true;

        public PartitionPump(EventProcessorHost host, String partitionId)
        {
            this.host = host;

            this.partitionId = partitionId;
        }

        public void startPump() throws Exception
        {
            this.partitionContext = new PartitionContext(this.host.getCheckpointManager(), this.partitionId);
            // TODO fill in context?
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
                this.processor.onClose(this.partitionContext, reason);
            }
            catch (Exception e)
            {
                // DUMMY STARTS
                System.out.println("Failed closing processor " + e.toString());
                // DUMMY ENDS
            }
        }

        public void shutdown()
        {
            this.keepGoing = false;
        }

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
                // DUMMY ENDS
            }

            int i = 0; // DUMMY
            while (keepGoing)
            {
                // Receive loop goes here
                // DUMMY STARTS
                EventData dummyEvent = new EventData(("event " + i + "on partition " + this.partitionId).getBytes());
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

            try
            {
                this.processor.onClose(this.partitionContext, CloseReason.Shutdown);
            }
            catch (Exception e)
            {
                // DUMMY STARTS
                System.out.println("Failed closing processor " + e.toString());
                // DUMMY ENDS
            }
        }
    }
}
