/*
 * LICENSE GOES HERE TOO
 */

package com.microsoft.azure.eventprocessorhost;

public class AzureBlobCheckpoint extends Checkpoint
{
	private AzureBlobLease lease;
	
	public AzureBlobCheckpoint(AzureBlobLease lease)
	{
		super(lease.getPartitionId());
		this.lease = lease;
		this.setOffset(lease.getOffset());
		this.setSequenceNumber(lease.getSequenceNumber());
	}
	
	public AzureBlobCheckpoint(AzureBlobCheckpoint source)
	{
		super(source);
		this.lease = source.lease;
	}
	
	public AzureBlobCheckpoint(Checkpoint source, AzureBlobLease lease)
	{
		super(source);
		this.lease = lease;
	}
	
	void setLease(AzureBlobLease lease)
	{
		this.lease = lease;
	}
}
