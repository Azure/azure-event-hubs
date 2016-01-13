package com.microsoft.azure.eventprocessorhost;


import java.util.ArrayList;
import java.util.HashMap;

public class PartitionManager
{
    private String eventHubConnectionString;
    private ILeaseManager leaseManager;

    private ArrayList<String> partitionIds = null;

    public PartitionManager(String eventHubConnectionString, ILeaseManager leaseManager)
    {
        this.eventHubConnectionString = eventHubConnectionString;
        this.leaseManager = leaseManager;
    }

    public Iterable<String> getPartitionIds()
    {
        // DUMMY BEGINS
        if (this.partitionIds == null)
        {
            ArrayList<String> partitionIds = new ArrayList<String>();
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
        HashMap<String, Lease> leases = new HashMap<String, Lease>();

        if (!this.leaseManager.leaseStoreExists().get())
        {
            if (!this.leaseManager.createLeaseStoreIfNotExists().get())
            {
                // DUMMY STARTS
                throw new Exception("couldn't create lease store");
                // DUMMY ENDS
            }

            // Determine how many partitions there are, create leases for them, and acquire those leases
            // DUMMY STARTS
            for (String id : getPartitionIds())
            {
                this.leaseManager.createLeaseIfNotExists(id).get();
                Lease gotLease = this.leaseManager.acquireLease(id).get();
                if (gotLease != null)
                {
                    leases.put(id, gotLease);
                }
            }
            // DUMMY ENDS
        }
        else
        {
            // Inspect all leases. If any are expired, take those.
            // Grab more if needed for load balancing
        }

        return leases;
    }
}
