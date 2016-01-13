package com.microsoft.azure.eventprocessorhost;

import java.awt.*;
import java.util.HashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;


public class Pump implements Runnable
{
    private HashMap<String, Lease> leases;
    private HashMap<String, PartitionPump> pumps;
    private EventProcessorHost host;
    private Future<?> pumpFuture;
    private Boolean keepGoing = true;

    public Pump(EventProcessorHost host)
    {
        this.leases = new HashMap<String, Lease>();
        this.pumps = new HashMap<String, PartitionPump>();
        this.host = host;
    }

    public void doStartupTasks() throws Exception
    {
        this.leases = host.getPartitionManager().getSomeLeases();
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
            // Remove any pumps for which we have lost the lease.
            // DUMMY STARTS
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
            // DUMMY ENDS

            // Check status of pumps for leases, start/restart where needed.
            for (String partitionId : this.leases.keySet())
            {
                if (this.pumps.containsKey(partitionId))
                {

                }
                else
                {
                    PartitionPump partitionPump = new PartitionPump(partitionId);
                    this.pumps.put(partitionId, partitionPump);
                    partitionPump.startPump(host.getExecutorService());
                }
            }

            // Sleep

            // Re-get leases
        }
    }

    private class PartitionPump implements Runnable
    {
        private Future<?> future;
        private String partitionId;

        public PartitionPump(String partitionId)
        {
            this.partitionId = partitionId;
        }

        public void startPump(ExecutorService executorService)
        {
            this.future = executorService.submit(this);
        }

        public Future<?> getStatus()
        {
            return this.future;
        }

        public void forceClose(CloseReason reason)
        {

        }

        public void run()
        {

        }
    }
}
