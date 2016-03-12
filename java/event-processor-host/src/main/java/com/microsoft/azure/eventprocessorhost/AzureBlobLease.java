/*
 * LICENSE GOES HERE TOO
 */

package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.storage.blob.CloudBlockBlob;
import com.microsoft.azure.storage.blob.LeaseState;

public class AzureBlobLease extends Lease
{
	private transient CloudBlockBlob blob; // do not serialize
	
	public AzureBlobLease(String eventHub, String consumerGroup, String partitionId, CloudBlockBlob blob)
	{
		super(eventHub, consumerGroup, partitionId);
		this.blob = blob;
	}
	
	public AzureBlobLease(AzureBlobLease source)
	{
		super(source);
		this.blob = source.blob;
	}
	
	public AzureBlobLease(Lease source, CloudBlockBlob blob)
	{
		super(source);
		this.blob = blob;
	}
	
	public CloudBlockBlob getBlob() { return this.blob; }
	
	@Override
	public CheckPoint getCheckpoint()
	{
		return new AzureBlobCheckPoint(this.checkpoint, this);
	}

	@Override
	public boolean isExpired()
	{
		return (this.blob.getProperties().getLeaseState() != LeaseState.LEASED);
	}
}
