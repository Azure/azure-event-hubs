// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;

    /// <summary>
    /// This is a logical representation of receiving from a EventHub partition.
    /// <para>
    /// A PartitionReceiver is tied to a ConsumerGroup + Partition combination. If you are creating an epoch based
    /// PartitionReceiver (i.e. PartitionReceiver.Epoch != 0) you cannot have more than one active receiver per
    /// ConsumerGroup + Partition combo. You can have multiple receivers per ConsumerGroup + Partition combination with 
    /// non-epoch receivers.
    /// </para>
    /// </summary>
    /// <seealso cref="EventHubClient.CreateReceiver"/>
    /// <seealso cref="EventHubClient.CreateEpochReceiver"/>
    public sealed class PartitionReceiver : ClientEntity
    {
        const int MINIMUM_PREFETCH_COUNT = 10;
        const int MAXIMUM_PREFETCH_COUNT = 999;

        const int DEFAULT_PREFETCH_COUNT = 300;
        const long NULL_EPOCH = 0;

        /// <summary>
        /// This is a constant defined to represent the start of a partition stream in EventHub.
        /// </summary>
        public static readonly string StartOfStream = "-1";

        private PartitionReceiver(
            EventHubClient eventHubClient,
            string consumerGroupName,
            string partitionId,
            string startingOffset,
            bool offsetInclusive,
            DateTime dateTime,
            long epoch,
            bool isEpochReceiver)
            : base(null)
        {
        }

        internal static PartitionReceiver Create(
            EventHubClient eventHubClient,
            string consumerGroupName,
            string partitionId,
            string startingOffset,
            bool offsetInclusive,
            DateTime? dateTime,
            long epoch,
            bool isEpochReceiver)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Get the EventHub partition identifier.
        /// </summary>
        /// <value>The identifier representing the partition from which this receiver is fetching data</value>
        public string PartitionId
        {
            get; private set;
        }

        /// <summary>
        /// Get Prefetch Count configured on the Receiver.
        /// </summary>
        /// <value>The upper limit of events this receiver will actively receive regardless of whether a receive operation is pending.</value>
        public int PrefetchCount
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        /// <summary>
        /// Get the epoch value that this receiver is currently using for partition ownership.
        /// <para>A value of 0 means this receiver is not an epoch-based receiver.</para>
        /// </summary>
        /// <value>the epoch value that this receiver is currently using for partition ownership.</value>
        public long Epoch
        {
            get; private set;
        }

        /// <summary>
        /// Receive a batch of <see cref="EventData"/>'s from an EventHub partition
        /// </summary>
        /// <example>
        /// Sample code:
        /// <code>
        /// EventHubClient client = EventHubClient.Create("__connectionString__");
        /// PartitionReceiver receiver = client.CreateReceiver("ConsumerGroup1", "1");
        /// IEnumerable&lt;EventData&gt; receivedEvents = await receiver.ReceiveAsync();
        ///      
        /// while (true)
        /// {
        ///     int batchSize = 0;
        ///     if (receivedEvents != null)
        ///     {
        ///         foreach (EventData receivedEvent in receivedEvents)
        ///         {
        ///             Console.WriteLine("Message Payload: {0}", Encoding.UTF8.GetString(receivedEvent.Body));
        ///             Console.WriteLine("Offset: {0}, SeqNo: {1}, EnqueueTime: {2}", 
        ///                 receivedEvent.SystemProperties.Offset, 
        ///                 receivedEvent.SystemProperties.SequenceNumber, 
        ///                 receivedEvent.SystemProperties.EnqueuedTime);
        ///             batchSize++;
        ///         }
        ///     }
        ///          
        ///     Console.WriteLine("ReceivedBatch Size: {0}", batchSize);
        ///     receivedEvents = await receiver.ReceiveAsync();
        /// }
        /// </code>
        /// </example>
        /// <returns>A Task that will yield a batch of <see cref="EventData"/> from the partition on which this receiver is created. Returns 'null' if no EventData is present.</returns>
        public Task<IEnumerable<EventData>> ReceiveAsync()
        {
            throw new NotImplementedException();
        }

        public void SetReceiveHandler(IPartitionReceiveHandler receiveHandler)
        {
            throw new NotImplementedException();
        }

        public override Task CloseAsync()
        {
            throw new NotImplementedException();
        }
    }
}
