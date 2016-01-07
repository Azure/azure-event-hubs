package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;

// TODO class which implements this interface using Azure Storage.
public interface ICheckpointManager
{
    public void initializeCheckpointManager(String eventHub, String consumerGroup, ExecutorService executorService);

    public Future<Boolean> checkpointStoreExists();

    public Future<Boolean> createCheckpointStoreIfNotExists();

    public Future<String> getCheckpoint(String partitionId);
    public Iterable<Future<String>> getAllCheckpoints();

    public Future<Void> updateCheckpoint(String partitionId, String offset);

    public Future<Void> deleteCheckpoint(String partitionId);

}
