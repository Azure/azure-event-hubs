/*
 * LICENSE GOES HERE
 */

package com.microsoft.azure.eventprocessorhost;

import java.io.IOException;
import java.net.URISyntaxException;
import java.security.InvalidKeyException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.UUID;
import java.util.concurrent.*;

import com.google.gson.Gson;
import com.microsoft.azure.storage.AccessCondition;
import com.microsoft.azure.storage.CloudStorageAccount;
import com.microsoft.azure.storage.StorageErrorCodeStrings;
import com.microsoft.azure.storage.StorageException;
import com.microsoft.azure.storage.StorageExtendedErrorInformation;
import com.microsoft.azure.storage.blob.BlobRequestOptions;
import com.microsoft.azure.storage.blob.CloudBlobClient;
import com.microsoft.azure.storage.blob.CloudBlobContainer;
import com.microsoft.azure.storage.blob.CloudBlobDirectory;
import com.microsoft.azure.storage.blob.CloudBlockBlob;
import com.microsoft.azure.storage.blob.LeaseState;


public class AzureStorageCheckpointLeaseManager implements ICheckpointManager, ILeaseManager
{
    private EventProcessorHost host;
    private String storageConnectionString;
    
    private CloudBlobClient storageClient;
    private CloudBlobContainer eventHubContainer;
    private CloudBlobDirectory consumerGroupDirectory;
    private CloudBlockBlob eventHubInfoBlob;
    
    private Gson gson;
    
    private final static int storageMaximumExecutionTimeInMs = 2 * 60 * 1000; // two minutes
    private final static int leaseIntervalInSeconds = 30;
    private final static String eventHubInfoBlobName = "eventhub.info";
    private final BlobRequestOptions renewRequestOptions = new BlobRequestOptions();

    public AzureStorageCheckpointLeaseManager(String storageConnectionString)
    {
        this.storageConnectionString = storageConnectionString;
    }

    // The EventProcessorHost can't pass itself to the AzureStorageCheckpointLeaseManager constructor
    // because it is still being constructed.
    public void initialize(EventProcessorHost host) throws InvalidKeyException, URISyntaxException, StorageException
    {
        this.host = host;
        
        this.storageClient = CloudStorageAccount.parse(this.storageConnectionString).createCloudBlobClient();
        BlobRequestOptions options = new BlobRequestOptions();
        options.setMaximumExecutionTimeInMs(AzureStorageCheckpointLeaseManager.storageMaximumExecutionTimeInMs);
        this.storageClient.setDefaultRequestOptions(options);
        
        this.eventHubContainer = this.storageClient.getContainerReference(this.host.getEventHubPath());
        
        this.consumerGroupDirectory = this.eventHubContainer.getDirectoryReference(this.host.getConsumerGroupName());
        
        this.eventHubInfoBlob = this.eventHubContainer.getBlockBlobReference(AzureStorageCheckpointLeaseManager.eventHubInfoBlobName);
        
        this.gson = new Gson();

        // The only option that .NET sets on renewRequestOptions is ServerTimeout, which doesn't exist in Java equivalent
    }

    public Future<Boolean> checkpointStoreExists()
    {
        return leaseStoreExists();
    }

    public Future<Boolean> createCheckpointStoreIfNotExists()
    {
        return createLeaseStoreIfNotExists();
    }

    public Future<CheckPoint> getCheckpoint(String partitionId)
    {
    	
        return EventProcessorHost.getExecutorService().submit(() -> getCheckpointSync(partitionId));
    }
    
    private CheckPoint getCheckpointSync(String partitionId) throws URISyntaxException, IOException, StorageException
    {
    	AzureBlobLease lease = (AzureBlobLease)getLeaseSync(partitionId);
    	return lease.getCheckpoint();
    }

    public Future<Void> updateCheckpoint(CheckPoint checkpoint)
    {
        return EventProcessorHost.getExecutorService().submit(() -> updateCheckpointSync((AzureBlobCheckPoint)checkpoint));
    }
    
    private Void updateCheckpointSync(AzureBlobCheckPoint checkpoint)
    {
    	// TODO
    	return null;
    }

    public Future<Void> deleteCheckpoint(String partitionId)
    {
    	// Make this a no-op to avoid deleting leases by accident.
        return null;
    }


    public Future<Boolean> leaseStoreExists()
    {
        return EventProcessorHost.getExecutorService().submit(() -> this.eventHubContainer.exists());
    }

    public Future<Boolean> createLeaseStoreIfNotExists()
    {
        return EventProcessorHost.getExecutorService().submit(() -> this.eventHubContainer.createIfNotExists());
    }

    public Future<Lease> getLease(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> getLeaseSync(partitionId));
    }
    
    private Lease getLeaseSync(String partitionId) throws URISyntaxException, IOException, StorageException
    {
    	AzureBlobLease retval = null;
    	
		CloudBlockBlob leaseBlob = this.consumerGroupDirectory.getBlockBlobReference(partitionId);
		if (leaseBlob.exists())
		{
			retval = downloadLease(leaseBlob);
		}

    	return retval;
    }

    public Iterable<Future<Lease>> getAllLeases()
    {
        ArrayList<Future<Lease>> leaseFutures = new ArrayList<Future<Lease>>();
        Iterable<String> partitionIds = this.host.getPartitionManager().getPartitionIds(); 
        for (String id : partitionIds)
        {
            leaseFutures.add(getLease(id));
        }
        return leaseFutures;
    }

    public Future<Void> createLeaseIfNotExists(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> createLeaseIfNotExistsSync(partitionId));
    }
    
    private Void createLeaseIfNotExistsSync(String partitionId) throws URISyntaxException, IOException
    {
    	try
    	{
    		CloudBlockBlob leaseBlob = this.consumerGroupDirectory.getBlockBlobReference(partitionId);
    		Lease lease = new Lease(this.host.getEventHubPath(), this.host.getConsumerGroupName(), partitionId);
    		String jsonLease = this.gson.toJson(lease);
    		this.host.logWithHostAndPartition(partitionId,
    				"CreateLeaseIfNotExist - leaseContainerName: " + this.host.getEventHubPath() + " consumerGroupName: " + this.host.getConsumerGroupName());
    		leaseBlob.uploadText(jsonLease, null, AccessCondition.generateIfNoneMatchCondition("*"), null, null);
    	}
    	catch (StorageException se)
    	{
    		// From .NET: 
    		// Eat any storage exception related to conflict.
    		// This means the blob already exists.
    		this.host.logWithHostAndPartition(partitionId,
    				"CreateLeaseIfNotExist StorageException - leaseContainerName: " + this.host.getEventHubPath() + " consumerGroupName: " + this.host.getConsumerGroupName(),
    				se);
    	}
    	
    	return null;
    }

    public Future<Void> deleteLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> deleteLeaseSync((AzureBlobLease)lease));
    }
    
    private Void deleteLeaseSync(AzureBlobLease lease) throws StorageException
    {
    	lease.getBlob().deleteIfExists();
    	return null;
    }

    public Future<Boolean> acquireLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> acquireLeaseSync((AzureBlobLease)lease));
    }
    
    private Boolean acquireLeaseSync(AzureBlobLease lease) throws Exception
    {
    	CloudBlockBlob leaseBlob = lease.getBlob();
    	String newLeaseId = UUID.randomUUID().toString();
    	try
    	{
    		String newToken = null;
	    	if (leaseBlob.getProperties().getLeaseState() == LeaseState.LEASED)
	    	{
	    		newToken = leaseBlob.changeLease(newLeaseId, AccessCondition.generateLeaseCondition(lease.getToken()));
	    	}
	    	else
	    	{
	    		newToken = leaseBlob.acquireLease(AzureStorageCheckpointLeaseManager.leaseIntervalInSeconds, newLeaseId);
	    	}
	    	lease.setToken(newToken);
	    	lease.setOwner(this.host.getHostName());
	    	// Increment epoch each time lease is acquired or stolen by a new host
	    	lease.incrementEpoch();
	    	leaseBlob.uploadText(this.gson.toJson(lease), null, AccessCondition.generateLeaseCondition(lease.getToken()), null, null);
    	}
    	catch (StorageException se)
    	{
    		throw handleStorageException(lease, se, false);
    	}
    	
    	return true;
    }

    public Future<Boolean> renewLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> renewLeaseSync((AzureBlobLease)lease));
    }
    
    private Boolean renewLeaseSync(AzureBlobLease lease) throws Exception
    {
    	CloudBlockBlob leaseBlob = lease.getBlob();
    	
    	try
    	{
    		leaseBlob.renewLease(AccessCondition.generateLeaseCondition(lease.getToken()), this.renewRequestOptions, null);
    	}
    	catch (StorageException se)
    	{
    		throw handleStorageException(lease, se, false);
    	}
    	
    	return true;
    }

    public Future<Boolean> releaseLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> releaseLeaseSync((AzureBlobLease)lease));
    }
    
    private Boolean releaseLeaseSync(AzureBlobLease lease) throws Exception
    {
    	CloudBlockBlob leaseBlob = lease.getBlob();
    	try
    	{
    		String leaseId = lease.getToken();
    		AzureBlobLease releasedCopy = new AzureBlobLease(lease);
    		releasedCopy.setToken("");
    		releasedCopy.setOwner("");
    		leaseBlob.uploadText(this.gson.toJson(releasedCopy), null, AccessCondition.generateLeaseCondition(leaseId), null, null);
    		leaseBlob.releaseLease(AccessCondition.generateLeaseCondition(leaseId));
    	}
    	catch (StorageException se)
    	{
    		throw handleStorageException(lease, se, false);
    	}
    	
    	return true;
    }

    public Future<Boolean> updateLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> updateLeaseSync((AzureBlobLease)lease));
    }
    
    public Boolean updateLeaseSync(AzureBlobLease lease) throws Exception
    {
    	if (lease == null)
    	{
    		return false;
    	}
    	String token = lease.getToken();
    	if ((token == null) || (token.length() == 0))
    	{
    		return false;
    	}
    	
    	// First, renew the lease to make sure the update will go through.
    	renewLeaseSync(lease);
    	
    	CloudBlockBlob leaseBlob = lease.getBlob();
    	try
    	{
    		leaseBlob.uploadText(this.gson.toJson(lease), null, AccessCondition.generateLeaseCondition(token), null, null);
    	}
    	catch (StorageException se)
    	{
    		throw handleStorageException(lease, se, true);
    	}
    	
    	return true;
    }

    private AzureBlobLease downloadLease(CloudBlockBlob blob) throws StorageException, IOException
    {
    	String jsonLease = blob.downloadText();
    	AzureBlobLease blobLease = new AzureBlobLease(this.gson.fromJson(jsonLease, Lease.class), blob);
    	return blobLease;
    }
    
    private Exception handleStorageException(AzureBlobLease lease, StorageException storageException, boolean ignoreLeaseLost)
    {
    	Exception retval = storageException;
    	
    	if ((storageException.getHttpStatusCode() == 409) || // conflict
    			(storageException.getHttpStatusCode() == 412)) // precondition failed
    	{
    		// From .NET
    		// Don't throw LeaseLostException if caller chooses to ignore it.
    		// This is mainly needed in Checkpoint scenario where Checkpoint could fail due to lease expiration
    		// but later attempts to renew could still succeed.
    		// REALITY: if ignoreLeaseLost is false, any 409 or 412 will become a LeaseLostException. But if it's true, a limited
    		// subset will still become LeaseLostException.
    		StorageExtendedErrorInformation extendedErrorInfo = storageException.getExtendedErrorInformation();
    		if (!ignoreLeaseLost || (extendedErrorInfo == null) || (extendedErrorInfo.getErrorCode().compareTo(StorageErrorCodeStrings.LEASE_LOST) == 0))
    		{
    			retval = new LeaseLostException(lease, storageException);
    		}
    	}

    	return retval;
    }
}
