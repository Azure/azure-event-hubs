/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.ArrayList;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Future;
import java.util.logging.Level;
import java.util.concurrent.Callable;


class Pump
{
    private EventProcessorHost host;

    private ConcurrentHashMap<String, LeaseAndPump> pumpStates;
    
    private Class<?> pumpClass = EventHubPartitionPump.class;
    
    private class LeaseAndPump implements Callable<Void>
    {
    	private Lease lease;
    	private PartitionPump pump;
    	
    	public LeaseAndPump(Lease lease, PartitionPump pump)
    	{
    		this.lease = lease;
    		this.pump = pump;
    	}
    	
    	public void setLease(Lease newLease)
    	{
    		this.lease = newLease;
    		this.pump.setLease(newLease);
    	}
    	
    	public PartitionPump getPump()
    	{
    		return this.pump;
    	}
    	
		@Override
		public Void call() throws Exception
		{
			this.pump.startPump();
			return null;
		}
    }

    public Pump(EventProcessorHost host)
    {
        this.host = host;

        this.pumpStates = new ConcurrentHashMap<String, LeaseAndPump>();
    }
    
    public <T extends PartitionPump> void setPumpClass(Class<T> pumpClass)
    {
    	this.pumpClass = pumpClass;
    }

    public void addPump(String partitionId, Lease lease) throws Exception
    {
    	LeaseAndPump capturedState = this.pumpStates.get(partitionId);
    	if (capturedState != null)
    	{
    		// There already is a pump. Make sure the pump is working and replace the lease.
    		if ((capturedState.getPump().getPumpStatus() == PartitionPumpStatus.PP_ERRORED) || capturedState.getPump().isClosing())
    		{
    			// The existing pump is bad. Remove it and create a new one.
    			removePump(partitionId, CloseReason.Shutdown).get();
    			createNewPump(partitionId, lease);
    		}
    		else
    		{
    			// Pump is working, just replace the lease.
    			this.host.logWithHostAndPartition(Level.FINE, partitionId, "updating lease for pump");
    			capturedState.setLease(lease);
    		}
    	}
    	else
    	{
    		// No existing pump, create a new one.
    		createNewPump(partitionId, lease);
    	}
    }
    
    private void createNewPump(String partitionId, Lease lease) throws Exception
    {
		PartitionPump newPartitionPump = (PartitionPump)this.pumpClass.newInstance();
		newPartitionPump.initialize(this.host, lease);
		LeaseAndPump newPump = new LeaseAndPump(lease, newPartitionPump);
		EventProcessorHost.getExecutorService().submit(newPump);
        this.pumpStates.put(partitionId, newPump); // do the put after start, if the start fails then put doesn't happen
		this.host.logWithHostAndPartition(Level.INFO, partitionId, "created new pump");
    }
    
    public Future<?> removePump(String partitionId, final CloseReason reason)
    {
    	Future<?> retval = null;
    	LeaseAndPump capturedState = this.pumpStates.get(partitionId);
    	if (capturedState != null)
    	{
			this.host.logWithHostAndPartition(Level.INFO, partitionId, "closing pump for reason " + reason.toString());
    		if (!capturedState.getPump().isClosing())
    		{
    			retval = EventProcessorHost.getExecutorService().submit(() -> capturedState.getPump().shutdown(reason));
    		}
    		// else, pump is already closing/closed, don't need to try to shut it down again
    		
    		this.host.logWithHostAndPartition(Level.INFO, partitionId, "removing pump");
    		this.pumpStates.remove(partitionId);
    	}
    	else
    	{
    		this.host.logWithHostAndPartition(Level.WARNING, partitionId, "no pump found to remove for partition " + partitionId);
    	}
    	return retval;
    }
    
    public Iterable<Future<?>> removeAllPumps(CloseReason reason)
    {
    	ArrayList<Future<?>> futures = new ArrayList<Future<?>>();
    	for (String partitionId : this.pumpStates.keySet())
    	{
    		futures.add(removePump(partitionId, reason));
    	}
    	return futures;
    }
    
    public boolean hasPump(String partitionId)
    {
    	return this.pumpStates.containsKey(partitionId);
    }
}
