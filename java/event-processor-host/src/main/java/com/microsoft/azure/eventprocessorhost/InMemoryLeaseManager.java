/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.*;
import java.util.logging.Level;

public class InMemoryLeaseManager implements ILeaseManager
{
    private EventProcessorHost host;

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
    	// Leases don't expire in this manager but we want the partition manager loop to execute reasonably often.
    	return 10 * 1000;
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
            InMemoryLeaseStore.singleton.inMemoryLeases = new HashMap<String, Lease>();
        }
        return true;
    }
    
    @Override
    public Future<Lease> getLease(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> getLeaseSync(partitionId));
    }

    private Lease getLeaseSync(String partitionId)
    {
    	Lease returnLease = null;
        Lease leaseInStore = InMemoryLeaseStore.singleton.inMemoryLeases.get(partitionId);
        if (leaseInStore == null)
        {
        	this.host.logWithHostAndPartition(Level.WARNING, partitionId, "getLease() no existing lease");
        	returnLease = null;
        }
        else
        {
        	returnLease = new Lease(leaseInStore);
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

    private Lease createLeaseIfNotExistsSync(String partitionId)
    {
    	Lease returnLease = InMemoryLeaseStore.singleton.inMemoryLeases.get(partitionId);
        if (returnLease != null)
        {
        	this.host.logWithHostAndPartition(Level.INFO, partitionId, "createLeaseIfNotExists() found existing lease, OK");
        }
        else
        {
        	this.host.logWithHostAndPartition(Level.INFO, partitionId, "createLeaseIfNotExists() creating new lease");
            Lease storeLease = new Lease(this.host.getEventHubPath(), this.host.getConsumerGroupName(), partitionId);
            storeLease.setEpoch(0L);
            storeLease.setOwner(this.host.getHostName());
            InMemoryLeaseStore.singleton.inMemoryLeases.put(partitionId, storeLease);
            returnLease = new Lease(storeLease);
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
    	InMemoryLeaseStore.singleton.inMemoryLeases.remove(lease.getPartitionId());
    	return null;
    }

    @Override
    public Future<Boolean> acquireLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> acquireLeaseSync(lease));
    }

    private Boolean acquireLeaseSync(Lease lease)
    {
    	Boolean retval = true;
        Lease leaseInStore = InMemoryLeaseStore.singleton.inMemoryLeases.get(lease.getPartitionId());
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
    	// No-op at this time
        return EventProcessorHost.getExecutorService().submit(() -> true);
    }

    @Override
    public Future<Boolean> releaseLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> releaseLeaseSync(lease));
    }
    
    private Boolean releaseLeaseSync(Lease lease)
    {
    	Boolean retval = true;
    	Lease leaseInStore = InMemoryLeaseStore.singleton.inMemoryLeases.get(lease.getPartitionId());
    	if (leaseInStore != null)
    	{
    		if (leaseInStore.getOwner().compareTo(this.host.getHostName()) == 0)
    		{
	    		this.host.logWithHostAndPartition(Level.INFO, lease.getPartitionId(), "releaseLease() released OK");
	    		leaseInStore.setOwner("");
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
        return EventProcessorHost.getExecutorService().submit(() -> updateLeaseSync(lease));
    }
    
    private Boolean updateLeaseSync(Lease lease)
    {
    	Boolean retval = true;
    	Lease leaseInStore = InMemoryLeaseStore.singleton.inMemoryLeases.get(lease.getPartitionId());
    	if (leaseInStore != null)
    	{
    		if (leaseInStore.getOwner().compareTo(this.host.getHostName()) == 0)
    		{
   				leaseInStore.setEpoch(lease.getEpoch());
    			leaseInStore.setToken(lease.getToken());
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

        public HashMap<String, Lease> inMemoryLeases = null;
    }
}


