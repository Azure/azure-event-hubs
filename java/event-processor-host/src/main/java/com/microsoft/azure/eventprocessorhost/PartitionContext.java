/*
 * LICENSE GOES HERE TOO
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.Future;
import com.microsoft.azure.eventhubs.EventData;

public class PartitionContext
{
    private ICheckpointManager checkpointManager;
    private String consumerGroupName;
    private String eventHubPath;
    private Lease lease;
    private String partitionId;

    private PartitionContext()
    {
        // FORBIDDEN!
    }

    PartitionContext(ICheckpointManager checkpointManager, String partitionId)
    {
        this.checkpointManager = checkpointManager;
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
    
    public String getPartitionId()
    {
    	return this.partitionId;
    }

    public Future<Void> checkpoint()
    {
        return this.checkpointManager.updateCheckpoint(this.lease.getCheckpoint());
    }

    public Future<Void> checkpoint(EventData data)
    {
        return checkpointManager.updateCheckpoint(this.lease.getCheckpoint(), data.getSystemProperties().getOffset(), data.getSystemProperties().getSequenceNumber());
    }
}
