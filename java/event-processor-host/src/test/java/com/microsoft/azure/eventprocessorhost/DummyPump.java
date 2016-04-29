package com.microsoft.azure.eventprocessorhost;

import java.util.HashSet;
import java.util.concurrent.Future;

class DummyPump extends Pump
{
	private HashSet<String> pumps = new HashSet<String>();
	
	public DummyPump(EventProcessorHost host)
	{
		super(host);
	}
	
	Iterable<String> getPumpsList()
	{
		return this.pumps;
	}

	//
	// Completely override all functionality.
	//
	
	@Override
    public void addPump(String partitionId, Lease lease) throws Exception
    {
		this.pumps.add(partitionId);
    }
    
	@Override
    public Future<?> removePump(String partitionId, final CloseReason reason)
    {
		this.pumps.remove(partitionId);
    	return null; // PartitionManager does not use the returned future
    }
    
	@Override
    public Iterable<Future<?>> removeAllPumps(CloseReason reason)
    {
		this.pumps.clear();
		return null;
    }
    
	@Override
    public boolean hasPump(String partitionId)
    {
		return this.pumps.contains(partitionId);
    }    
}
