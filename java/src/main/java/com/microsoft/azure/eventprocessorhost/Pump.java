package com.microsoft.azure.eventprocessorhost;

import java.awt.*;
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

            // Sleep

            // Re-get leases
        }
    }

    private void startSinglePump(String partitionId)
    {
        PartitionPump partitionPump = new PartitionPump(this.host, partitionId);
        this.pumps.put(partitionId, partitionPump);
        partitionPump.startPump();
    }

    private class PartitionPump implements Runnable
    {
        private EventProcessorHost host;

        private Future<?> future;
        private String partitionId;

        public PartitionPump(EventProcessorHost host, String partitionId)
        {
            this.host = host;

            this.partitionId = partitionId;
        }

        public void startPump()
        {
            this.future = this.host.getExecutorService().submit(this);
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
