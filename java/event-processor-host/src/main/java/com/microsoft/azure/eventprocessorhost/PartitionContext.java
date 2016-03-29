/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

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

    /**
     * Updates the offset/sequenceNumber in the PartitionContext with the values in the received EventData object.
     *  
     * Since offset is a string it cannot be compared easily, but sequenceNumber is checked. The new sequenceNumber must be
     * at least the same as the current value or the entire assignment is aborted. It is assumed that if the new sequenceNumber
     * is equal or greater, the new offset will be as well.
     * 
     * @param event  A received EventData with valid offset and sequenceNumber
     * @throws IllegalArgumentException  If the sequenceNumber in the provided event is less than the current value
     */
    public void setOffsetAndSequenceNumber(EventData event) throws IllegalArgumentException
    {
    	setOffsetAndSequenceNumber(event.getSystemProperties().getOffset(), event.getSystemProperties().getSequenceNumber());
    }
    
    /**
     * Updates the offset/sequenceNumber in the PartitionContext.
     * 
     * These two values are closely tied and must be updated in an atomic fashion, hence the combined setter.
     * Since offset is a string it cannot be compared easily, but sequenceNumber is checked. The new sequenceNumber must be
     * at least the same as the current value or the entire assignment is aborted. It is assumed that if the new sequenceNumber
     * is equal or greater, the new offset will be as well.
     * 
     * @param offset  New offset value
     * @param sequenceNumber  New sequenceNumber value 
     * @throws IllegalArgumentException  If the new sequenceNumber is less than the current value
     */
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

    /**
     * Writes the current offset and sequenceNumber to the checkpoint store via the checkpoint manager.
     */
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

    /**
     * Stores the offset and sequenceNumber from the provided received EventData instance, then writes those
     * values to the checkpoint store via the checkpoint manager.
     *  
     * @param event  A received EventData with valid offset and sequenceNumber
     * @throws IllegalArgumentException  If the sequenceNumber in the provided event is less than the current value  
     */
    public void checkpoint(EventData event) throws IllegalArgumentException
    {
    	setOffsetAndSequenceNumber(event.getSystemProperties().getOffset(), event.getSystemProperties().getSequenceNumber());
    	this.host.getCheckpointDispatcher().enqueueCheckpoint(new Checkpoint(this.partitionId, event.getSystemProperties().getOffset(), event.getSystemProperties().getSequenceNumber()));
    }
}
