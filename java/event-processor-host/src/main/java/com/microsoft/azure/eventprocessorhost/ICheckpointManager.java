package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.Future;

// WILL NORMALLY BE IMPLEMENTED ON THE SAME CLASS AS ILeaseManager
public interface ICheckpointManager
{
    public Future<Boolean> checkpointStoreExists();

    public Future<Boolean> createCheckpointStoreIfNotExists();

    public Future<String> getCheckpoint(String partitionId);
    public Iterable<Future<String>> getAllCheckpoints();

    public Future<Void> updateCheckpoint(String partitionId, String offset);

    public Future<Void> deleteCheckpoint(String partitionId);

}
