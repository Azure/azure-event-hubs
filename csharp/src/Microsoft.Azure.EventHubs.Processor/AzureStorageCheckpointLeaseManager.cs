// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Threading.Tasks;
    using Newtonsoft.Json;
    using WindowsAzure.Storage;
    using WindowsAzure.Storage.Blob;
    using WindowsAzure.Storage.Blob.Protocol;

    class AzureStorageCheckpointLeaseManager : ICheckpointManager, ILeaseManager
    {
        EventProcessorHost host;
        readonly string storageConnectionString;
        string storageContainerName = null;

        CloudBlobClient storageClient;
        CloudBlobContainer eventHubContainer;
        CloudBlobDirectory consumerGroupDirectory;

        static readonly TimeSpan storageMaximumExecutionTime = TimeSpan.FromMinutes(2);
        static readonly TimeSpan leaseDuration = TimeSpan.FromSeconds(30);
        static readonly TimeSpan leaseRenewInterval = TimeSpan.FromSeconds(10);
        readonly BlobRequestOptions renewRequestOptions = new BlobRequestOptions();

        internal AzureStorageCheckpointLeaseManager(string storageConnectionString)
        {
            this.storageConnectionString = storageConnectionString;
        }

        internal AzureStorageCheckpointLeaseManager(string storageConnectionString, string storageContainerName)
        {
            this.storageConnectionString = storageConnectionString;
            this.storageContainerName = storageContainerName;
        }

        // The EventProcessorHost can't pass itself to the AzureStorageCheckpointLeaseManager constructor
        // because it is still being constructed. Do other initialization here also because it might throw and
        // hence we don't want it in the constructor.
        internal void Initialize(EventProcessorHost host) // throws InvalidKeyException, URISyntaxException, StorageException
        {
            this.host = host;
            if (this.storageContainerName == null)
            {
                this.storageContainerName = this.host.EventHubPath;
            }
        
            this.storageClient = CloudStorageAccount.Parse(this.storageConnectionString).CreateCloudBlobClient();
            BlobRequestOptions options = new BlobRequestOptions();
            options.MaximumExecutionTime = AzureStorageCheckpointLeaseManager.storageMaximumExecutionTime;
            this.storageClient.DefaultRequestOptions = options;
        
            this.eventHubContainer = this.storageClient.GetContainerReference(this.storageContainerName);
        
            this.consumerGroupDirectory = this.eventHubContainer.GetDirectoryReference(this.host.ConsumerGroupName);
        
            // The only option that .NET sets on renewRequestOptions is ServerTimeout, which doesn't exist in Java equivalent.
            // So right now renewRequestOptions is completely default, but keep it around in case we need to change something later.
        }

        //
        // In this implementation, checkpoints are data that's actually in the lease blob, so checkpoint operations
        // turn into lease operations under the covers.
        //

        public Task<bool> CheckpointStoreExistsAsync()
        {
            return LeaseStoreExistsAsync();
        }

        public Task<bool> CreateCheckpointStoreIfNotExistsAsync()
        {
            return CreateLeaseStoreIfNotExistsAsync();
        }

        public async Task<Checkpoint> GetCheckpointAsync(string partitionId)
        {
    	    AzureBlobLease lease = (AzureBlobLease)(await GetLeaseAsync(partitionId));
            Checkpoint checkpoint = new Checkpoint(partitionId);
            checkpoint.Offset = lease.Offset;
    	    checkpoint.SequenceNumber = lease.SequenceNumber;
    	    return checkpoint;
        }

        public async Task<Checkpoint> CreateCheckpointIfNotExistsAsync(string partitionId)
        {
    	    // Normally the lease will already be created, checkpoint store is initialized after lease store.
    	    AzureBlobLease lease = (AzureBlobLease)(await CreateLeaseIfNotExistsAsync(partitionId));
            Checkpoint checkpoint = new Checkpoint(partitionId);
            checkpoint.Offset = lease.Offset;
    	    checkpoint.SequenceNumber = lease.SequenceNumber;
    	    return checkpoint;
        }

        public async Task UpdateCheckpointAsync(Checkpoint checkpoint)
        {
            // Need to fetch the most current lease data so that we can update it correctly.
            AzureBlobLease lease = (AzureBlobLease)(await GetLeaseAsync(checkpoint.PartitionId));
            lease.Offset = checkpoint.Offset;
            lease.SequenceNumber = checkpoint.SequenceNumber;
            await UpdateLeaseAsync(lease);
        }

        public Task DeleteCheckpointAsync(string partitionId)
        {
            // Make this a no-op to avoid deleting leases by accident.
            return Task.CompletedTask;
        }

        //
        // Lease operations.
        //

        public TimeSpan LeaseRenewInterval
        {
            get
            {
                return AzureStorageCheckpointLeaseManager.leaseRenewInterval;
            }
        }

        public TimeSpan LeaseDuration
        {
            get
            {
                return AzureStorageCheckpointLeaseManager.leaseDuration;
            }
        }

        public Task<bool> LeaseStoreExistsAsync()
        {
            return this.eventHubContainer.ExistsAsync();
        }

        public Task<bool> CreateLeaseStoreIfNotExistsAsync()
        {
            return this.eventHubContainer.CreateIfNotExistsAsync();
        }

        public async Task<bool> DeleteLeaseStoreAsync()
        {
            bool retval = true;

            BlobContinuationToken outerContinuationToken = null;
            do
            {
                BlobResultSegment outerResultSegment = await this.eventHubContainer.ListBlobsSegmentedAsync(outerContinuationToken);
                outerContinuationToken = outerResultSegment.ContinuationToken;
                foreach (IListBlobItem blob in outerResultSegment.Results)
                {
                    if (blob is CloudBlobDirectory)
                    {
                        BlobContinuationToken innerContinuationToken = null;
                        do
                        {
                            BlobResultSegment innerResultSegment = await ((CloudBlobDirectory)blob).ListBlobsSegmentedAsync(innerContinuationToken);
                            innerContinuationToken = innerResultSegment.ContinuationToken;
                            foreach (IListBlobItem subBlob in innerResultSegment.Results)
                            {
                                try
                                {
                                    await ((CloudBlockBlob)subBlob).DeleteIfExistsAsync();
                                }
                                catch (StorageException e)
                                {
                                    this.host.LogWarning("AzureStorage: Failure while deleting lease store", e);
                                    retval = false;
                                }
                            }
                        }
                        while (innerContinuationToken != null);
                    }
    		        else if (blob is CloudBlockBlob)
                    {
                        try
                        {
                            await ((CloudBlockBlob)blob).DeleteIfExistsAsync();
                        }
                        catch (StorageException e)
                        {
                            this.host.LogWarning("AzureStorage: Failure while deleting lease store", e);
                            retval = false;
                        }
                    }
                }
            }
            while (outerContinuationToken != null);

            return retval;
        }

        public async Task<Lease> GetLeaseAsync(string partitionId) // throws URISyntaxException, IOException, StorageException
        {
    	    AzureBlobLease retval = null;

            CloudBlockBlob leaseBlob = this.consumerGroupDirectory.GetBlockBlobReference(partitionId);
		    if (await leaseBlob.ExistsAsync())
		    {
			    retval = await DownloadLeaseAsync(partitionId, leaseBlob);
		    }

    	    return retval;
        }

        public IEnumerable<Task<Lease>> GetAllLeases()
        {
            List<Task<Lease>> leaseFutures = new List<Task<Lease>>();
            IEnumerable<string> partitionIds = this.host.PartitionManager.GetPartitionIdsAsync().Result;
            foreach (string id in partitionIds)
            {
                leaseFutures.Add(GetLeaseAsync(id));
            }

            return leaseFutures;
        }

        public async Task<Lease> CreateLeaseIfNotExistsAsync(string partitionId) // throws URISyntaxException, IOException, StorageException
        {
        	AzureBlobLease returnLease = null;
    	    try
    	    {
    		    CloudBlockBlob leaseBlob = this.consumerGroupDirectory.GetBlockBlobReference(partitionId);
                returnLease = new AzureBlobLease(partitionId, leaseBlob);
                string jsonLease = JsonConvert.SerializeObject(returnLease);

                this.host.LogPartitionInfo(partitionId,
                    "AzureStorage: CreateLeaseIfNotExist - leaseContainerName: " + this.storageContainerName + " consumerGroupName: " + this.host.ConsumerGroupName);
                await leaseBlob.UploadTextAsync(jsonLease, null, AccessCondition.GenerateIfNoneMatchCondition("*"), null, null);
            }
    	    catch (StorageException se)
    	    {
    		    StorageExtendedErrorInformation extendedErrorInfo = se.RequestInformation.ExtendedErrorInformation;
    		    if (extendedErrorInfo != null &&
        		    (extendedErrorInfo.ErrorCode == BlobErrorCodeStrings.BlobAlreadyExists ||
    	    	     extendedErrorInfo.ErrorCode == BlobErrorCodeStrings.LeaseIdMissing)) // occurs when somebody else already has leased the blob
    		    {
                    // The blob already exists.
                    this.host.LogPartitionInfo(partitionId, "AzureStorage: Lease already exists");
                    returnLease = (AzureBlobLease)(await GetLeaseAsync(partitionId));
                }
    		    else
    		    {
    			    Console.WriteLine("errorCode " + extendedErrorInfo.ErrorCode);
                    Console.WriteLine("errorString " + extendedErrorInfo.ErrorMessage);

                    this.host.LogPartitionError(
                        partitionId,
                        "AzureStorage: CreateLeaseIfNotExist StorageException - leaseContainerName: " + this.storageContainerName + " consumerGroupName: " + this.host.ConsumerGroupName,
                        se);
    			    throw;
                }
    	    }
    	
    	    return returnLease;
        }

        public async Task DeleteLeaseAsync(Lease lease)
        {
            var azureBlobLease = (AzureBlobLease)lease;
            this.host.LogPartitionInfo(azureBlobLease.PartitionId, "AzureStorage: Deleting lease");
            await azureBlobLease.Blob.DeleteIfExistsAsync();
        }

        public Task<bool> AcquireLeaseAsync(Lease lease)
        {
            return AcquireLeaseCoreAsync((AzureBlobLease)lease);
        }

        async Task<bool> AcquireLeaseCoreAsync(AzureBlobLease lease)
        {
            CloudBlockBlob leaseBlob = lease.Blob;
            bool retval = true;
            string newLeaseId = Guid.NewGuid().ToString();
        	try
            {
                string newToken = null;
                await leaseBlob.FetchAttributesAsync();
                if (leaseBlob.Properties.LeaseState == LeaseState.Leased)
                {
                    this.host.LogPartitionInfo(lease.PartitionId, "AzureStorage: need to ChangeLease");
                    newToken = await leaseBlob.ChangeLeaseAsync(newLeaseId, AccessCondition.GenerateLeaseCondition(lease.Token));
                }
                else
                {
                    this.host.LogPartitionInfo(lease.PartitionId, "AzureStorage: need to AcquireLease");
                    newToken = await leaseBlob.AcquireLeaseAsync(AzureStorageCheckpointLeaseManager.leaseDuration, newLeaseId);
                }

                lease.Token = newToken;
                lease.Owner = this.host.HostName;
                lease.IncrementEpoch(); // Increment epoch each time lease is acquired or stolen by a new host
                await leaseBlob.UploadTextAsync(JsonConvert.SerializeObject(lease), null, AccessCondition.GenerateLeaseCondition(lease.Token), null, null);
            }
    	    catch (StorageException se)
            {
                if (WasLeaseLost(se))
                {
                    retval = false;
                }
                else
                {
                    throw;
                }
            }
    	
    	    return retval;
        }

        public Task<bool> RenewLeaseAsync(Lease lease)
        {
            return RenewLeaseCoreAsync((AzureBlobLease)lease);
        }

        async Task<bool> RenewLeaseCoreAsync(AzureBlobLease lease)
        {
            CloudBlockBlob leaseBlob = lease.Blob;
            bool retval = true;
    	
    	    try
            {
                await leaseBlob.RenewLeaseAsync(AccessCondition.GenerateLeaseCondition(lease.Token), this.renewRequestOptions, null);
            }
    	    catch (StorageException se)
            {
                if (WasLeaseLost(se))
                {
                    retval = false;
                }
                else
                {
                    throw;
                }
            }
    	
 	        return retval;
        }

        public Task<bool> ReleaseLeaseAsync(Lease lease)
        {
            return ReleaseLeaseCoreAsync((AzureBlobLease)lease);
        }

        async Task<bool> ReleaseLeaseCoreAsync(AzureBlobLease lease)
        {
    	    this.host.LogPartitionInfo(lease.PartitionId, "AzureStorage: Releasing lease");

            CloudBlockBlob leaseBlob = lease.Blob;
            bool retval = true;
        	try
            {
                string leaseId = lease.Token;
                AzureBlobLease releasedCopy = new AzureBlobLease(lease);
                releasedCopy.Token = string.Empty;
                releasedCopy.Owner = string.Empty;
                await leaseBlob.UploadTextAsync(JsonConvert.SerializeObject(releasedCopy), null, AccessCondition.GenerateLeaseCondition(leaseId), null, null);
                await leaseBlob.ReleaseLeaseAsync(AccessCondition.GenerateLeaseCondition(leaseId));
            }
    	    catch (StorageException se)
            {
                if (WasLeaseLost(se))
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

        public Task<bool> UpdateLeaseAsync(Lease lease)
        {
            return UpdateLeaseCoreAsync((AzureBlobLease)lease);
        }

        async Task<bool> UpdateLeaseCoreAsync(AzureBlobLease lease)
        {
    	    if (lease == null)
            {
                return false;
            }
    	
    	    this.host.LogPartitionInfo(lease.PartitionId, "AzureStorage: Updating lease");

            string token = lease.Token;
    	    if (string.IsNullOrEmpty(token))
            {
                return false;
            }
    	
        	// First, renew the lease to make sure the update will go through.
    	    if (!await this.RenewLeaseAsync(lease))
            {
                return false;
            }

            CloudBlockBlob leaseBlob = lease.Blob;
    	    try
            {
                string jsonToUpload = JsonConvert.SerializeObject(lease);
                this.host.LogPartitionInfo(lease.PartitionId, "AzureStorage: Raw JSON uploading: " + jsonToUpload);
                await leaseBlob.UploadTextAsync(jsonToUpload, null, AccessCondition.GenerateLeaseCondition(token), null, null);
            }
    	    catch (StorageException se)
            {
                if (WasLeaseLost(se))
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

        async Task<AzureBlobLease> DownloadLeaseAsync(string partitionId, CloudBlockBlob blob) // throws StorageException, IOException
        {
            string jsonLease = await blob.DownloadTextAsync();

            this.host.LogPartitionInfo(partitionId, "AzureStorage: Raw JSON downloaded: " + jsonLease);
            AzureBlobLease rehydrated = (AzureBlobLease)JsonConvert.DeserializeObject(jsonLease, typeof(AzureBlobLease));
    	    AzureBlobLease blobLease = new AzureBlobLease(rehydrated, blob);
    	    return blobLease;
        }
    
        bool WasLeaseLost(StorageException se)
        {
            bool retval = false;
            this.host.LogInfo("AzureStorage: WAS LEASE LOST?");
            this.host.LogInfo("AzureStorage: Http " + se.RequestInformation.HttpStatusCode);
            if (se.RequestInformation.HttpStatusCode == 409 || // conflict
                se.RequestInformation.HttpStatusCode == 412) // precondition failed
            {
                StorageExtendedErrorInformation extendedErrorInfo = se.RequestInformation.ExtendedErrorInformation;
                if (extendedErrorInfo != null)
                {
                    string errorCode = extendedErrorInfo.ErrorCode;
                    this.host.LogInfo("AzureStorage: Error code: " + errorCode);
                    this.host.LogInfo("AzureStorage: Error message: " + extendedErrorInfo.ErrorMessage);
                    if (errorCode == BlobErrorCodeStrings.LeaseLost ||
                        errorCode == BlobErrorCodeStrings.LeaseIdMismatchWithLeaseOperation ||
                        errorCode == BlobErrorCodeStrings.LeaseIdMismatchWithBlobOperation)
                    {
                        retval = true;
                    }
                }
            }
            return retval;
        }
    }
}