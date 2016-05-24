// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    class PartitionManager
    {
        readonly EventProcessorHost host;
        readonly CancellationTokenSource cancellationTokenSource;
        Pump pump;
        IList<string> partitionIds;
        Task runTask;

        internal PartitionManager(EventProcessorHost host)
        {
            this.host = host;
            this.cancellationTokenSource = new CancellationTokenSource();
        }

        public async Task<IEnumerable<string>> GetPartitionIdsAsync()
        {
            if (this.partitionIds == null)
            {
                EventHubClient eventHubClient = null;
                try
                {
                    eventHubClient = EventHubClient.Create(this.host.EventHubConnectionString);
                    var runtimeInfo = await eventHubClient.GetRuntimeInformationAsync();
                    this.partitionIds = runtimeInfo.PartitionIds.ToList();
                }
                catch (Exception e)
        	    {
                    throw new EventProcessorConfigurationException("Encountered error while fetching the list of EventHub PartitionIds", e);
                }
                finally
                {
                    if (eventHubClient != null)
                    {
                        await eventHubClient.CloseAsync();
                    }
                }

                this.host.LogInfo("PartitionCount: " + this.partitionIds.Count);
            }

            return this.partitionIds;
        }

        // Testability hook: allows a test subclass to insert dummy pump.
        Pump CreatePumpTestHook()
        {
            return new Pump(this.host);
        }

        // Testability hook: called after stores are initialized.
        protected virtual void OnInitializeComplete()
        {
        }

        // Testability hook: called at the end of the main loop after all partition checks/stealing is complete.
        protected virtual void OnPartitionCheckComplete()
        {
        }

        public async Task StartAsync()
        {
            if (this.runTask != null)
            {
                throw new InvalidOperationException("A PartitionManager cannot be started multiple times.");
            }

            this.pump = CreatePumpTestHook();

            await this.InitializeStoresAsync();
            this.OnInitializeComplete();

            this.runTask = this.RunAsync();
        }

        public async Task StopAsync()
        {
            this.cancellationTokenSource.Cancel();
            var localRunTask = this.runTask;
            if (localRunTask != null)
            {
                await localRunTask;
            }
        }

        async Task RunAsync()
        {
            try
            {
                await this.RunLoopAsync(this.cancellationTokenSource.Token);
                this.host.LogInfo("Partition manager main loop exited normally, shutting down");
            }
            catch (Exception e)
            {
                this.host.LogError("Exception from partition manager main loop, shutting down", e);
                this.host.EventProcessorOptions.NotifyOfException(this.host.HostName, e, "Partition Manager Main Loop");
            }

            try
            {
                // Cleanup
                this.host.LogInfo("Shutting down all pumps");
                await this.pump.RemoveAllPumpsAsync(CloseReason.Shutdown);
            }
            catch (Exception e)
	    	{
                this.host.LogError("Failure during shutdown", e);
                this.host.EventProcessorOptions.NotifyOfException(this.host.HostName, e, EventProcessorHostActionStrings.ParitionManagerCleanup);
            }

            this.host.LogInfo("Partition manager exiting");
        }

        async Task InitializeStoresAsync() //throws InterruptedException, ExecutionException, ExceptionWithAction
        {
            ILeaseManager leaseManager = this.host.LeaseManager;
        
            // Make sure the lease store exists
            if (!await leaseManager.LeaseStoreExistsAsync())
            {
                await RetryAsync(() => leaseManager.CreateLeaseStoreIfNotExistsAsync(), null, "Failure creating lease store for this Event Hub, retrying",
        			    "Out of retries creating lease store for this Event Hub", EventProcessorHostActionStrings.CreatingLeaseStore, 5);
            }
            // else
            //	lease store already exists, no work needed
        
            // Now make sure the leases exist
            foreach (string id in await this.GetPartitionIdsAsync())
            {
                await RetryAsync(() => leaseManager.CreateLeaseIfNotExistsAsync(id), id, "Failure creating lease for partition, retrying",
        			    "Out of retries creating lease for partition", EventProcessorHostActionStrings.CreatingLease, 5);
            }
        
            ICheckpointManager checkpointManager = this.host.CheckpointManager;
        
            // Make sure the checkpoint store exists
            if (!await checkpointManager.CheckpointStoreExistsAsync())
            {
                await RetryAsync(() => checkpointManager.CreateCheckpointStoreIfNotExistsAsync(), null, "Failure creating checkpoint store for this Event Hub, retrying",
        			    "Out of retries creating checkpoint store for this Event Hub", EventProcessorHostActionStrings.CreatingCheckpointStore, 5);
            }
            // else
            //	checkpoint store already exists, no work needed
        
            // Now make sure the checkpoints exist
            foreach (string id in await this.GetPartitionIdsAsync())
            {
                await RetryAsync(() => checkpointManager.CreateCheckpointIfNotExistsAsync(id), id, "Failure creating checkpoint for partition, retrying",
        			    "Out of retries creating checkpoint blob for partition", EventProcessorHostActionStrings.CreatingCheckpoint, 5);
            }
        }
    
        // Throws if it runs out of retries. If it returns, action succeeded.
        async Task RetryAsync(Func<Task> lambda, string partitionId, string retryMessage, string finalFailureMessage, string action, int maxRetries) // throws ExceptionWithAction
        {
            bool createdOK = false;
    	    int retryCount = 0;
    	    do
            {
                try
                {
                    await lambda();
                    createdOK = true;
                }
                catch (Exception e)
                {
                    if (partitionId != null)
                    {
                        this.host.LogPartitionWarning(partitionId, retryMessage, e);
                    }
                    else
                    {
                        this.host.LogWarning(retryMessage, e);
                    }
                    retryCount++;
                }
            }
            while (!createdOK && (retryCount < maxRetries));

            if (!createdOK)
            {
                if (partitionId != null)
                {
                    this.host.LogPartitionError(partitionId, finalFailureMessage);
                }
                else
                {
                    this.host.LogError(finalFailureMessage, null);
                }

                throw new EventProcessorRuntimeException(finalFailureMessage, action);
            }
        }

        async Task RunLoopAsync(CancellationToken cancellationToken) // throws Exception, ExceptionWithAction
        {
    	    while (!cancellationToken.IsCancellationRequested)
            {
                ILeaseManager leaseManager = this.host.LeaseManager;
                Dictionary<string, Lease> allLeases = new Dictionary<string, Lease>();

                // Inspect all leases.
                // Acquire any expired leases.
                // Renew any leases that currently belong to us.
                IEnumerable<Task<Lease>> gettingAllLeases = leaseManager.GetAllLeases();
                List<Lease> leasesOwnedByOthers = new List<Lease>();
                int ourLeasesCount = 0;
                foreach (Task<Lease> getLeastTask in gettingAllLeases)
                {
                    try
                    {
                        Lease possibleLease = await getLeastTask;
                        if (possibleLease.IsExpired())
                        {
                            if (await leaseManager.AcquireLeaseAsync(possibleLease))
                            {
                                allLeases.Add(possibleLease.PartitionId, possibleLease);
                            }
                        }
                        else if (possibleLease.Owner == this.host.HostName)
                        {
                            if (await leaseManager.RenewLeaseAsync(possibleLease))
                            {
                                allLeases.Add(possibleLease.PartitionId, possibleLease);
                                ourLeasesCount++;
                            }
                        }
                        else
                        {
                            allLeases.Add(possibleLease.PartitionId, possibleLease);
                            leasesOwnedByOthers.Add(possibleLease);
                        }
                    }
                    catch (Exception e)
                    {
                        this.host.LogWarning("Failure getting/acquiring/renewing lease, skipping", e);
                        this.host.EventProcessorOptions.NotifyOfException(this.host.HostName, e, EventProcessorHostActionStrings.CheckingLeases);
                    }
                }

                // Grab more leases if available and needed for load balancing
                if (leasesOwnedByOthers.Count > 0)
                {
                    IEnumerable<Lease> stealTheseLeases = WhichLeasesToSteal(leasesOwnedByOthers, ourLeasesCount);
                    if (stealTheseLeases != null)
                    {
                        foreach (Lease stealee in stealTheseLeases)
                        {
                            try
                            {
                                if (await leaseManager.AcquireLeaseAsync(stealee))
                                {
                                    this.host.LogPartitionInfo(stealee.PartitionId, "Stole lease");
                                    allLeases.Add(stealee.PartitionId, stealee);
                                    ourLeasesCount++;
                                }
                                else
                                {
                                    this.host.LogWarning("Failed to steal lease for partition " + stealee.PartitionId, null);
                                }
                            }
                            catch (Exception e)
                            {
                                this.host.LogError("Exception stealing lease for partition " + stealee.PartitionId, e);
                                this.host.EventProcessorOptions.NotifyOfException(this.host.HostName, e, EventProcessorHostActionStrings.StealingLease);
                            }
                        }
                    }
                }

                // Update pump with new state of leases.
                foreach (string partitionId in allLeases.Keys)
                {
                    Lease updatedLease = allLeases[partitionId];
                    this.host.LogInfo("Lease on partition " + updatedLease.PartitionId + " owned by " + updatedLease.Owner); // DEBUG
                    if (updatedLease.Owner == this.host.HostName)
                    {
                        await this.pump.AddPumpAsync(partitionId, updatedLease);
                    }
                    else
                    {
                        await this.pump.RemovePumpAsync(partitionId, CloseReason.LeaseLost);
                    }
                }

                this.OnPartitionCheckComplete();

                try
                {
                    await Task.Delay(leaseManager.LeaseRenewInterval, cancellationToken);
                }
                catch (TaskCanceledException)
                {
                    // Bail on the async work if we are canceled.
                    this.host.LogInfo("Delay was canceled");
                }
            }
        }

        IEnumerable<Lease> WhichLeasesToSteal(List<Lease> stealableLeases, int haveLeaseCount)
        {
            IDictionary<string, int> countsByOwner = CountLeasesByOwner(stealableLeases);
            string biggestOwner = FindBiggestOwner(countsByOwner);
            int biggestCount = countsByOwner[biggestOwner];
            List<Lease> stealTheseLeases = null;

            // If the number of leases is a multiple of the number of hosts, then the desired configuration is
            // that all hosts own the name number of leases, and the difference between the "biggest" owner and
            // any other is 0.
            //
            // If the number of leases is not a multiple of the number of hosts, then the most even configuration
            // possible is for some hosts to have (leases/hosts) leases and others to have ((leases/hosts) + 1).
            // For example, for 16 partitions distributed over five hosts, the distribution would be 4, 3, 3, 3, 3,
            // or any of the possible reorderings.
            //
            // In either case, if the difference between this host and the biggest owner is 2 or more, then the
            // system is not in the most evenly-distributed configuration, so steal one lease from the biggest.
            // If there is a tie for biggest, findBiggestOwner() picks whichever appears first in the list because
            // it doesn't really matter which "biggest" is trimmed down.
            //
            // Stealing one at a time prevents flapping because it reduces the difference between the biggest and
            // this host by two at a time. If the starting difference is two or greater, then the difference cannot
            // end up below 0. This host may become tied for biggest, but it cannot become larger than the host that
            // it is stealing from.

            if ((biggestCount - haveLeaseCount) >= 2)
            {
                stealTheseLeases = new List<Lease>();
                foreach (Lease l in stealableLeases)
                {
                    if (l.Owner == biggestOwner)
                    {
                        stealTheseLeases.Add(l);
                        this.host.LogInfo("Proposed to steal lease for partition " + l.PartitionId + " from " + biggestOwner);
                        break;
                    }
                }
            }
            return stealTheseLeases;
        }

        string FindBiggestOwner(IDictionary<string, int> countsByOwner)
        {
            int biggestCount = 0;
            string biggestOwner = null;
            foreach (string owner in countsByOwner.Keys)
            {
                if (countsByOwner[owner] > biggestCount)
                {
                    biggestCount = countsByOwner[owner];
                    biggestOwner = owner;
                }
            }
            return biggestOwner;
        }

        IDictionary<string, int> CountLeasesByOwner(IEnumerable<Lease> leases)
        {
            IDictionary<string, int> counts = new Dictionary<string, int>();
            foreach (Lease l in leases)
            {
                if (counts.ContainsKey(l.Owner))
                {
                    int oldCount = counts[l.Owner];
                    counts[l.Owner] =  oldCount + 1;
                }
                else
                {
                    counts[l.Owner] = 1;
                }
            }

            foreach (string owner in counts.Keys)
            {
                this.host.LogInfo($"Host {owner} owns {counts[owner]} leases");
            }

            this.host.LogInfo($"Total hosts in list: {counts.Count}");
            return counts;
        }
    }
}