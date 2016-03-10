/*
 * LICENSE GOES HERE
 */

package com.microsoft.azure.eventprocessorhost;

public class Lease
{
    private String eventHubPath;
    private String consumerGroup;
    private String partitionId;

    private long epoch;
    protected CheckPoint checkpoint;
    private String owner;
    private String token;

    public Lease(String eventHub, String consumerGroup, String partitionId)
    {
        this.eventHubPath = eventHub;
        this.consumerGroup = consumerGroup;
        this.partitionId = partitionId;

        this.epoch = 0;
        this.checkpoint = new CheckPoint(this.partitionId);
        this.checkpoint.setOffset(""); // empty string is magic
        this.checkpoint.setSequenceNumber(0);
        this.owner = "";
        this.token = "";
    }

    public Lease(Lease source)
    {
        this.eventHubPath = source.eventHubPath;
        this.consumerGroup = source.consumerGroup;
        this.partitionId = source.partitionId;

        this.epoch = source.epoch;
        this.checkpoint = new CheckPoint(source.getCheckpoint());
        this.owner = source.owner;
        this.token = source.token;
    }

    public long getEpoch()
    {
        return this.epoch;
    }

    public void setEpoch(long epoch)
    {
        this.epoch = epoch;
    }
    
    public long incrementEpoch()
    {
    	this.epoch++;
    	return this.epoch;
    }
    
    public CheckPoint getCheckpoint()
    {
    	return new CheckPoint(this.checkpoint);
    }

    public void setOffset(String offset)
    {
        this.checkpoint.setOffset(offset);
    }

    public String getOwner()
    {
        return this.owner;
    }

    public void setOwner(String owner)
    {
        this.owner = owner;
    }

    public String getPartitionId()
    {
        return this.partitionId;
    }

    public String getEventHubPath()
    {
        return this.eventHubPath;
    }

    public String getConsumerGroup()
    {
        return this.consumerGroup;
    }

    public void setSequenceNumber(long sequenceNumber)
    {
        this.checkpoint.setSequenceNumber(sequenceNumber);
    }

    public String getToken()
    {
        return this.token;
    }

    public void setToken(String token)
    {
        this.token = token;
    }

    public boolean isExpired()
    {
    	// this function is meaningless in the base class
    	return false;
    }
}
