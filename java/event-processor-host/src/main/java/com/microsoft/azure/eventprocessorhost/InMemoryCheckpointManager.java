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
    	return EventProcessorHost.getExecutorService().submit(() -> (InMemoryCheckpointStore.getSingleton().inMemoryCheckpoints != null));
    }

    @Override
    public Future<Boolean> createCheckpointStoreIfNotExists()
    {
        return EventProcessorHost.getExecutorService().submit(() -> createCheckpointStoreIfNotExistsSync());
    }

    private Boolean createCheckpointStoreIfNotExistsSync()
    {
        if (InMemoryCheckpointStore.getSingleton().inMemoryCheckpoints == null)
        {
        	this.host.logWithHost("createCheckpointStoreIfNotExists() creating in memory hashmap");
            InMemoryCheckpointStore.getSingleton().inMemoryCheckpoints = new HashMap<String, CheckPoint>();
        }
        return true;
    }
    
    @Override
    public Future<CheckPoint> getCheckpoint(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> getCheckpointSync(partitionId));
    }
    
    private CheckPoint getCheckpointSync(String partitionId)
    {
    	CheckPoint returnCheckpoint = null;
        CheckPoint CheckpointInStore = InMemoryCheckpointStore.getSingleton().inMemoryCheckpoints.get(partitionId);
        if (CheckpointInStore == null)
        {
        	this.host.logWithHostAndPartition(partitionId, "getCheckpoint() no existing Checkpoint");
        	returnCheckpoint = null;
        }
        else
        {
        	returnCheckpoint = new CheckPoint(CheckpointInStore);
        }
        return returnCheckpoint;
    }

    @Override
    public Future<Void> updateCheckpoint(CheckPoint checkpoint)
    {
    	return updateCheckpoint(checkpoint, checkpoint.getOffset(), checkpoint.getSequenceNumber());
    }

    @Override
    public Future<Void> updateCheckpoint(CheckPoint checkpoint, String offset, long sequenceNumber)
    {
        return EventProcessorHost.getExecutorService().submit(() -> updateCheckpointSync(checkpoint.getPartitionId(), offset, sequenceNumber));
    }

    private Void updateCheckpointSync(String partitionId, String offset, long sequenceNumber)
    {
    	CheckPoint checkpointInStore = InMemoryCheckpointStore.getSingleton().inMemoryCheckpoints.get(partitionId);
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
    	InMemoryCheckpointStore.getSingleton().inMemoryCheckpoints.remove(partitionId);
    	return null;
    }



    private static class InMemoryCheckpointStore
    {
        private static InMemoryCheckpointStore singleton = null;

        public static InMemoryCheckpointStore getSingleton()
        {
        	synchronized (InMemoryCheckpointStore.singleton)
        	{
	            if (InMemoryCheckpointStore.singleton == null)
	            {
	                InMemoryCheckpointStore.singleton = new InMemoryCheckpointStore();
	            }
        	}
            return InMemoryCheckpointStore.singleton;
        }

        public HashMap<String, CheckPoint> inMemoryCheckpoints = null;
    }
}
