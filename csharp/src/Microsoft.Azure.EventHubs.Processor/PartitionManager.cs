// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    class PartitionManager
    {
        readonly EventProcessorHost host;
        readonly CancellationTokenSource cancellationTokenSource;
        readonly ConcurrentDictionary<string, PartitionPump> partitionPumps;
        IList<string> partitionIds;
        Task runTask;

        internal PartitionManager(EventProcessorHost host)
        {
            this.host = host;
            this.cancellationTokenSource = new CancellationTokenSource();
            this.partitionPumps = new ConcurrentDictionary<string, PartitionPump>();
        }

        public async Task<IEnumerable<string>> GetPartitionIdsAsync()
        {
            if (this.partitionIds == null)
            {
                EventHubClient eventHubClient = null;
                try
                {
                    eventHubClient = EventHubClient.Create(this.host.ConnectionSettings);
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

                ProcessorEventSource.Log.EventProcessorHostInfo(this.host.Id, $"PartitionCount: {this.partitionIds.Count}");
            }

            return this.partitionIds;
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
            }
            catch (Exception e)
            {
                ProcessorEventSource.Log.EventProcessorHostError(this.host.Id, "Exception from partition manager main loop, shutting down", e.ToString());
                this.host.EventProcessorOptions.NotifyOfException(this.host.HostName, "N/A", e, EventProcessorHostActionStrings.PartitionManagerMainLoop);
            }

            try
            {
                // Cleanup
                ProcessorEventSource.Log.EventProcessorHostInfo(this.host.Id, "Shutting down all pumps");
                await this.RemoveAllPumpsAsync(CloseReason.Shutdown);
            }
            catch (Exception e)
	    	{
                ProcessorEventSource.Log.EventProcessorHostError(this.host.Id, "Failure during shutdown", e.ToString());
                this.host.EventProcessorOptions.NotifyOfException(this.host.HostName, "N/A", e, EventProcessorHostActionStrings.PartitionManagerCleanup);
            }
        }

        async Task InitializeStoresAsync() //throws InterruptedException, ExecutionException, ExceptionWithAction
        {
            // Make sure the lease store exists
            ILeaseManager leaseManager = this.host.LeaseManager;
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

            // Make sure the checkpoint store exists
            ICheckpointManager checkpointManager = this.host.CheckpointManager;
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
                        ProcessorEventSource.Log.PartitionPumpWarning(this.host.Id, partitionId, retryMessage, e.ToString());
                    }
                    else
                    {
                        ProcessorEventSource.Log.EventProcessorHostWarning(this.host.Id, retryMessage, e.ToString());
                    }
                    retryCount++;
                }
            }
            while (!createdOK && (retryCount < maxRetries));

            if (!createdOK)
            {
                if (partitionId != null)
                {
                    ProcessorEventSource.Log.PartitionPumpError(this.host.Id, partitionId, finalFailureMessage);
                }
                else
                {
                    ProcessorEventSource.Log.EventProcessorHostError(this.host.Id, finalFailureMessage, null);
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
                        allLeases[possibleLease.PartitionId] = possibleLease;
                        if (possibleLease.IsExpired())
                        {
                            ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, possibleLease.PartitionId, "Trying to acquire lease.");
                            if (await leaseManager.AcquireLeaseAsync(possibleLease))
                            {
                                ourLeasesCount++;
                            }
                            else
                            {
                                // Probably failed because another host stole it between get and acquire  
                                leasesOwnedByOthers.Add(possibleLease);
                            }
                        }
                        else if (possibleLease.Owner == this.host.HostName)
                        {
                            ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, possibleLease.PartitionId, "Trying to renew lease.");
                            if (await leaseManager.RenewLeaseAsync(possibleLease))
                            {
                                ourLeasesCount++;
                            }
                            else
                            {
                                // Probably failed because another host stole it between get and renew 
                                leasesOwnedByOthers.Add(possibleLease);
                            }
                        }
                        else
                        {
                            leasesOwnedByOthers.Add(possibleLease);
                        }
                    }
                    catch (Exception e)
                    {
                        ProcessorEventSource.Log.EventProcessorHostWarning(this.host.Id, "Failure getting/acquiring/renewing lease, skipping", e.ToString());
                        this.host.EventProcessorOptions.NotifyOfException(this.host.HostName, "N/A", e, EventProcessorHostActionStrings.CheckingLeases);
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
                                ProcessorEventSource.Log.PartitionPumpStealLeaseStart(this.host.Id, stealee.PartitionId);
                                if (await leaseManager.AcquireLeaseAsync(stealee))
                                {
                                    // Succeeded in stealing lease
                                    ProcessorEventSource.Log.PartitionPumpStealLeaseStop(this.host.Id, stealee.PartitionId);
                                    ourLeasesCount++;
                                }
                                else
                                {
                                    ProcessorEventSource.Log.EventProcessorHostWarning(this.host.Id, "Failed to steal lease for partition " + stealee.PartitionId, null);
                                }
                            }
                            catch (Exception e)
                            {
                                ProcessorEventSource.Log.EventProcessorHostError(this.host.Id, "Exception stealing lease for partition " + stealee.PartitionId, e.ToString());
                                this.host.EventProcessorOptions.NotifyOfException(this.host.HostName, stealee.PartitionId, e, EventProcessorHostActionStrings.StealingLease);
                            }
                        }
                    }
                }

                // Update pump with new state of leases.
                foreach (string partitionId in allLeases.Keys)
                {
                    Lease updatedLease = allLeases[partitionId];
                    ProcessorEventSource.Log.EventProcessorHostInfo(this.host.Id, $"Lease on partition {updatedLease.PartitionId} owned by {updatedLease.Owner}");
                    if (updatedLease.Owner == this.host.HostName)
                    {
                        await this.AddPumpAsync(partitionId, updatedLease);
                    }
                    else
                    {
                        await this.RemovePumpAsync(partitionId, CloseReason.LeaseLost);
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
                }
            }
        }

        async Task AddPumpAsync(string partitionId, Lease lease)
        {
            PartitionPump capturedPump;
            if (this.partitionPumps.TryGetValue(partitionId, out capturedPump))
            {
                // There already is a pump. Make sure the pump is working and replace the lease.
                if (capturedPump.PumpStatus == PartitionPumpStatus.Errored || capturedPump.IsClosing)
                {
                    // The existing pump is bad. Remove it and create a new one.
                    await RemovePumpAsync(partitionId, CloseReason.Shutdown);
                    await CreateNewPumpAsync(partitionId, lease);
                }
                else
                {
                    // Pump is working, just replace the lease.
                    ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, partitionId, "Updating lease for pump");
                    capturedPump.SetLease(lease);
                }
            }
            else
            {
                // No existing pump, create a new one.
                await CreateNewPumpAsync(partitionId, lease);
            }
        }

        async Task CreateNewPumpAsync(string partitionId, Lease lease)
        {
            PartitionPump newPartitionPump = new EventHubPartitionPump(this.host, lease);
            await newPartitionPump.OpenAsync();
            this.partitionPumps.TryAdd(partitionId, newPartitionPump); // do the put after start, if the start fails then put doesn't happen
            ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, partitionId, "Created new PartitionPump");
        }

        async Task RemovePumpAsync(string partitionId, CloseReason reason)
        {
            PartitionPump capturedPump;
            if (this.partitionPumps.TryRemove(partitionId, out capturedPump))
            {
                if (!capturedPump.IsClosing)
                {
                    await capturedPump.CloseAsync(reason);
                }
                // else, pump is already closing/closed, don't need to try to shut it down again
            }
            else
            {
                // PartitionManager main loop tries to remove pump for every partition that the host does not own, just to be sure.
                // Not finding a pump for a partition is normal and expected most of the time.
                ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, partitionId, "No pump found to remove for this partition");
            }
        }

        Task RemoveAllPumpsAsync(CloseReason reason)
        {
            List<Task> tasks = new List<Task>();
            var keys = new List<string>(this.partitionPumps.Keys);
            foreach (string partitionId in keys)
            {
                tasks.Add(this.RemovePumpAsync(partitionId, reason));
            }

            return Task.WhenAll(tasks);
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
                        ProcessorEventSource.Log.EventProcessorHostInfo(this.host.Id, $"Proposed to steal lease for partition {l.PartitionId} from {biggestOwner}");
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
                ProcessorEventSource.Log.EventProcessorHostInfo(this.host.Id, $"Host {owner} owns {counts[owner]} leases");
            }

            ProcessorEventSource.Log.EventProcessorHostInfo(this.host.Id, $"Total hosts in list: {counts.Count}");
            return counts;
        }
    }
}