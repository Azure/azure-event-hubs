/*
 * LICENSE GOES HERE TOO
 */

package com.microsoft.azure.eventprocessorhost;

import java.io.IOException;
import java.net.URISyntaxException;
import java.security.InvalidKeyException;
import java.util.ArrayList;
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
    // private CloudBlockBlob eventHubInfoBlob;  // TODO not used?
    
    private Gson gson;
    
    private final static int storageMaximumExecutionTimeInMs = 2 * 60 * 1000; // two minutes
    private final static int leaseIntervalInSeconds = 30;
    private final static int leaseRenewIntervalInMilliseconds = 10 * 1000; // ten seconds
    // private final static String eventHubInfoBlobName = "eventhub.info";
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
        
        //this.eventHubInfoBlob = this.eventHubContainer.getBlockBlobReference(AzureStorageCheckpointLeaseManager.eventHubInfoBlobName);
        
        this.gson = new Gson();

        // The only option that .NET sets on renewRequestOptions is ServerTimeout, which doesn't exist in Java equivalent
    }

    @Override
    public Future<Boolean> checkpointStoreExists()
    {
        return leaseStoreExists();
    }

    @Override
    public Future<Boolean> createCheckpointStoreIfNotExists()
    {
        return createLeaseStoreIfNotExists();
    }

    @Override
    public Future<CheckPoint> getCheckpoint(String partitionId)
    {
    	
        return EventProcessorHost.getExecutorService().submit(() -> getCheckpointSync(partitionId));
    }
    
    private AzureBlobCheckPoint getCheckpointSync(String partitionId) throws URISyntaxException, IOException, StorageException
    {
    	AzureBlobLease lease = (AzureBlobLease)getLeaseSync(partitionId);
    	AzureBlobCheckPoint checkpoint = new AzureBlobCheckPoint(lease);
    	checkpoint.setOffset(lease.getOffset());
    	checkpoint.setSequenceNumber(lease.getSequenceNumber());
    	return checkpoint;
    }

    @Override
    public Future<Void> updateCheckpoint(CheckPoint checkpoint)
    {
    	return updateCheckpoint(checkpoint, checkpoint.getOffset(), checkpoint.getSequenceNumber());
    }
    
    @Override
    public Future<Void> updateCheckpoint(CheckPoint checkpoint, String offset, long sequenceNumber)
    {
        return EventProcessorHost.getExecutorService().submit(() -> updateCheckpointSync((AzureBlobCheckPoint)checkpoint, offset, sequenceNumber));
    }
    
    private Void updateCheckpointSync(AzureBlobCheckPoint checkpoint, String offset, long sequenceNumber) throws Exception
    {
    	AzureBlobLease lease = checkpoint.getLease();
    	lease.setOffset(offset);
    	lease.setSequenceNumber(sequenceNumber);
    	updateLeaseSync(lease);
    	return null;
    }

    @Override
    public Future<Void> deleteCheckpoint(String partitionId)
    {
    	// Make this a no-op to avoid deleting leases by accident.
        return null;
    }


    @Override
    public int getLeaseRenewIntervalInMilliseconds()
    {
    	return AzureStorageCheckpointLeaseManager.leaseRenewIntervalInMilliseconds;
    }
    
    @Override
    public Future<Boolean> leaseStoreExists()
    {
        return EventProcessorHost.getExecutorService().submit(() -> this.eventHubContainer.exists());
    }

    @Override
    public Future<Boolean> createLeaseStoreIfNotExists()
    {
        return EventProcessorHost.getExecutorService().submit(() -> this.eventHubContainer.createIfNotExists());
    }

    @Override
    public Future<Lease> getLease(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> getLeaseSync(partitionId));
    }
    
    private AzureBlobLease getLeaseSync(String partitionId) throws URISyntaxException, IOException, StorageException
    {
    	AzureBlobLease retval = null;
    	
		CloudBlockBlob leaseBlob = this.consumerGroupDirectory.getBlockBlobReference(partitionId);
		if (leaseBlob.exists())
		{
			retval = downloadLease(leaseBlob);
		}

    	return retval;
    }

    @Override
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

    @Override
    public Future<Lease> createLeaseIfNotExists(String partitionId)
    {
        return EventProcessorHost.getExecutorService().submit(() -> createLeaseIfNotExistsSync(partitionId));
    }
    
    private AzureBlobLease createLeaseIfNotExistsSync(String partitionId) throws URISyntaxException, IOException, StorageException
    {
    	AzureBlobLease returnLease = null;
    	try
    	{
    		CloudBlockBlob leaseBlob = this.consumerGroupDirectory.getBlockBlobReference(partitionId);
    		Lease lease = new Lease(this.host.getEventHubPath(), this.host.getConsumerGroupName(), partitionId);
    		String jsonLease = this.gson.toJson(lease);
    		this.host.logWithHostAndPartition(partitionId,
    				"CreateLeaseIfNotExist - leaseContainerName: " + this.host.getEventHubPath() + " consumerGroupName: " + this.host.getConsumerGroupName());
    		leaseBlob.uploadText(jsonLease, null, AccessCondition.generateIfNoneMatchCondition("*"), null, null);
    		returnLease = new AzureBlobLease(lease, leaseBlob);
    	}
    	catch (StorageException se)
    	{
    		StorageExtendedErrorInformation extendedErrorInfo = se.getExtendedErrorInformation();
    		if ((extendedErrorInfo != null) && (extendedErrorInfo.getErrorCode().compareTo(StorageErrorCodeStrings.BLOB_ALREADY_EXISTS) != 0))
    		{
    			this.host.logWithHostAndPartition(partitionId,
    				"CreateLeaseIfNotExist StorageException - leaseContainerName: " + this.host.getEventHubPath() + " consumerGroupName: " + this.host.getConsumerGroupName(),
    				se);
    		}
    		else
    		{
    			// The blob already exists.
        		returnLease = getLeaseSync(partitionId);
    		}
    	}
    	
    	return returnLease;
    }

    @Override
    public Future<Void> deleteLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> deleteLeaseSync((AzureBlobLease)lease));
    }
    
    private Void deleteLeaseSync(AzureBlobLease lease) throws StorageException
    {
    	lease.getBlob().deleteIfExists();
    	return null;
    }

    @Override
    public Future<Boolean> acquireLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> acquireLeaseSync((AzureBlobLease)lease));
    }
    
    private Boolean acquireLeaseSync(AzureBlobLease lease) throws Exception
    {
    	CloudBlockBlob leaseBlob = lease.getBlob();
    	boolean retval = true;
    	String newLeaseId = UUID.randomUUID().toString();
    	try
    	{
    		String newToken = null;
    		leaseBlob.downloadAttributes();
	    	if (leaseBlob.getProperties().getLeaseState() == LeaseState.LEASED)
	    	{
	    		//this.host.logWithHostAndPartition(lease.getPartitionId(), "changeLease");
	    		newToken = leaseBlob.changeLease(newLeaseId, AccessCondition.generateLeaseCondition(lease.getToken()));
	    		//this.host.logWithHostAndPartition(lease.getPartitionId(), "changeLease OK");
	    	}
	    	else
	    	{
	    		//this.host.logWithHostAndPartition(lease.getPartitionId(), "acquireLease");
	    		newToken = leaseBlob.acquireLease(AzureStorageCheckpointLeaseManager.leaseIntervalInSeconds, newLeaseId);
	    		//this.host.logWithHostAndPartition(lease.getPartitionId(), "acquireLease OK");
	    	}
	    	lease.setToken(newToken);
	    	lease.setOwner(this.host.getHostName());
	    	// Increment epoch each time lease is acquired or stolen by a new host
	    	lease.incrementEpoch();
    		//this.host.logWithHostAndPartition(lease.getPartitionId(), "uploadText");
	    	leaseBlob.uploadText(this.gson.toJson(lease), null, AccessCondition.generateLeaseCondition(lease.getToken()), null, null);
    		//this.host.logWithHostAndPartition(lease.getPartitionId(), "uploadText OK");
    	}
    	catch (StorageException se)
    	{
    		if (wasLeaseLost(se))
    		{
    			retval = false;
    		}
    		else
    		{
    			throw se;
    		}
    	}
    	
    	return retval;
    }

    @Override
    public Future<Boolean> renewLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> renewLeaseSync((AzureBlobLease)lease));
    }
    
    private Boolean renewLeaseSync(AzureBlobLease lease) throws Exception
    {
    	CloudBlockBlob leaseBlob = lease.getBlob();
    	boolean retval = true;
    	
    	try
    	{
    		leaseBlob.renewLease(AccessCondition.generateLeaseCondition(lease.getToken()), this.renewRequestOptions, null);
    	}
    	catch (StorageException se)
    	{
    		if (wasLeaseLost(se))
    		{
    			retval = false;
    		}
    		else
    		{
    			throw se;
    		}
    	}
    	
    	return retval;
    }

    @Override
    public Future<Boolean> releaseLease(Lease lease)
    {
        return EventProcessorHost.getExecutorService().submit(() -> releaseLeaseSync((AzureBlobLease)lease));
    }
    
    private Boolean releaseLeaseSync(AzureBlobLease lease) throws Exception
    {
    	CloudBlockBlob leaseBlob = lease.getBlob();
    	boolean retval = true;
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
    		if (wasLeaseLost(se))
    		{
    			retval = false;
    		}
    		else
    		{
    			throw se;
    		}
    	}
    	
    	return retval;
    }

    @Override
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
    	if (!renewLeaseSync(lease))
    	{
    		return false;
    	}
    	
    	CloudBlockBlob leaseBlob = lease.getBlob();
    	try
    	{
    		leaseBlob.uploadText(this.gson.toJson(lease), null, AccessCondition.generateLeaseCondition(token), null, null);
    	}
    	catch (StorageException se)
    	{
    		if (wasLeaseLost(se))
    		{
    			throw new LeaseLostException(lease, se);
    		}
    		else
    		{
    			throw se;
    		}
    	}
    	
    	return true;
    }

    private AzureBlobLease downloadLease(CloudBlockBlob blob) throws StorageException, IOException
    {
    	String jsonLease = blob.downloadText();
    	AzureBlobLease blobLease = new AzureBlobLease(this.gson.fromJson(jsonLease, Lease.class), blob);
    	return blobLease;
    }
    
    private boolean wasLeaseLost(StorageException se)
    {
    	boolean retval = false;
		//this.host.logWithHost("WAS LEASE LOST?");
		//this.host.logWithHost("Http " + se.getHttpStatusCode());
    	if ((se.getHttpStatusCode() == 409) || // conflict
    		(se.getHttpStatusCode() == 412)) // precondition failed
    	{
    		StorageExtendedErrorInformation extendedErrorInfo = se.getExtendedErrorInformation();
    		if (extendedErrorInfo != null)
    		{
    			String errorCode = extendedErrorInfo.getErrorCode();
				//this.host.logWithHost("Error code: " + errorCode);
				//this.host.logWithHost("Error message: " + extendedErrorInfo.getErrorMessage());
    			if ((errorCode.compareTo(StorageErrorCodeStrings.LEASE_LOST) == 0) ||
    				(errorCode.compareTo(StorageErrorCodeStrings.LEASE_ID_MISMATCH_WITH_LEASE_OPERATION) == 0) ||
    				(errorCode.compareTo(StorageErrorCodeStrings.LEASE_ID_MISMATCH_WITH_BLOB_OPERATION) == 0))
    			{
    				retval = true;
    			}
    		}
    	}
    	return retval;
    }
}
