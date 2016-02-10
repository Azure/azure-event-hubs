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
            // Grab more if needed for load balancing

            Iterable<Future<Lease>> allLeases = leaseManager.getAllLeases();
            int ourLeasesCount = 0;
            ArrayList<Lease> somebodyElsesLeases = new ArrayList<Lease>();
            for (Future<Lease> future : allLeases)
            {
                Lease possibleLease = future.get();
                if (possibleLease.getOwner().compareTo(this.host.getHostName()) == 0)
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

            if (somebodyElsesLeases.size() > 0)
            {
	            Iterable<Lease> stealTheseLeases = whichLeasesToSteal(somebodyElsesLeases, ourLeasesCount);
	            if (stealTheseLeases != null)
	            {
	            	for (Lease stealee : stealTheseLeases)
	            	{
	            		Lease takenLease = leaseManager.acquireLease(stealee.getPartitionId()).get();
	                	if (takenLease != null)
	                	{
	                		this.host.logWithHostAndPartition(takenLease.getPartitionId(), "Stole lease");
	                		ourLeases.put(takenLease.getPartitionId(), takenLease);
	                		ourLeasesCount++;
	                	}
	                	else
	                	{
	                		this.host.logWithHost("Failed to steal lease for partition " + stealee.getPartitionId());
	                	}
	            	}
	            }
            }

            // DUMMY BEGINS
            allLeases = leaseManager.getAllLeases();
            for (Future<Lease> future : allLeases)
            {
            	Lease dumpLease = future.get();
            	this.host.logWithHost("Lease on partition " + dumpLease.getPartitionId() + " owned by " + dumpLease.getOwner());
            }
            // DUMMY ENDS
        }

        return ourLeases;
    }
    
    private Iterable<Lease> whichLeasesToSteal(ArrayList<Lease> stealableLeases, int haveLeaseCount)
    {
    	HashMap<String, Integer> countsByOwner = countLeasesByOwner(stealableLeases);
    	int totalLeases = stealableLeases.size() + haveLeaseCount;
    	int hostCount = countsByOwner.size() + 1;
    	int desiredToHave = (totalLeases + hostCount - 1) / hostCount; // round up
    	ArrayList<Lease> stealTheseLeases = null;
    	if ((desiredToHave - haveLeaseCount) == 1)
    	{
    		this.host.logWithHost("Have only one less than desired, skipping lease stealing");
    	}
    	else if (haveLeaseCount < desiredToHave)
    	{
    		this.host.logWithHost("Has " + haveLeaseCount + " leases, wants " + desiredToHave);
    		stealTheseLeases = new ArrayList<Lease>();
    		String stealFrom = findBiggestOwner(countsByOwner);
    		this.host.logWithHost("Proposed to steal leases from " + stealFrom);
    		for (Lease l : stealableLeases)
    		{
    			if (l.getOwner().compareTo(stealFrom) == 0)
    			{
    				stealTheseLeases.add(l);
    				this.host.logWithHost("Proposed to steal lease for partition " + l.getPartitionId());
    				haveLeaseCount++;
    				if (haveLeaseCount >= desiredToHave)
    				{
    					break;
    				}
    			}
    		}
    	}
    	return stealTheseLeases;
    }
    
    private String findBiggestOwner(HashMap<String, Integer> countsByOwner)
    {
    	int biggestCount = 0;
    	String biggestOwner = null;
    	for (String owner : countsByOwner.keySet())
    	{
    		if (countsByOwner.get(owner) > biggestCount)
    		{
    			biggestCount = countsByOwner.get(owner);
    			biggestOwner = owner;
    		}
    	}
    	return biggestOwner;
    }
    
    private HashMap<String, Integer> countLeasesByOwner(Iterable<Lease> leases)
    {
    	HashMap<String, Integer> counts = new HashMap<String, Integer>();
    	for (Lease l : leases)
    	{
    		if (counts.containsKey(l.getOwner()))
    		{
    			Integer oldCount = counts.get(l.getOwner());
    			counts.put(l.getOwner(), oldCount + 1);
    		}
    		else
    		{
    			counts.put(l.getOwner(), 1);
    		}
    	}
    	for (String owner : counts.keySet())
    	{
    		this.host.log("host " + owner + " owns " + counts.get(owner) + " leases");
    	}
    	this.host.log("total hosts in sorted list: " + counts.size());
    	
    	return counts;
    }
}
