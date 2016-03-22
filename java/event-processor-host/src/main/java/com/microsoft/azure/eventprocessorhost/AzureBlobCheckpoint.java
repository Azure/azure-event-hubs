/*
 * LICENSE GOES HERE TOO
 */

package com.microsoft.azure.eventprocessorhost;

public class AzureBlobCheckPoint extends CheckPoint
{
	private AzureBlobLease lease;
	
	public AzureBlobCheckPoint(AzureBlobLease lease)
	{
		super(lease.getPartitionId());
		this.lease = lease;
	}
	
	public AzureBlobCheckPoint(AzureBlobCheckPoint source)
	{
		super(source);
		this.lease = source.lease;
	}
	
	public AzureBlobCheckPoint(CheckPoint source, AzureBlobLease lease)
	{
		super(source);
		this.lease = lease;
	}
	
	AzureBlobLease getLease()
	{
		return this.lease;
	}
}
