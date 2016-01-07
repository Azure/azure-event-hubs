package com.microsoft.azure.eventprocessorhost;

import sun.reflect.generics.reflectiveObjects.NotImplementedException;

import java.util.ArrayList;
import java.util.concurrent.*;


public class AzureStorageCheckpointLeaseManager implements ICheckpointManager, ILeaseManager
{
    private ExecutorService executorService = null;
    private ArrayList<String> partitionIds = null;
    private String storageConnectionString;

    public AzureStorageCheckpointLeaseManager(String storageConnectionString)
    {
        this.storageConnectionString = storageConnectionString;
    }

    public void initializeCombinedManager(String eventHub, String consumerGroup, ExecutorService executorService)
    {
        this.executorService = executorService;
        this.partitionIds = new ArrayList<String>();
        this.partitionIds.add("0"); // DUMMY
        this.partitionIds.add("1"); // DUMMY
        this.partitionIds.add("2"); // DUMMY
        this.partitionIds.add("3"); // DUMMY
    }

    public void initializeCheckpointManager(String eventHub, String consumerGroup, ExecutorService executorService)
    {
        // Use initializeCombinedManager instead
        throw new NotImplementedException();
    }

    public Future<Boolean> checkpointStoreExists()
    {
        return this.executorService.submit(new CheckpointStoreExistsCallable());
    }

    public Future<Boolean> createCheckpointStoreIfNotExists()
    {
        return this.executorService.submit(new CreateCheckpointStoreIfNotExistsCallable());
    }

    public Future<String> getCheckpoint(String partitionId)
    {
        return this.executorService.submit(new GetCheckpointCallable(partitionId));
    }

    public Iterable<Future<String>> getAllCheckpoints()
    {
        ArrayList<Future<String>> checkpoints = new ArrayList<Future<String>>();
        for (String id : this.partitionIds)
        {
            checkpoints.add(getCheckpoint(id));
        }
        return checkpoints;
    }

    public Future<Void> updateCheckpoint(String partitionId, String offset)
    {
        return this.executorService.submit(new UpdateCheckpointCallable(partitionId, offset));
    }

    public Future<Void> deleteCheckpoint(String partitionId)
    {
        return this.executorService.submit(new DeleteCheckpointCallable(partitionId));
    }

    public void initializeLeaseManager(String eventHub, String consumerGroup, ExecutorService executorService)
    {
        // Use initializeCombinedManager instead
        throw new NotImplementedException();
    }


    public Future<Boolean> leaseStoreExists()
    {
        return this.executorService.submit(new LeaseStoreExistsCallable());
    }

    public Future<Boolean> createLeaseStoreIfNotExists()
    {
        return this.executorService.submit(new CreateLeaseStoreIfNotExistsCallable());
    }

    public Future<Lease> getLease(String partitionId)
    {
        return this.executorService.submit(new GetLeaseCallable(partitionId));
    }

    public Iterable<Future<Lease>> getAllLeases()
    {
        ArrayList<Future<Lease>> leases = new ArrayList<Future<Lease>>();
        for (String id : this.partitionIds)
        {
            leases.add(getLease(id));
        }
        return leases;
    }

    public Future<Void> createLeaseIfNotExists(String partitionId)
    {
        return this.executorService.submit(new CreateLeaseIfNotExistsCallable(partitionId));
    }

    public Future<Void> deleteLease(String partitionId)
    {
        return this.executorService.submit(new DeleteLeaseCallable(partitionId));
    }

    public Future<Lease> acquireLease(String partitionId)
    {
        return this.executorService.submit(new AcquireLeaseCallable(partitionId));
    }

    public Future<Boolean> renewLease(Lease lease)
    {
        return this.executorService.submit(new RenewLeaseCallable(lease));
    }

    public Future<Boolean> releaseLease(Lease lease)
    {
        return this.executorService.submit(new ReleaseLeaseCallable(lease));
    }

    public Future<Boolean> updateLease(Lease lease)
    {
        return this.executorService.submit(new UpdateLeaseCallable(lease));
    }


    private class CheckpointStoreExistsCallable implements Callable<Boolean>
    {
        public Boolean call()
        {
            return false;
        }
    }

    private class CreateCheckpointStoreIfNotExistsCallable implements Callable<Boolean>
    {
        public Boolean call()
        {
            return false;
        }
    }

    private class GetCheckpointCallable implements Callable<String>
    {
        private String partitionId;

        public GetCheckpointCallable(String partitionId)
        {
            this.partitionId = partitionId;
        }

        public String call()
        {
            return "";
        }
    }

    private class UpdateCheckpointCallable implements Callable<Void>
    {
        private String partitionId;
        private String offset;

        public UpdateCheckpointCallable(String partitionId, String offset)
        {
            this.partitionId = partitionId;
            this.offset = offset;
        }

        public Void call()
        {
            return null;
        }
    }

    private class DeleteCheckpointCallable implements Callable<Void>
    {
        private String partitionId;

        public DeleteCheckpointCallable(String partitionId)
        {
            this.partitionId = partitionId;
        }

        public Void call()
        {
            return null;
        }
    }


    private class LeaseStoreExistsCallable implements Callable<Boolean>
    {
        public Boolean call()
        {
            return false;
        }
    }

    private class CreateLeaseStoreIfNotExistsCallable implements Callable<Boolean>
    {
        public Boolean call()
        {
            return false;
        }
    }

    private class GetLeaseCallable implements Callable<Lease>
    {
        private String partitionId;

        public GetLeaseCallable(String partitionId)
        {
            this.partitionId = partitionId;
        }

        public Lease call()
        {
            return null;
        }
    }

    private class CreateLeaseIfNotExistsCallable implements Callable<Void>
    {
        private String partitionId;

        public CreateLeaseIfNotExistsCallable(String partitionId)
        {
            this.partitionId = partitionId;
        }

        public Void call()
        {
            return null;
        }
    }

    private class DeleteLeaseCallable implements Callable<Void>
    {
        private String partitionId;

        public DeleteLeaseCallable(String partitionId)
        {
            this.partitionId = partitionId;
        }

        public Void call()
        {
            return null;
        }
    }

    private class AcquireLeaseCallable implements Callable<Lease>
    {
        private String partitionId;

        public AcquireLeaseCallable(String partitionId)
        {
            this.partitionId = partitionId;
        }

        public Lease call()
        {
            return null;
        }
    }

    private class RenewLeaseCallable implements Callable<Boolean>
    {
        private Lease lease;

        public RenewLeaseCallable(Lease lease)
        {
            this.lease = lease;
        }

        public Boolean call()
        {
            return false;
        }
    }

    private class ReleaseLeaseCallable implements Callable<Boolean>
    {
        private Lease lease;

        public ReleaseLeaseCallable(Lease lease)
        {
            this.lease = lease;
        }

        public Boolean call()
        {
            return false;
        }
    }

    private class UpdateLeaseCallable implements Callable<Boolean>
    {
        private Lease lease;

        public UpdateLeaseCallable(Lease lease)
        {
            this.lease = lease;
        }

        public Boolean call()
        {
            return false;
        }
    }
}
