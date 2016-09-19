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
        string offset = PartitionReceiver.StartOfStream;
        long sequenceNumber = 0;

        internal PartitionContext(EventProcessorHost host, string partitionId, string eventHubPath, string consumerGroupName)
        {
            this.host = host;
            this.PartitionId = partitionId;
            this.EventHubPath = eventHubPath;
            this.ConsumerGroupName = consumerGroupName;
            this.ThisLock = new object();
        }

        public string ConsumerGroupName { get; }

        public string EventHubPath { get; }

        public string PartitionId { get; }

        public string Owner
        {
            get
            {
                return this.Lease.Owner;
            }
        }

        // Unlike other properties which are immutable after creation, the lease is updated dynamically and needs a setter.
        internal Lease Lease { get; set; }

        object ThisLock { get; }

        /// <summary>
        /// Updates the offset/sequenceNumber in the PartitionContext with the values in the received EventData object.
        ///  
        /// <para>Since offset is a string it cannot be compared easily, but sequenceNumber is checked. The new sequenceNumber must be
        /// at least the same as the current value or the entire assignment is aborted. It is assumed that if the new sequenceNumber
        /// is equal or greater, the new offset will be as well.</para>
        /// </summary>
        /// <param name="eventData">A received EventData with valid offset and sequenceNumber</param>
        /// <exception cref="ArgumentOutOfRangeException">If the sequenceNumber in the provided event is less than the current value</exception>
        public void SetOffsetAndSequenceNumber(EventData eventData)
        {
            if (eventData == null)
            {
                throw new ArgumentNullException(nameof(eventData));
            }

            this.SetOffsetAndSequenceNumber(eventData.SystemProperties.Offset, eventData.SystemProperties.SequenceNumber);
        }

        /// <summary>
        /// Updates the offset/sequenceNumber in the PartitionContext.
        /// 
        /// <para>These two values are closely tied and must be updated in an atomic fashion, hence the combined setter.
        /// Since offset is a string it cannot be compared easily, but sequenceNumber is checked. The new sequenceNumber must be
        /// at least the same as the current value or the entire assignment is aborted. It is assumed that if the new sequenceNumber
        /// is equal or greater, the new offset will be as well.</para>
        /// </summary>
        /// <param name="offset">New offset value</param>
        /// <param name="sequenceNumber">New sequenceNumber value </param>
        /// <exception cref="ArgumentOutOfRangeException">If the sequenceNumber in the provided event is less than the current value</exception>
        public void SetOffsetAndSequenceNumber(string offset, long sequenceNumber)
        {
            lock(this.ThisLock)
            {
                if (sequenceNumber >= this.sequenceNumber)
                {
                    this.offset = offset;
                    this.sequenceNumber = sequenceNumber;
                }
                else
                {
                    throw new ArgumentOutOfRangeException("offset/sequenceNumber", $"New offset {offset}/{sequenceNumber} is less than previous {this.offset}/{this.sequenceNumber}");
                }
            }
        }    

        internal async Task<object> GetInitialOffsetAsync() // throws InterruptedException, ExecutionException
        {
            Checkpoint startingCheckpoint = await this.host.CheckpointManager.GetCheckpointAsync(this.PartitionId);
            Object startAt = null;

            if (startingCheckpoint == null)
            {
                // No checkpoint was ever stored. Use the initialOffsetProvider instead.
                Func<string, object> initialOffsetProvider = this.host.EventProcessorOptions.InitialOffsetProvider;
                ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, this.PartitionId, "Calling user-provided initial offset provider");
                startAt = initialOffsetProvider(this.PartitionId);

                if (startAt is string)
                {
                    this.offset = (string)startAt;
                    this.sequenceNumber = 0; // TODO we use sequenceNumber to check for regression of offset, 0 could be a problem until it gets updated from an event
                    ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, this.PartitionId, $"Initial offset/sequenceNumber provided: {this.offset}/{this.sequenceNumber}");
                }
                else if (startAt is DateTime)
                {
                    // can't set offset/sequenceNumber
                    ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, this.PartitionId, $"Initial timestamp provided: {(DateTime)startAt}");
                }
                else
                {
                    throw new ArgumentException("Unexpected object type returned by user-provided initialOffsetProvider");
                }
    	    }
    	    else
    	    {
                this.offset = startingCheckpoint.Offset;
	    	    this.sequenceNumber = startingCheckpoint.SequenceNumber;
                ProcessorEventSource.Log.PartitionPumpInfo(this.host.Id, this.PartitionId, $"Retrieved starting offset/sequenceNumber: {this.offset}/{this.sequenceNumber}");
                startAt = this.offset;
            }

    	    return startAt;
        }

        /// <summary>
        /// Writes the current offset and sequenceNumber to the checkpoint store via the checkpoint manager.
        /// </summary>
        /// <exception cref="ArgumentOutOfRangeException">If this.sequenceNumber is less than the last checkpointed value</exception>
        public Task CheckpointAsync()
        {
    	    // Capture the current offset and sequenceNumber. Synchronize to be sure we get a matched pair
    	    // instead of catching an update halfway through. Do the capturing here because by the time the checkpoint
    	    // task runs, the fields in this object may have changed, but we should only write to store what the user
    	    // has directed us to write.
    	    Checkpoint capturedCheckpoint;
            lock(this.ThisLock)
            {
                capturedCheckpoint = new Checkpoint(this.PartitionId, this.offset, this.sequenceNumber);
            }

            return this.PersistCheckpointAsync(capturedCheckpoint);
        }

        /// <summary>
        /// Stores the offset and sequenceNumber from the provided received EventData instance, then writes those
        /// values to the checkpoint store via the checkpoint manager.
        /// </summary>
        /// <param name="eventData">A received EventData with valid offset and sequenceNumber</param>
        /// <exception cref="ArgumentOutOfRangeException">If the sequenceNumber is less than the last checkpointed value</exception>
        public Task CheckpointAsync(EventData eventData)
        {
            this.SetOffsetAndSequenceNumber(eventData.SystemProperties.Offset, eventData.SystemProperties.SequenceNumber);
            return this.PersistCheckpointAsync(new Checkpoint(this.PartitionId, eventData.SystemProperties.Offset, eventData.SystemProperties.SequenceNumber));
        }

        public override string ToString()
        {
            return $"PartitionContext({this.EventHubPath}/{this.ConsumerGroupName}/{this.PartitionId}/{this.sequenceNumber})";
        }

        async Task PersistCheckpointAsync(Checkpoint checkpoint) // throws ArgumentOutOfRangeException, InterruptedException, ExecutionException
        {
            ProcessorEventSource.Log.PartitionPumpCheckpointStart(this.host.Id, checkpoint.PartitionId, checkpoint.Offset, checkpoint.SequenceNumber);
            try
            {
                Checkpoint inStoreCheckpoint = await this.host.CheckpointManager.GetCheckpointAsync(checkpoint.PartitionId);
                if (inStoreCheckpoint == null || checkpoint.SequenceNumber >= inStoreCheckpoint.SequenceNumber)
                {
                    if (inStoreCheckpoint == null)
                    {
                        inStoreCheckpoint = await this.host.CheckpointManager.CreateCheckpointIfNotExistsAsync(checkpoint.PartitionId);
                    }

                    inStoreCheckpoint.Offset = checkpoint.Offset;
                    inStoreCheckpoint.SequenceNumber = checkpoint.SequenceNumber;
                    await this.host.CheckpointManager.UpdateCheckpointAsync(inStoreCheckpoint);
                }
                else
                {
                    string msg = $"Ignoring out of date checkpoint with offset {checkpoint.Offset}/sequence number {checkpoint.SequenceNumber}" +
                            $" because current persisted checkpoint has higher offset {inStoreCheckpoint.Offset}/sequence number {inStoreCheckpoint.SequenceNumber}";
                    ProcessorEventSource.Log.PartitionPumpError(this.host.Id, checkpoint.PartitionId, msg);
                    throw new ArgumentOutOfRangeException("offset/sequenceNumber", msg);
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