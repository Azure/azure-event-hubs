/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.*;
import java.util.logging.Level;

//
// An ILeaseManager implementation based on an in-memory store. This is obviously volatile
// and can only be shared among hosts within a process, but is useful for testing. Overall, its
// behavior is fairly close to that of AzureStorageCheckpointLeaseManager, but on the other hand
// it is completely separate from the InMemoryCheckpointManager, to allow testing scenarios where
// the two stores are not combined.
//

public class InMemoryLeaseManager implements ILeaseManager
{
    private EventProcessorHost host;

    private final static int leaseIntervalInMillieconds = 30 * 1000;	   // thirty seconds
    private final static int leaseRenewIntervalInMilliseconds = 10 * 1000; // ten seconds

    public InMemoryLeaseManager()
    {
    }

    // The EventProcessorHost can't pass itself to the constructor
    // because it is still being constructed.
    public void initialize(EventProcessorHost host)
    {
        this.host = host;
    }

    @Override
    public int getLeaseRenewIntervalInMilliseconds()
    {
    	return InMemoryLeaseManager.leaseRenewIntervalInMilliseconds;
    }

    @Override
    public Future<Boolean> leaseStoreExists()
    {
        return EventProcessorHost.getExecutorService().submit(() -> leaseStoreExistsSync());
    }
    
    private Boolean leaseStoreExistsSync()
    {
    	return (InMemoryLeaseStore.singleton.inMemoryLeases != null);
    }

    @Override
    public Future<Boolean> createLeaseStoreIfNotExists()
    {
        return EventProcessorHost.getExecutorService().submit(() -> createLeaseStoreIfNotExistsSync());
    }

    private Boolean createLeaseStoreIfNotExistsSync()
    {
        if (InMemoryLeaseStore.singleton.inMemoryLeases == null)
        {
        	this.host.logWithHost(Level.INFO, "createLeaseStoreIfNotExists() creating in memory hashmap");
            InMemoryLeaseStore.singleton.inMemoryLeases = new HashMap<String, InMemoryLease>();
        }
        return true;
    }
    
    @Override
    public Future<Lease> getLease(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> getLeaseSync(partitionId));
    }

    private InMemoryLease getLeaseSync(String partitionId)
    {
    	InMemoryLease returnLease = null;
    	InMemoryLease leaseInStore = InMemoryLeaseStore.singleton.inMemoryLeases.get(partitionId);
        if (leaseInStore == null)
        {
        	this.host.logWithHostAndPartition(Level.WARNING, partitionId, "getLease() no existing lease");
        	returnLease = null;
        }
        else
        {
        	returnLease = new InMemoryLease(leaseInStore);
        }
        return returnLease;
    }

    @Override
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

    @Override
    public Future<Lease> createLeaseIfNotExists(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> createLeaseIfNotExistsSync(partitionId));
    }

    private InMemoryLease createLeaseIfNotExistsSync(String partitionId)
    {
    	InMemoryLease returnLease = InMemoryLeaseStore.singleton.inMemoryLeases.get(partitionId);
        if (returnLease != null)
        {
        	this.host.logWithHostAndPartition(Level.INFO, partitionId, "createLeaseIfNotExists() found existing lease, OK");
        }
        else
        {
        	this.host.logWithHostAndPartition(Level.INFO, partitionId, "createLeaseIfNotExists() creating new lease");
        	InMemoryLease storeLease = new InMemoryLease(this.host.getEventHubPath(), this.host.getConsumerGroupName(), partitionId);
            storeLease.setEpoch(0L);
            storeLease.setOwner("");
            InMemoryLeaseStore.singleton.inMemoryLeases.put(partitionId, storeLease);
            returnLease = new InMemoryLease(storeLease);
        }
        return returnLease;
    }
    
    @Override
    public Future<Void> deleteLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> deleteLeaseSync(lease));
    }
    
    private Void deleteLeaseSync(Lease lease)
    {
    	this.host.logWithHostAndPartition(Level.INFO, lease.getPartitionId(), "Deleting lease");
    	InMemoryLeaseStore.singleton.inMemoryLeases.remove(lease.getPartitionId());
    	return null;
    }

    @Override
    public Future<Boolean> acquireLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> acquireLeaseSync((InMemoryLease)lease));
    }

    private Boolean acquireLeaseSync(InMemoryLease lease)
    {
    	this.host.logWithHostAndPartition(Level.INFO, lease.getPartitionId(), "Acquiring lease");
    	
    	Boolean retval = true;
    	InMemoryLease leaseInStore = InMemoryLeaseStore.singleton.inMemoryLeases.get(lease.getPartitionId());
        if (leaseInStore != null)
        {
            if ((leaseInStore.getOwner() == null) || (leaseInStore.getOwner().length() == 0))
            {
                leaseInStore.setOwner(this.host.getHostName());
            	this.host.logWithHostAndPartition(Level.INFO, lease.getPartitionId(), "acquireLease() acquired lease");
            }
            else if (leaseInStore.getOwner().compareTo(this.host.getHostName()) == 0)
            {
            	this.host.logWithHostAndPartition(Level.INFO, lease.getPartitionId(), "acquireLease() already hold lease");
            }
            else
            {
            	String oldOwner = leaseInStore.getOwner();
            	leaseInStore.setOwner(this.host.getHostName());
            	this.host.logWithHostAndPartition(Level.WARNING, lease.getPartitionId(), "acquireLease() stole lease from " + oldOwner);
            }
            long newExpiration = System.currentTimeMillis() + InMemoryLeaseManager.leaseIntervalInMillieconds;
            leaseInStore.setExpirationTime(newExpiration);
            lease.setExpirationTime(newExpiration);
        }
        else
        {
        	this.host.logWithHostAndPartition(Level.SEVERE, lease.getPartitionId(), "acquireLease() can't find lease");
        	retval = false;
        }
        
        return retval;
    }
    
    @Override
    public Future<Boolean> renewLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> renewLeaseSync((InMemoryLease)lease));
    }
    
    private Boolean renewLeaseSync(InMemoryLease lease)
    {
    	this.host.logWithHostAndPartition(Level.INFO, lease.getPartitionId(), "Renewing lease");
    	
    	Boolean retval = true;
    	InMemoryLease leaseInStore = InMemoryLeaseStore.singleton.inMemoryLeases.get(lease.getPartitionId());
        if (leaseInStore != null)
        {
        	if (leaseInStore.getOwner().compareTo(this.host.getHostName()) == 0)
        	{
                long newExpiration = System.currentTimeMillis() + InMemoryLeaseManager.leaseIntervalInMillieconds;
                leaseInStore.setExpirationTime(newExpiration);
                lease.setExpirationTime(newExpiration);
        	}
        	else
            {
            	this.host.logWithHostAndPartition(Level.WARNING, lease.getPartitionId(), "renewLease() not renewed because we don't own lease");
            	retval = false;
            }
        }
        else
        {
        	this.host.logWithHostAndPartition(Level.SEVERE, lease.getPartitionId(), "renewLease() can't find lease");
        	retval = false;
        }
        
        return retval;
    }

    @Override
    public Future<Boolean> releaseLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> releaseLeaseSync((InMemoryLease)lease));
    }
    
    private Boolean releaseLeaseSync(InMemoryLease lease)
    {
    	this.host.logWithHostAndPartition(Level.INFO, lease.getPartitionId(), "Releasing lease");
    	
    	Boolean retval = true;
    	InMemoryLease leaseInStore = InMemoryLeaseStore.singleton.inMemoryLeases.get(lease.getPartitionId());
    	if (leaseInStore != null)
    	{
    		if (leaseInStore.getOwner().compareTo(this.host.getHostName()) == 0)
    		{
	    		this.host.logWithHostAndPartition(Level.INFO, lease.getPartitionId(), "releaseLease() released OK");
	    		leaseInStore.setOwner("");
	    		leaseInStore.setExpirationTime(0);
	    		lease.setExpirationTime(0);
    		}
    		else
    		{
	    		this.host.logWithHostAndPartition(Level.WARNING, lease.getPartitionId(), "releaseLease() not released because we don't own lease");
    			retval = false;
    		}
    	}
    	else
    	{
    		this.host.logWithHostAndPartition(Level.SEVERE, lease.getPartitionId(), "releaseLease() can't find lease");
    		retval = false;
    	}
    	return retval;
    }

    @Override
    public Future<Boolean> updateLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> updateLeaseSync((InMemoryLease)lease));
    }
    
    private Boolean updateLeaseSync(InMemoryLease lease)
    {
    	this.host.logWithHostAndPartition(Level.INFO, lease.getPartitionId(), "Updating lease");
    	
    	Boolean retval = true;
    	InMemoryLease leaseInStore = InMemoryLeaseStore.singleton.inMemoryLeases.get(lease.getPartitionId());
    	if (leaseInStore != null)
    	{
    		if (leaseInStore.getOwner().compareTo(this.host.getHostName()) == 0)
    		{
   				leaseInStore.setEpoch(lease.getEpoch());
    			leaseInStore.setToken(lease.getToken());
    			// Don't copy expiration time, that is managed directly by Acquire/Renew/Release
    		}
    		else
    		{
	    		this.host.logWithHostAndPartition(Level.WARNING, lease.getPartitionId(), "updateLease() not updated because we don't own lease");
    			retval = false;
    		}
    	}
    	else
    	{
    		this.host.logWithHostAndPartition(Level.SEVERE, lease.getPartitionId(), "updateLease() can't find lease");
    		retval = false;
    	}
    	return retval;
    }


    private static class InMemoryLeaseStore
    {
        public final static InMemoryLeaseStore singleton = new InMemoryLeaseStore();

        public HashMap<String, InMemoryLease> inMemoryLeases = null;
    }
    
    
    private static class InMemoryLease extends Lease
    {
    	private long expirationTimeMillis = 0;
    	
		public InMemoryLease(String eventHub, String consumerGroup, String partitionId)
		{
			super(eventHub, consumerGroup, partitionId);
		}
		
		public InMemoryLease(InMemoryLease source)
		{
			super(source);
			this.expirationTimeMillis = source.expirationTimeMillis;
		}
		
		public void setExpirationTime(long expireAtMillis)
		{
			this.expirationTimeMillis = expireAtMillis;
		}
		
		@Override
	    public boolean isExpired() throws Exception
	    {
			return (System.currentTimeMillis() >= this.expirationTimeMillis);
	    }
    }
}
