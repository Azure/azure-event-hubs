// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Threading.Tasks;

    class Pump
    {
        readonly EventProcessorHost host;
        ConcurrentDictionary<string, PartitionPump> pumpStates;

        public Pump(EventProcessorHost host)
        {
            this.host = host;
            this.pumpStates = new ConcurrentDictionary<string, PartitionPump>();
        }

        public async Task AddPumpAsync(string partitionId, Lease lease)
        {
            PartitionPump capturedPump;
            if (this.pumpStates.TryGetValue(partitionId, out capturedPump))
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
                    this.host.LogWithHostAndPartition(EventLevel.Informational, partitionId, "updating lease for pump");
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
            this.pumpStates.TryAdd(partitionId, newPartitionPump); // do the put after start, if the start fails then put doesn't happen
		    this.host.LogWithHostAndPartition(EventLevel.Informational, partitionId, "created new pump");
        }

        public async Task RemovePumpAsync(string partitionId, CloseReason reason)
        {
            PartitionPump capturedPump;
            if (this.pumpStates.TryRemove(partitionId, out capturedPump))
            {
                this.host.LogWithHostAndPartition(EventLevel.Informational, partitionId, "closing pump for reason " + reason);
                if (!capturedPump.IsClosing)
                {
                    await capturedPump.CloseAsync(reason);
                }
                // else, pump is already closing/closed, don't need to try to shut it down again

                this.host.LogWithHostAndPartition(EventLevel.Informational, partitionId, "removing pump");
            }
            else
            {
                // PartitionManager main loop tries to remove pump for every partition that the host does not own, just to be sure.
                // Not finding a pump for a partition is normal and expected most of the time.
                this.host.LogWithHostAndPartition(EventLevel.Informational, partitionId, "no pump found to remove for partition " + partitionId);
            }
        }

        public Task RemoveAllPumpsAsync(CloseReason reason)
        {
            List<Task> tasks = new List<Task>();
            var keys = new List<string>(this.pumpStates.Keys);
            foreach (string partitionId in keys)
            {
                tasks.Add(RemovePumpAsync(partitionId, reason));
            }

            return Task.WhenAll(tasks);
        }
    }
}