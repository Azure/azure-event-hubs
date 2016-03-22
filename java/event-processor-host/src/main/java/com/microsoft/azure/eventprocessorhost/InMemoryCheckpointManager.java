/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.HashMap;
import java.util.concurrent.Future;


public class InMemoryCheckpointManager implements ICheckpointManager
{
    private EventProcessorHost host;

    public InMemoryCheckpointManager()
    {
    }

    // The EventProcessorHost can't pass itself to the constructor
    // because it is still being constructed.
    public void initialize(EventProcessorHost host)
    {
        this.host = host;
    }

    @Override
    public Future<Boolean> checkpointStoreExists()
    {
    	return EventProcessorHost.getExecutorService().submit(() -> checkpointStoreExistsSync());
    }
    
    private Boolean checkpointStoreExistsSync()
    {
    	return (InMemoryCheckpointStore.singleton.inMemoryCheckpoints != null);
    }
    

    @Override
    public Future<Boolean> createCheckpointStoreIfNotExists()
    {
        return EventProcessorHost.getExecutorService().submit(() -> createCheckpointStoreIfNotExistsSync());
    }

    private Boolean createCheckpointStoreIfNotExistsSync()
    {
        if (InMemoryCheckpointStore.singleton.inMemoryCheckpoints == null)
        {
        	this.host.logWithHost("createCheckpointStoreIfNotExists() creating in memory hashmap");
            InMemoryCheckpointStore.singleton.inMemoryCheckpoints = new HashMap<String, Checkpoint>();
        }
        return true;
    }
    
    @Override
    public Future<Checkpoint> getCheckpoint(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> getCheckpointSync(partitionId));
    }
    
    private Checkpoint getCheckpointSync(String partitionId)
    {
    	Checkpoint returnCheckpoint = null;
        Checkpoint CheckpointInStore = InMemoryCheckpointStore.singleton.inMemoryCheckpoints.get(partitionId);
        if (CheckpointInStore == null)
        {
        	this.host.logWithHostAndPartition(partitionId, "getCheckpoint() no existing Checkpoint");
        	returnCheckpoint = null;
        }
        else
        {
        	returnCheckpoint = new Checkpoint(CheckpointInStore);
        }
        return returnCheckpoint;
    }

    @Override
    public Future<Void> updateCheckpoint(Checkpoint checkpoint)
    {
    	return updateCheckpoint(checkpoint, checkpoint.getOffset(), checkpoint.getSequenceNumber());
    }

    @Override
    public Future<Void> updateCheckpoint(Checkpoint checkpoint, String offset, long sequenceNumber)
    {
        return EventProcessorHost.getExecutorService().submit(() -> updateCheckpointSync(checkpoint.getPartitionId(), offset, sequenceNumber));
    }

    private Void updateCheckpointSync(String partitionId, String offset, long sequenceNumber)
    {
    	Checkpoint checkpointInStore = InMemoryCheckpointStore.singleton.inMemoryCheckpoints.get(partitionId);
    	if (checkpointInStore != null)
    	{
    		checkpointInStore.setOffset(offset);
    		checkpointInStore.setSequenceNumber(sequenceNumber);
    	}
    	else
    	{
    		this.host.logWithHostAndPartition(partitionId, "updateCheckpoint() can't find checkpoint");
    	}
    	return null;
    }

    @Override
    public Future<Void> deleteCheckpoint(String partitionId)
    {
    	return EventProcessorHost.getExecutorService().submit(() -> deleteCheckpointSync(partitionId));
    }
    
    private Void deleteCheckpointSync(String partitionId)
    {
    	InMemoryCheckpointStore.singleton.inMemoryCheckpoints.remove(partitionId);
    	return null;
    }



    private static class InMemoryCheckpointStore
    {
        private final static InMemoryCheckpointStore singleton = new InMemoryCheckpointStore();

        public HashMap<String, Checkpoint> inMemoryCheckpoints = null;
    }
}
