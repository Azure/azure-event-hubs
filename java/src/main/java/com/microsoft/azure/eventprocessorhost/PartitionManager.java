package com.microsoft.azure.eventprocessorhost;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.Future;


public class PartitionManager
{
    private EventProcessorHost host;

    private ArrayList<String> partitionIds = null;

    // DUMMY STARTS
    public static int dummyPartitionCount = 4;
    public static int dummyHostCount = 1;
    // DUMMY ENDS

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
            for (int i = 0; i < PartitionManager.dummyPartitionCount; i++)
            {
            	partitionIds.add(String.valueOf(i));
            }
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
            int allLeasesCount = 0;
            int ourLeasesCount = 0;
            ArrayList<Lease> somebodyElsesLeases = new ArrayList<Lease>();
            for (Future<Lease> future : allLeases)
            {
                Lease possibleLease = future.get();
                allLeasesCount++;
                if ((possibleLease.getOwner().compareTo(this.host.getHostName()) == 0) || possibleLease.isExpired())
                {
                    Lease gotLease = leaseManager.acquireLease(possibleLease.getPartitionId()).get();
                    if (gotLease != null)
                    {
                        ourLeases.put(gotLease.getPartitionId(), gotLease);
                        ourLeasesCount++;
                    }
                }
                else
                {
                	somebodyElsesLeases.add(possibleLease);
                }
            }
            // DUMMY STARTS
            while ((ourLeasesCount < (allLeasesCount / PartitionManager.dummyHostCount)) && (somebodyElsesLeases.size() > 0))
            {
            	Lease tryTake = somebodyElsesLeases.remove(0);
            	Lease takenLease = leaseManager.acquireLease(tryTake.getPartitionId()).get();
            	if (takenLease != null)
            	{
            		System.out.println("Took lease for partition " + takenLease.getPartitionId());
            		ourLeases.put(takenLease.getPartitionId(), takenLease);
            		ourLeasesCount++;
            	}
            	else
            	{
            		System.out.println("Failed to take lease for partition " + tryTake.getPartitionId());
            	}
            }
            
            allLeases = leaseManager.getAllLeases();
            for (Future<Lease> future : allLeases)
            {
            	Lease dumpLease = future.get();
            	System.out.println("Lease on partition " + dumpLease.getPartitionId() + " owned by " + dumpLease.getOwner());
            }
            // DUMMY ENDS
        }

        return ourLeases;
    }
}
