/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

public class Lease
{
    private String eventHubPath;
    private String consumerGroup;
    private String partitionId;

    private long epoch;
    private String owner;
    private String token;

    public Lease(String eventHub, String consumerGroup, String partitionId)
    {
        this.eventHubPath = eventHub;
        this.consumerGroup = consumerGroup;
        this.partitionId = partitionId;

        this.epoch = 0;
        this.owner = "";
        this.token = "";
    }

    public Lease(Lease source)
    {
        this.eventHubPath = source.eventHubPath;
        this.consumerGroup = source.consumerGroup;
        this.partitionId = source.partitionId;

        this.epoch = source.epoch;
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

    public String getToken()
    {
        return this.token;
    }

    public void setToken(String token)
    {
        this.token = token;
    }

    public boolean isExpired() throws Exception
    {
    	// this function is meaningless in the base class
    	return false;
    }
}
