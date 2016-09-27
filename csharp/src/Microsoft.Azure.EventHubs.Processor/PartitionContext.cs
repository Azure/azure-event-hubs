// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using System.Diagnostics.Tracing;
    using System.Threading.Tasks;

    public class PartitionContext
    {
        readonly EventProcessorHost host;

        internal PartitionContext(EventProcessorHost host, string partitionId, string eventHubPath, string consumerGroupName)
        {
            this.host = host;
            this.PartitionId = partitionId;
            this.EventHubPath = eventHubPath;
            this.ConsumerGroupName = consumerGroupName;
            this.ThisLock = new object();
            this.SequenceNumber = 0;
            this.Offset = PartitionReceiver.StartOfStream;
        }

        /// <summary>
        /// Returns the current Offset from the last checkpoint of the partition.
        /// </summary>
        public string Offset { get; private set; }

        /// <summary>
        /// Returns the current sequence number from the last checkpoint of the partition.
        /// </summary>
        public long SequenceNumber { get; private set; }

        public string ConsumerGroupName { get; }

        public string EventHubPath { get; }

        public string PartitionId { get; }

        // Unlike other properties which are immutable after creation, the lease is updated dynamically and needs a setter.
        internal Lease Lease { get; set; }

        object ThisLock { get; }

        /// <summary>
        /// Updates the Offset/SequenceNumber in the PartitionContext with the values in the received EventData object.
        ///  
        /// <para>Since Offset is a string it cannot be compared easily, but SequenceNumber is checked. The new SequenceNumber must be
        /// at least the same as the current value or the entire assignment is aborted. It is assumed that if the new SequenceNumber
        /// is equal or greater, the new Offset will be as well.</para>
        /// </summary>
        /// <param name="eventData">A received EventData with valid Offset and SequenceNumber</param>
        /// <exception cref="ArgumentOutOfRangeException">If the SequenceNumber in the provided event is less than the current value</exception>
        public void SetOffsetAndSequenceNumber(EventData eventData)
        {
            if (eventData == null)
            {
                throw new ArgumentNullException(nameof(eventData));
            }

            this.SetOffsetAndSequenceNumber(eventData.SystemProperties.Offset, eventData.SystemProperties.SequenceNumber);
        }

        /// <summary>
        /// Updates the Offset/SequenceNumber in the PartitionContext.
        /// 
        /// <para>These two values are closely tied and must be updated in an atomic fashion, hence the combined setter.
        /// Since Offset is a string it cannot be compared easily, but SequenceNumber is checked. The new SequenceNumber must be
        /// at least the same as the current value or the entire assignment is aborted. It is assumed that if the new SequenceNumber
        /// is equal or greater, the new Offset will be as well.</para>
        /// </summary>
        /// <param name="Offset">New Offset value</param>
        /// <param name="SequenceNumber">New SequenceNumber value </param>
        /// <exception cref="ArgumentOutOfRangeException">If the SequenceNumber in the provided event is less than the current value</exception>
        public void SetOffsetAndSequenceNumber(string Offset, long SequenceNumber)
        {
            lock(this.ThisLock)
            {
                if (SequenceNumber >= this.SequenceNumber)
                {
                    this.Offset = Offset;
                    this.SequenceNumber = SequenceNumber;
                }
                else
                {
                    throw new ArgumentOutOfRangeException("Offset/SequenceNumber", $"New Offset {Offset}/{SequenceNumber} is less than previous {this.Offset}/{this.SequenceNumber}");
                }
            }
        }    

        internal async Task<string> GetInitialOffsetAsync() // throws InterruptedException, ExecutionException
        {
            Func<string, string> initialOffsetProvider = this.host.EventProcessorOptions.InitialOffsetProvider;
            if (initialOffsetProvider != null)
            {
                ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, this.PartitionId, "Calling user-provided initial Offset provider");
                this.Offset = initialOffsetProvider(this.PartitionId);
                this.SequenceNumber = 0; // TODO we use SequenceNumber to check for regression of Offset, 0 could be a problem until it gets updated from an event
                ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, this.PartitionId, $"Initial Offset/SequenceNumber provided: {this.Offset}/{this.SequenceNumber}");
            }
            else
            {
                Checkpoint startingCheckpoint = await this.host.CheckpointManager.GetCheckpointAsync(this.PartitionId);

                this.Offset = startingCheckpoint.Offset;
                this.SequenceNumber = startingCheckpoint.SequenceNumber;
                ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, this.PartitionId, $"Retrieved starting Offset/SequenceNumber: {this.Offset}/{this.SequenceNumber}");
            }

            return this.Offset;
        }

        /// <summary>
        /// Writes the current Offset and SequenceNumber to the checkpoint store via the checkpoint manager.
        /// </summary>
        /// <exception cref="ArgumentOutOfRangeException">If this.SequenceNumber is less than the last checkpointed value</exception>
        public Task CheckpointAsync()
        {
            // Capture the current Offset and SequenceNumber. Synchronize to be sure we get a matched pair
            // instead of catching an update halfway through. Do the capturing here because by the time the checkpoint
            // task runs, the fields in this object may have changed, but we should only write to store what the user
            // has directed us to write.
            Checkpoint capturedCheckpoint;
            lock(this.ThisLock)
            {
                capturedCheckpoint = new Checkpoint(this.PartitionId, this.Offset, this.SequenceNumber);
            }

            return this.PersistCheckpointAsync(capturedCheckpoint);
        }

        /// <summary>
        /// Stores the Offset and SequenceNumber from the provided received EventData instance, then writes those
        /// values to the checkpoint store via the checkpoint manager.
        /// </summary>
        /// <param name="eventData">A received EventData with valid Offset and SequenceNumber</param>
        /// <exception cref="ArgumentOutOfRangeException">If the SequenceNumber is less than the last checkpointed value</exception>
        public Task CheckpointAsync(EventData eventData)
        {
            this.SetOffsetAndSequenceNumber(eventData.SystemProperties.Offset, eventData.SystemProperties.SequenceNumber);
            return this.PersistCheckpointAsync(new Checkpoint(this.PartitionId, eventData.SystemProperties.Offset, eventData.SystemProperties.SequenceNumber));
        }

        public override string ToString()
        {
            return $"PartitionContext({this.EventHubPath}/{this.ConsumerGroupName}/{this.PartitionId}/{this.SequenceNumber})";
        }

        async Task PersistCheckpointAsync(Checkpoint checkpoint) // throws ArgumentOutOfRangeException, InterruptedException, ExecutionException
        {
            ProcessorEventSource.Log.PartitionPumpCheckpointStart(this.host.Id, checkpoint.PartitionId, checkpoint.Offset, checkpoint.SequenceNumber);
            try
            {
                Checkpoint inStoreCheckpoint = await this.host.CheckpointManager.GetCheckpointAsync(checkpoint.PartitionId);
                if (checkpoint.SequenceNumber >= inStoreCheckpoint.SequenceNumber)
                {
                    inStoreCheckpoint.Offset = checkpoint.Offset;
                    inStoreCheckpoint.SequenceNumber = checkpoint.SequenceNumber;
                    await this.host.CheckpointManager.UpdateCheckpointAsync(inStoreCheckpoint);
                }
                else
                {
                    string msg = $"Ignoring out of date checkpoint {checkpoint.Offset}/{checkpoint.SequenceNumber}" +
                            $" because store is at {inStoreCheckpoint.Offset}/{inStoreCheckpoint.SequenceNumber}";
                    ProcessorEventSource.Log.PartitionPumpError(this.host.Id, checkpoint.PartitionId, msg);
                    throw new ArgumentOutOfRangeException("Offset/SequenceNumber", msg);
                }
            }
            catch (Exception e)
            {
                ProcessorEventSource.Log.PartitionPumpCheckpointError(this.host.Id, checkpoint.PartitionId, e.ToString());
                throw;
            }
            finally
            {
                ProcessorEventSource.Log.PartitionPumpCheckpointStop(this.host.Id, checkpoint.PartitionId);
            }
        }
    }
}