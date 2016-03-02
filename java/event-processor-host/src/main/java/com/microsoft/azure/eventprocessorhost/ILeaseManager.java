/*
 * LICENSE GOES HERE
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.Future;

// WILL NORMALLY BE IMPLEMENTED ON THE SAME OBJECT AS ICheckpointManager
// REQUIRES IManagerBase also
public interface ILeaseManager
{
    public Future<Boolean> leaseStoreExists();

    public Future<Boolean> createLeaseStoreIfNotExists();

    public Future<Lease> getLease(String partitionId);
    public Iterable<Future<Lease>> getAllLeases();

    public Future<Void> createLeaseIfNotExists(String partitionId);

    public Future<Void> deleteLease(String partitionId);

    public Future<Lease> acquireLease(String partitionId);

    public Future<Boolean> renewLease(Lease lease);

    public Future<Boolean> releaseLease(Lease lease);

    public Future<Boolean> updateLease(Lease lease);
}
