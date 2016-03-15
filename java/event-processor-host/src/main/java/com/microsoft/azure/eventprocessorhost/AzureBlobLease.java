/*
 * LICENSE GOES HERE TOO
 */

package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.storage.StorageException;
import com.microsoft.azure.storage.blob.BlobProperties;
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
	
	public String getStateDebug()
	{
		String retval = "uninitialized";
		try
		{
			this.blob.downloadAttributes();
			BlobProperties props = this.blob.getProperties();
			retval = props.getLeaseState().toString() + " " + props.getLeaseStatus().toString() + " " + props.getLeaseDuration().toString();
		}
		catch (StorageException e)
		{
			retval = "downloadAttributes on the blob caught " + e.toString();
		}
		return retval; 
	}

	@Override
	public boolean isExpired() throws Exception
	{
		this.blob.downloadAttributes(); // Get the latest metadata
		LeaseState currentState = this.blob.getProperties().getLeaseState();
		return (currentState != LeaseState.LEASED); 
	}
}
