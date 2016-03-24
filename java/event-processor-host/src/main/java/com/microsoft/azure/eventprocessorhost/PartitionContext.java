/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.logging.Level;

import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.PartitionReceiver;

public class PartitionContext
{
	private EventProcessorHost host;
    private String consumerGroupName;
    private String eventHubPath;
    private Lease lease;
    private String partitionId;
    private String offset = PartitionReceiver.START_OF_STREAM;
    private long sequenceNumber = 0;;
    
    private Object updateSynchronizer = new Object();
    

    PartitionContext(EventProcessorHost host, String partitionId)
    {
        this.host = host;
        this.partitionId = partitionId;
    }

    public String getConsumerGroupName()
    {
        return this.consumerGroupName;
    }

    public void setConsumerGroupName(String consumerGroupName)
    {
        this.consumerGroupName = consumerGroupName;
    }

    public String getEventHubPath()
    {
        return this.eventHubPath;
    }

    public void setEventHubPath(String eventHubPath)
    {
        this.eventHubPath = eventHubPath;
    }

    public Lease getLease()
    {
        return this.lease;
    }

    public void setLease(Lease lease)
    {
        this.lease = lease;
    }
    
    public void setOffsetAndSequenceNumber(EventData event)
    {
    	setOffsetAndSequenceNumber(event.getSystemProperties().getOffset(), event.getSystemProperties().getSequenceNumber());
    }
    
    public void setOffsetAndSequenceNumber(String offset, long sequenceNumber) throws IllegalArgumentException
    {
    	synchronized (this.offset)
    	{
    		if (sequenceNumber >= this.sequenceNumber)
    		{
    			this.offset = offset;
    			this.sequenceNumber = sequenceNumber;
    		}
    		else
    		{
    			throw new IllegalArgumentException("new offset " + offset + "//" + sequenceNumber + " less than old " + this.offset + "//" + this.sequenceNumber);
    		}
    	}
    }
    
    public String getPartitionId()
    {
    	return this.partitionId;
    }
    
    String getStartingOffset() throws InterruptedException, ExecutionException
    {
    	Checkpoint startingCheckpoint = this.host.getCheckpointManager().getCheckpoint(this.partitionId).get();
    	this.offset = startingCheckpoint.getOffset();
    	this.sequenceNumber = startingCheckpoint.getSequenceNumber();
    	this.host.logWithHostAndPartition(Level.FINE, this.partitionId, "Retrieved starting offset " + this.offset + "//" + this.sequenceNumber);
    	return this.offset;
    }

    public Future<Void> checkpoint() throws InterruptedException, ExecutionException
    {
    	// Capture the current offset and sequenceNumber. Synchronize to be sure we get a matched pair
    	// instead of catching an update halfway through. Do the capturing here because by the time the checkpoint
    	// task runs, the fields in this object may have changed, but we should only write to store what the user
    	// has directed us to write.
    	String capturedOffset;
    	long capturedSequenceNumber;
    	synchronized (this.offset)
    	{
    		capturedOffset = this.offset;
    		capturedSequenceNumber = this.sequenceNumber;
    	}
    	return EventProcessorHost.getExecutorService().submit(() -> checkpointSync(capturedOffset, capturedSequenceNumber));
    }

    public Future<Void> checkpoint(EventData event) throws InterruptedException, ExecutionException
    {
    	setOffsetAndSequenceNumber(event.getSystemProperties().getOffset(), event.getSystemProperties().getSequenceNumber());
    	return EventProcessorHost.getExecutorService().submit(() -> checkpointSync(event.getSystemProperties().getOffset(), event.getSystemProperties().getSequenceNumber()));
    }
    
    // Checkpointing needs to be async, otherwise when called from IEventProcessor.onEvents it could block the javaClient pump thread.
    // However, making checkpointing async means that multiple checkpointing tasks for the same partition can race against each other.
    // Synchronize the read/modify/write cycle on this.updateSynchronizer, so there is only one rmw cycle going on at a time. The cycles
    // may be executed out of order, so before doing the modify-write we check whether the captured state is 
    // out of date and abandon the update if it is. Since this is an expected possibility, we don't throw on abandon, just log.
    private Void checkpointSync(String capturedOffset, long capturedSequenceNumber) throws InterruptedException, ExecutionException
    {
    	this.host.logWithHostAndPartition(Level.FINE, this.partitionId, "Checkpoint task starting for " + capturedOffset + "//" + capturedSequenceNumber);

    	synchronized (this.updateSynchronizer)
    	{
	    	Checkpoint inStoreCheckpoint = this.host.getCheckpointManager().getCheckpoint(this.partitionId).get();
	    	if (capturedSequenceNumber >= inStoreCheckpoint.getSequenceNumber())
	    	{
		    	inStoreCheckpoint.setOffset(capturedOffset);
		    	inStoreCheckpoint.setSequenceNumber(capturedSequenceNumber);
		        this.host.getCheckpointManager().updateCheckpoint(inStoreCheckpoint).get();
	    	}
	    	else
	    	{
	    		// Another checkpoint task has already updated the in-store checkpoint beyond what we have. That's fine.
	    		this.host.logWithHostAndPartition(Level.FINE, this.partitionId, "Abandoning out of date checkpoint " + capturedOffset + "//" + capturedSequenceNumber);
	    	}
    	}
        
        this.host.logWithHostAndPartition(Level.FINE, this.partitionId, "Checkpoint task ending");
        return null;
    }
}
