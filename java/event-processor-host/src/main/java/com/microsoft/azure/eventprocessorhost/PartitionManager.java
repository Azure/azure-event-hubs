/*
 * LICENSE GOES HERE TOO
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;


class PartitionManager implements Runnable
{
    private EventProcessorHost host;
    private Pump pump;

    private ArrayList<String> partitionIds = null;
    
    private boolean keepGoing = true;

    // DUMMY STARTS
    public static int dummyPartitionCount = 4;
    // DUMMY ENDS

    public PartitionManager(EventProcessorHost host)
    {
        this.host = host;
        this.pump = new Pump(this.host);
    }
    
    public <T extends PartitionPump> void setPumpClass(Class<T> pumpClass)
    {
    	this.pump.setPumpClass(pumpClass);
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
    
    public void stopPartitions()
    {
    	this.keepGoing = false;
    }
    
    public void run()
    {
    	try
    	{
    		runLoop();
    		this.host.logWithHost("Partition manager main loop exited normally, shutting down");
    	}
    	catch (Exception e)
    	{
    		this.host.logWithHost("Exception, shutting down partition manager", e);
    	}
    	
    	// Cleanup
    	this.host.logWithHost("Shutting down all pumps");
    	Iterable<Future<?>> pumpRemovals = this.pump.removeAllPumps(CloseReason.Shutdown);
    	
    	// All of the shutdown threads have been launched, we can shut down the executor now.
    	// Shutting down the executor only prevents new tasks from being submitted.
    	// We can't wait for executor termination here because this thread is in the executor.
    	this.host.stopExecutor();
    	
    	// Wait for shutdown threads.
    	for (Future<?> removal : pumpRemovals)
    	{
    		try
    		{
				removal.get();
			}
    		catch (InterruptedException | ExecutionException e)
    		{
    			this.host.logWithHost("Failure during shutdown", e);
    			// By convention, bail immediately on interrupt, even though we're just cleaning
    			// up on the way out. Fortunately, we ARE just cleaning up on the way out, so we're
    			// free to bail without serious side effects.
    			if (e instanceof InterruptedException)
    			{
    				Thread.currentThread().interrupt();
    				throw new RuntimeException(e);
    			}
			}
    	}
    	
    	this.host.logWithHost("Partition manager exiting");
    }
    
    private void runLoop() throws Exception
    {
    	while (this.keepGoing)
    	{
            ILeaseManager leaseManager = this.host.getLeaseManager();
            HashMap<String, Lease> allLeases = new HashMap<String, Lease>();

            if (!leaseManager.leaseStoreExists().get())
            {
                if (!leaseManager.createLeaseStoreIfNotExists().get())
                {
                    throw new RuntimeException("Creating lease store returned false");
                }
                
                // Determine how many partitions there are, create leases for them, and acquire those leases
                for (String id : getPartitionIds())
                {
                	try
                	{
	                    Lease createdLease = leaseManager.createLeaseIfNotExists(id).get();
	                    if ((createdLease != null) && leaseManager.acquireLease(createdLease).get())
	                    {
	                        allLeases.put(createdLease.getPartitionId(), createdLease);
	                    }
                	}
                	catch (ExecutionException e)
                	{
                		this.host.logWithHostAndPartition(id, "Failure creating lease or acquiring created lease for this partition, skipping", e);
                		// TODO if creating a lease fails the first time through it will never be created!
                	}
                }
            }
            else
            {
                // Inspect all leases.
                // Acquire any expired leases.
                // Renew any leases that currently belong to us.
                Iterable<Future<Lease>> gettingAllLeases = leaseManager.getAllLeases();
                ArrayList<Lease> leasesOwnedByOthers = new ArrayList<Lease>();
                int ourLeasesCount = 0;
                for (Future<Lease> future : gettingAllLeases)
                {
                	try
                	{
	                    Lease possibleLease = future.get();
	                    if (possibleLease.isExpired())
	                    {
	                    	if (leaseManager.acquireLease(possibleLease).get())
	                    	{
	                    		allLeases.put(possibleLease.getPartitionId(), possibleLease);
	                    	}
	                    }
	                    else if (possibleLease.getOwner().compareTo(this.host.getHostName()) == 0)
	                    {
	                        if (leaseManager.renewLease(possibleLease).get())
	                        {
	                            allLeases.put(possibleLease.getPartitionId(), possibleLease);
	                            ourLeasesCount++;
	                        }
	                    }
	                    else
	                    {
	                    	allLeases.put(possibleLease.getPartitionId(), possibleLease);
	                    	leasesOwnedByOthers.add(possibleLease);
	                    }
                	}
                	catch (ExecutionException e)
                	{
                		this.host.logWithHost("Failure getting/acquiring/renewing lease, skipping", e);
                	}
                }
                
                // Grab more leases if available and needed for load balancing
                if (leasesOwnedByOthers.size() > 0)
                {
    	            Iterable<Lease> stealTheseLeases = whichLeasesToSteal(leasesOwnedByOthers, ourLeasesCount);
    	            if (stealTheseLeases != null)
    	            {
    	            	for (Lease stealee : stealTheseLeases)
    	            	{
    	            		try
    	            		{
	    	                	if (leaseManager.acquireLease(stealee).get())
	    	                	{
	    	                		this.host.logWithHostAndPartition(stealee.getPartitionId(), "Stole lease");
	    	                		allLeases.put(stealee.getPartitionId(), stealee);
	    	                		ourLeasesCount++;
	    	                	}
	    	                	else
	    	                	{
	    	                		this.host.logWithHost("Failed to steal lease for partition " + stealee.getPartitionId());
	    	                	}
    	            		}
    	            		catch (ExecutionException e)
    	            		{
    	            			this.host.logWithHost("Exception stealing lease for partition " + stealee.getPartitionId(), e);
    	            		}
    	            	}
    	            }
                }

                // Update pump with new state of leases.
                for (String partitionId : allLeases.keySet())
                {
                	Lease updatedLease = allLeases.get(partitionId);
                	this.host.logWithHost("Lease on partition " + updatedLease.getPartitionId() + " owned by " + updatedLease.getOwner()); // DEBUG
                	if (updatedLease.getOwner().compareTo(this.host.getHostName()) == 0)
                	{
                		this.pump.addPump(partitionId, updatedLease);
                	}
                	else
                	{
                		this.pump.removePump(partitionId, CloseReason.LeaseLost);
                	}
                }
            }
    		
            try
            {
                Thread.sleep(leaseManager.getLeaseRenewIntervalInMilliseconds());
            }
            catch (InterruptedException e)
            {
            	// Bail on the thread if we are interrupted.
                this.host.logWithHost("Sleep was interrupted", e);
                this.keepGoing = false;
				Thread.currentThread().interrupt();
				throw new RuntimeException(e);
            }
    	}
    }
    
    private Iterable<Lease> whichLeasesToSteal(ArrayList<Lease> stealableLeases, int haveLeaseCount)
    {
    	HashMap<String, Integer> countsByOwner = countLeasesByOwner(stealableLeases);
    	int totalLeases = stealableLeases.size() + haveLeaseCount;
    	int hostCount = countsByOwner.size() + 1;
    	int desiredToHave = (totalLeases + hostCount - 1) / hostCount; // round up
    	ArrayList<Lease> stealTheseLeases = null;
    	if (((desiredToHave - haveLeaseCount) == 1) && (haveLeaseCount > 0))
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
