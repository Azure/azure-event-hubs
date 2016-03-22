/*
 * LICENSE GOES HERE TOO
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.Future;

// WILL NORMALLY BE IMPLEMENTED ON THE SAME OBJECT AS ICheckpointManager
public interface ILeaseManager
{
	public int getLeaseRenewIntervalInMilliseconds();
	
    public Future<Boolean> leaseStoreExists();

    public Future<Boolean> createLeaseStoreIfNotExists();

    public Future<Lease> getLease(String partitionId);
    public Iterable<Future<Lease>> getAllLeases();

    public Future<Lease> createLeaseIfNotExists(String partitionId);

    public Future<Void> deleteLease(Lease lease);

    public Future<Boolean> acquireLease(Lease lease);

    public Future<Boolean> renewLease(Lease lease);

    public Future<Boolean> releaseLease(Lease lease);

    public Future<Boolean> updateLease(Lease lease);
}
