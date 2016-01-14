package com.microsoft.azure.eventprocessorhost;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.Future;


public class PartitionManager
{
    private EventProcessorHost host;

    private ArrayList<String> partitionIds = null;

    public PartitionManager(EventProcessorHost host)
    {
        this.host = host;
    }

    public Iterable<String> getPartitionIds()
    {
        // DUMMY BEGINS
        if (this.partitionIds == null)
        {
            this.partitionIds = new ArrayList<String>();
            partitionIds.add("0");
            partitionIds.add("1");
            partitionIds.add("2");
            partitionIds.add("3");
        }
        // DUMMY ENDS

        return this.partitionIds;
    }

    public HashMap<String, Lease> getSomeLeases() throws Exception
    {
        ILeaseManager leaseManager = this.host.getLeaseManager();
        HashMap<String, Lease> ourLeases = new HashMap<String, Lease>();

        if (!leaseManager.leaseStoreExists().get())
        {
            if (!leaseManager.createLeaseStoreIfNotExists().get())
            {
                // DUMMY STARTS
                throw new Exception("couldn't create lease store");
                // DUMMY ENDS
            }

            // Determine how many partitions there are, create leases for them, and acquire those leases
            // DUMMY STARTS
            for (String id : getPartitionIds())
            {
                leaseManager.createLeaseIfNotExists(id).get();
                Lease gotLease = leaseManager.acquireLease(id).get();
                if (gotLease != null)
                {
                    ourLeases.put(id, gotLease);
                }
            }
            // DUMMY ENDS
        }
        else
        {
            // Inspect all leases.
            // Take any leases that currently belong to us.
            // If any are expired, take those.
            // TODO Grab more if needed for load balancing

            Iterable<Future<Lease>> allLeases = leaseManager.getAllLeases();
            for (Future<Lease> future : allLeases)
            {
                Lease possibleLease = future.get();
                if ((possibleLease.getOwner().compareTo(this.host.getHostName()) == 0) || possibleLease.isExpired())
                {
                    Lease gotLease = leaseManager.acquireLease(possibleLease.getPartitionId()).get();
                    if (gotLease != null)
                    {
                        ourLeases.put(gotLease.getPartitionId(), gotLease);
                    }
                }
            }
        }

        return ourLeases;
    }
}
