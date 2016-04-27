/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.HashMap;
import java.util.concurrent.Future;
import java.util.logging.Level;

//
// An ICheckpointManager implementation based on an in-memory store. This is obviously volatile
// and can only be shared among hosts within a process, but is useful for testing. Overall, its
// behavior is fairly close to that of AzureStorageCheckpointLeaseManager, but on the other hand
// it is completely separate from the InMemoryLeaseManager, to allow testing scenarios where
// the two stores are not combined.
//

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
        	this.host.logWithHost(Level.INFO, "createCheckpointStoreIfNotExists() creating in memory hashmap");
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
        Checkpoint checkpointInStore = InMemoryCheckpointStore.singleton.inMemoryCheckpoints.get(partitionId);
        if (checkpointInStore == null)
        {
        	
        	this.host.logWithHostAndPartition(Level.SEVERE, partitionId, "getCheckpoint() no existing Checkpoint");
        	returnCheckpoint = null;
        }
        else
        {
        	returnCheckpoint = new Checkpoint(checkpointInStore);
        }
        return returnCheckpoint;
    }
    
    @Override
    public Future<Checkpoint> createCheckpointIfNotExists(String partitionId)
    {
    	return EventProcessorHost.getExecutorService().submit(() -> createCheckpointIfNotExistsSync(partitionId));
    }
    
    private Checkpoint createCheckpointIfNotExistsSync(String partitionId)
    {
    	Checkpoint returnCheckpoint = InMemoryCheckpointStore.singleton.inMemoryCheckpoints.get(partitionId);
    	if (returnCheckpoint == null)
    	{
    		returnCheckpoint = new Checkpoint(partitionId);
    		InMemoryCheckpointStore.singleton.inMemoryCheckpoints.put(partitionId, returnCheckpoint);
    	}
    	return returnCheckpoint;
    }

    @Override
    public Future<Void> updateCheckpoint(Checkpoint checkpoint)
    {
        return EventProcessorHost.getExecutorService().submit(() -> updateCheckpointSync(checkpoint.getPartitionId(), checkpoint.getOffset(), checkpoint.getSequenceNumber()));
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
    		this.host.logWithHostAndPartition(Level.SEVERE, partitionId, "updateCheckpoint() can't find checkpoint");
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
