/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

// BLAH

package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.ExecutionException;
import java.util.function.Function;
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
    
    String getInitialOffset() throws InterruptedException, ExecutionException
    {
    	Function<String, String> initialOffsetProvider = this.host.getEventProcessorOptions().getInitialOffsetProvider();
    	if (initialOffsetProvider != null)
    	{
    		this.host.logWithHostAndPartition(Level.FINE, this.partitionId, "Calling user-provided initial offset provider");
    		this.offset = initialOffsetProvider.apply(this.partitionId);
    		this.sequenceNumber = 0; // TODO we use sequenceNumber to check for regression of offset, 0 could be a problem until it gets updated from an event
	    	this.host.logWithHostAndPartition(Level.FINE, this.partitionId, "Initial offset provided: " + this.offset + "//" + this.sequenceNumber);
    	}
    	else
    	{
	    	Checkpoint startingCheckpoint = this.host.getCheckpointManager().getCheckpoint(this.partitionId).get();
	    	this.offset = startingCheckpoint.getOffset();
	    	this.sequenceNumber = startingCheckpoint.getSequenceNumber();
	    	this.host.logWithHostAndPartition(Level.FINE, this.partitionId, "Retrieved starting offset " + this.offset + "//" + this.sequenceNumber);
    	}
    	return this.offset;
    }

    public void checkpoint()
    {
    	// Capture the current offset and sequenceNumber. Synchronize to be sure we get a matched pair
    	// instead of catching an update halfway through. Do the capturing here because by the time the checkpoint
    	// task runs, the fields in this object may have changed, but we should only write to store what the user
    	// has directed us to write.
    	Checkpoint capturedCheckpoint = null;
    	synchronized (this.offset)
    	{
    		capturedCheckpoint = new Checkpoint(this.partitionId, this.offset, this.sequenceNumber);
    	}
    	this.host.getCheckpointDispatcher().enqueueCheckpoint(capturedCheckpoint);
    }

    public void checkpoint(EventData event)
    {
    	setOffsetAndSequenceNumber(event.getSystemProperties().getOffset(), event.getSystemProperties().getSequenceNumber());
    	this.host.getCheckpointDispatcher().enqueueCheckpoint(new Checkpoint(this.partitionId, event.getSystemProperties().getOffset(), event.getSystemProperties().getSequenceNumber()));
    }
}
