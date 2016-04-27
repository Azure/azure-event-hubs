/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

class Lease
{
    private final String partitionId;

    private long epoch;
    private String owner;
    private String token;

    Lease(String partitionId)
    {
        this.partitionId = partitionId;

        this.epoch = 0;
        this.owner = "";
        this.token = "";
    }

    Lease(Lease source)
    {
        this.partitionId = source.partitionId;

        this.epoch = source.epoch;
        this.owner = source.owner;
        this.token = source.token;
    }

    long getEpoch()
    {
        return this.epoch;
    }

    void setEpoch(long epoch)
    {
        this.epoch = epoch;
    }
    
    long incrementEpoch()
    {
    	this.epoch++;
    	return this.epoch;
    }
    
    String getOwner()
    {
        return this.owner;
    }

    void setOwner(String owner)
    {
        this.owner = owner;
    }

    String getPartitionId()
    {
        return this.partitionId;
    }

    String getToken()
    {
        return this.token;
    }

    void setToken(String token)
    {
        this.token = token;
    }

    boolean isExpired() throws Exception
    {
    	// this function is meaningless in the base class
    	return false;
    }
}
