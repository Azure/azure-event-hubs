package com.microsoft.azure.eventprocessorhost;

public class Lease
{
    private String eventHubPath;
    private String consumerGroup;
    private String partitionId;

    private long epoch;
    private String offset;
    private String owner;
    private long sequenceNumber;
    private String token;

    public Lease(String eventHub, String consumerGroup, String partitionId)
    {
        this.eventHubPath = eventHub;
        this.consumerGroup = consumerGroup;
        this.partitionId = partitionId;

        this.epoch = 0;
        this.offset = "-1"; // magic number
        this.owner = "";
        this.sequenceNumber = 0;
        this.token = "";
    }

    public Lease(Lease source)
    {
        this.eventHubPath = source.eventHubPath;
        this.consumerGroup = source.consumerGroup;
        this.partitionId = source.partitionId;

        this.epoch = source.epoch;
        this.offset = source.offset;
        this.owner = source.owner;
        this.sequenceNumber = source.sequenceNumber;
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

    public String getOffset()
    {
        return this.offset;
    }

    public void setOffset(String offset)
    {
        this.offset = offset;
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

    public long getSequenceNumber()
    {
        return this.sequenceNumber;
    }

    public void setSequenceNumber(long sequenceNumber)
    {
        this.sequenceNumber = sequenceNumber;
    }

    public String getToken()
    {
        return this.token;
    }

    public void setToken(String token)
    {
        this.token = token;
    }

    private Boolean isExpired()
    {
    	// .NET always returns false
    	return false;
    }
}
