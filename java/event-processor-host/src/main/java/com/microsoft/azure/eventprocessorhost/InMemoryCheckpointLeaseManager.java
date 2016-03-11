/*
 * LICENSE GOES HERE
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.*;

public class InMemoryCheckpointLeaseManager implements ILeaseManager, ICheckpointManager
{
    private EventProcessorHost host;

    public InMemoryCheckpointLeaseManager()
    {
    }

    // The EventProcessorHost can't pass itself to the AzureStorageCheckpointLeaseManager constructor
    // because it is still being constructed.
    public void setHost(EventProcessorHost host)
    {
        this.host = host;
    }

    public Future<Boolean> checkpointStoreExists()
    {
        return leaseStoreExists();
    }

    public Future<Boolean> createCheckpointStoreIfNotExists()
    {
        return createLeaseStoreIfNotExists();
    }

    public Future<CheckPoint> getCheckpoint(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> getCheckpointSync(partitionId));
    }
    
    private CheckPoint getCheckpointSync(String partitionId)
    {
    	Lease lease = getLeaseSync(partitionId);
    	CheckPoint retval = null;
    	if (lease != null)
    	{
    		retval = lease.getCheckpoint();
    	}
    	return retval;
    }

    public Future<Void> updateCheckpoint(CheckPoint checkpoint)
    {
    	return updateCheckpoint(checkpoint, checkpoint.getOffset(), checkpoint.getSequenceNumber());
    }

    public Future<Void> updateCheckpoint(CheckPoint checkpoint, String offset, long sequenceNumber)
    {
        return EventProcessorHost.getExecutorService().submit(() -> updateCheckpointSync(checkpoint.getPartitionId(), offset, sequenceNumber));
    }

    private Void updateCheckpointSync(String partitionId, String offset, long sequenceNumber)
    {
    	Lease lease = getLeaseSync(partitionId);
    	lease.setOffset(offset);
    	lease.setSequenceNumber(sequenceNumber);
    	updateLeaseSync(lease);
    	return null;
    }

    public Future<Void> deleteCheckpoint(String partitionId)
    {
    	// Make this a no-op to avoid deleting leases by accident.
        return null;
    }


    public Future<Boolean> leaseStoreExists()
    {
        return EventProcessorHost.getExecutorService().submit(() -> (InMemoryLeaseStore.getSingleton().inMemoryLeases != null));
    }

    public Future<Boolean> createLeaseStoreIfNotExists()
    {
        return EventProcessorHost.getExecutorService().submit(() -> createLeaseStoreIfNotExistsSync());
    }

    private Boolean createLeaseStoreIfNotExistsSync()
    {
        if (InMemoryLeaseStore.getSingleton().inMemoryLeases == null)
        {
        	this.host.logWithHost("createLeaseStoreIfNotExists() creating in memory hashmap");
            InMemoryLeaseStore.getSingleton().inMemoryLeases = new HashMap<String, Lease>();
        }
        return true;
    }
    
    public Future<Lease> getLease(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> getLeaseSync(partitionId));
    }

    private Lease getLeaseSync(String partitionId)
    {
    	Lease returnLease = null;
        Lease leaseInStore = InMemoryLeaseStore.getSingleton().inMemoryLeases.get(partitionId);
        if (leaseInStore == null)
        {
        	this.host.logWithHostAndPartition(partitionId, "getLease() no existing lease");
        	returnLease = null;
        }
        else
        {
        	returnLease = new Lease(leaseInStore);
        }
        return returnLease;
    }

    public Iterable<Future<Lease>> getAllLeases()
    {
        ArrayList<Future<Lease>> leases = new ArrayList<Future<Lease>>();
        Iterable<String> partitionIds = this.host.getPartitionManager().getPartitionIds();
        for (String id : partitionIds)
        {
            leases.add(getLease(id));
        }
        return leases;
    }

    public Future<Lease> createLeaseIfNotExists(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> createLeaseIfNotExistsSync(partitionId));
    }

    private Lease createLeaseIfNotExistsSync(String partitionId)
    {
    	Lease returnLease = InMemoryLeaseStore.getSingleton().inMemoryLeases.get(partitionId);
        if (returnLease != null)
        {
        	this.host.logWithHostAndPartition(partitionId, "createLeaseIfNotExists() found existing lease, OK");
        }
        else
        {
        	this.host.logWithHostAndPartition(partitionId, "createLeaseIfNotExists() creating new lease");
            Lease storeLease = new Lease(this.host.getEventHubPath(), this.host.getConsumerGroupName(), partitionId);
            storeLease.setEpoch(0L);
            storeLease.setOwner(this.host.getHostName());
            InMemoryLeaseStore.getSingleton().inMemoryLeases.put(partitionId, storeLease);
            returnLease = new Lease(storeLease);
        }
        return returnLease;
    }
    
    public Future<Void> deleteLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> deleteLeaseSync(lease));
    }
    
    private Void deleteLeaseSync(Lease lease)
    {
    	InMemoryLeaseStore.getSingleton().inMemoryLeases.remove(lease.getPartitionId());
    	return null;
    }

    public Future<Boolean> acquireLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> acquireLeaseSync(lease));
    }

    private Boolean acquireLeaseSync(Lease lease)
    {
    	Boolean retval = true;
        Lease leaseInStore = InMemoryLeaseStore.getSingleton().inMemoryLeases.get(lease.getPartitionId());
        if (leaseInStore != null)
        {
            if ((leaseInStore.getOwner() == null) || (leaseInStore.getOwner().length() == 0))
            {
                leaseInStore.setOwner(this.host.getHostName());
            	this.host.logWithHostAndPartition(lease.getPartitionId(), "acquireLease() acquired lease");
            }
            else if (leaseInStore.getOwner().compareTo(this.host.getHostName()) == 0)
            {
            	this.host.logWithHostAndPartition(lease.getPartitionId(), "acquireLease() already hold lease");
            }
            else
            {
            	String oldOwner = leaseInStore.getOwner();
            	leaseInStore.setOwner(this.host.getHostName());
            	this.host.logWithHostAndPartition(lease.getPartitionId(), "acquireLease() stole lease from " + oldOwner);
            }
        }
        else
        {
        	this.host.logWithHostAndPartition(lease.getPartitionId(), "acquireLease() can't find lease");
        	retval = false;
        }
        
        return retval;
    }
    
    public Future<Boolean> renewLease(Lease lease)
    {
    	// No-op at this time
        return EventProcessorHost.getExecutorService().submit(() -> true);
    }

    public Future<Boolean> releaseLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> releaseLeaseSync(lease));
    }
    
    private Boolean releaseLeaseSync(Lease lease)
    {
    	Boolean retval = true;
    	Lease leaseInStore = InMemoryLeaseStore.getSingleton().inMemoryLeases.get(lease.getPartitionId());
    	if (leaseInStore != null)
    	{
    		if (leaseInStore.getOwner().compareTo(this.host.getHostName()) == 0)
    		{
	    		this.host.logWithHostAndPartition(lease.getPartitionId(), "releaseLease() released OK");
	    		leaseInStore.setOwner("");
    		}
    		else
    		{
	    		this.host.logWithHostAndPartition(lease.getPartitionId(), "releaseLease() not released because we don't own lease");
    			retval = false;
    		}
    	}
    	else
    	{
    		this.host.logWithHostAndPartition(lease.getPartitionId(), "releaseLease() can't find lease");
    		retval = false;
    	}
    	return retval;
    }

    public Future<Boolean> updateLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> updateLeaseSync(lease));
    }
    
    private Boolean updateLeaseSync(Lease lease)
    {
    	Boolean retval = true;
    	Lease leaseInStore = InMemoryLeaseStore.getSingleton().inMemoryLeases.get(lease.getPartitionId());
    	if (leaseInStore != null)
    	{
    		if (leaseInStore.getOwner().compareTo(this.host.getHostName()) == 0)
    		{
   				leaseInStore.setEpoch(lease.getEpoch());
    			CheckPoint cp = lease.getCheckpoint();
  				leaseInStore.setOffset(cp.getOffset());
   				leaseInStore.setSequenceNumber(cp.getSequenceNumber());
    			leaseInStore.setToken(lease.getToken());
    		}
    		else
    		{
	    		this.host.logWithHostAndPartition(lease.getPartitionId(), "updateLease() not updated because we don't own lease");
    			retval = false;
    		}
    	}
    	else
    	{
    		this.host.logWithHostAndPartition(lease.getPartitionId(), "updateLease() can't find lease");
    		retval = false;
    	}
    	return retval;
    }


    private static class InMemoryLeaseStore
    {
        private static InMemoryLeaseStore singleton = null;

        public static InMemoryLeaseStore getSingleton()
        {
            if (InMemoryLeaseStore.singleton == null)
            {
                InMemoryLeaseStore.singleton = new InMemoryLeaseStore();
            }
            return InMemoryLeaseStore.singleton;
        }

        public HashMap<String, Lease> inMemoryLeases = null;
    }
}


