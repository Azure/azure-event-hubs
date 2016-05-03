// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;

    /// <summary>
    /// Anchor class - all EventHub client operations start here.
    /// See <see cref="EventHubClient.Create(string)"/>
    /// </summary>
    public abstract class EventHubClient : ClientEntity
    {
        EventDataSender innerSender;

        internal EventHubClient(ServiceBusConnectionSettings connectionSettings)
            : base(StringUtility.GetRandomString())
        {
            this.ConnectionSettings = connectionSettings;
            this.EventHubName = connectionSettings.EntityPath;
        }

        public string EventHubName { get; }

        public ServiceBusConnectionSettings ConnectionSettings { get; }

        protected object ThisLock { get; } = new object();

        EventDataSender InnerSender
        {
            get
            {
                if (this.innerSender == null)
                {
                    lock (this.ThisLock)
                    {
                        if (this.innerSender == null)
                        {
                            this.innerSender = this.CreateEventSender();
                        }
                    }
                }

                return this.innerSender;
            }
        }

        public static EventHubClient Create(string connectionString)
        {
            if (string.IsNullOrWhiteSpace(connectionString))
            {
                throw Fx.Exception.ArgumentNullOrWhiteSpace(nameof(connectionString));
            }

            var connectionSettings = new ServiceBusConnectionSettings(connectionString);
            return Create(connectionSettings);
        }

        public static EventHubClient Create(ServiceBusConnectionSettings connectionSettings)
        {
            if (connectionSettings == null)
            {
                throw Fx.Exception.ArgumentNull(nameof(connectionSettings));
            }
            else if (string.IsNullOrWhiteSpace(connectionSettings.EntityPath))
            {
                throw Fx.Exception.ArgumentNullOrWhiteSpace(nameof(connectionSettings.EntityPath));
            }

            EventHubsEventSource.Log.EventHubClientCreateStart(connectionSettings.Endpoint.Host, connectionSettings.EntityPath);
            try
            {
                return connectionSettings.CreateEventHubClient();
            }
            finally
            {
                EventHubsEventSource.Log.EventHubClientCreateStop();
            }
        }

        /// <summary>
        /// Send <see cref="EventData"/> to EventHub. The sent EventData will land on any arbitrarily chosen EventHubs partition.
        /// <para>There are 3 ways to send to EventHubs, each exposed as a method (along with its sendBatch overload):</para>
        /// <para>i.    <see cref="SendAsync(EventData)"/> or <see cref="SendAsync(IEnumerable{EventData})"/></para>
        /// <para>ii.   <see cref="SendAsync(EventData, string)"/> or <see cref="SendAsync(IEnumerable{EventData}, string)"/></para>
        /// <para>iii.  <see cref="PartitionSender.SendAsync(EventData)"/> or <see cref="PartitionSender.SendAsync(IEnumerable{EventData})"/></para>
        /// Use this method to send if:
        /// <para>a) the <see cref="SendAsync(EventData)"/> operation should be highly available and</para>
        /// <para>b) the data needs to be evenly distributed among all partitions; exception being, when a subset of partitions are unavailable</para>
        /// <see cref="SendAsync(EventData)"/> sends the <see cref="EventData"/> to a Service Gateway, which in-turn will forward the EventData to one of the EventHub's partitions.
        /// Here's the message forwarding algorithm:
        /// <para>i.  Forward the EventDatas to EventHub partitions, by equally distributing the data among all partitions (ex: Round-robin the EventDatas to all EventHub partitions) </para>
        /// <para>ii. If one of the EventHub partitions is unavailable for a moment, the Service Gateway will automatically detect it and forward the message to another available partition - making the send operation highly-available.</para>
        /// </summary>
        /// <param name="eventData">the <see cref="EventData"/> to be sent.</param>
        /// <returns>A Task that completes when the send operations is done.</returns>
        /// <seealso cref="SendAsync(EventData, string)"/>
        /// <seealso cref="PartitionSender.SendAsync(EventData)"/>
        public Task SendAsync(EventData eventData)
        {
            if (eventData == null)
            {
                throw Fx.Exception.ArgumentNull(nameof(eventData));
            }

            return this.InnerSender.SendAsync(new[] { eventData }, null);
        }

        /// <summary>
        /// Send a batch of <see cref="EventData"/> to EventHub. The sent EventData will land on any arbitrarily chosen EventHub partition.
        /// This is the most recommended way to send to EventHub.
        /// 
        /// <para>There are 3 ways to send to EventHubs, to understand this particular type of send refer to the overload <see cref="SendAsync(EventData)"/>, which is used to send single <see cref="EventData"/>.
        /// Use this overload if you need to send a batch of <see cref="EventData"/>.</para>
        /// 
        /// Sending a batch of <see cref="EventData"/>'s is useful in the following cases:
        /// <para>i.    Efficient send - sending a batch of <see cref="EventData"/> maximizes the overall throughput by optimally using the number of sessions created to EventHub's service.</para>
        /// <para>ii.   Send multiple <see cref="EventData"/>'s in a Transaction. To acheieve ACID properties, the Gateway Service will forward all <see cref="EventData"/>'s in the batch to a single EventHub partition.</para>
        /// </summary>
        /// <example>
        /// Sample code:
        /// <code>
        /// var client = EventHubClient.Create("__connectionString__");
        /// while (true)
        /// {
        ///     var events = new List&lt;EventData&gt;();
        ///     for (int count = 1; count &lt; 11; count++)
        ///     {
        ///         var payload = new PayloadEvent(count);
        ///         byte[] payloadBytes = Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(payload));
        ///         var sendEvent = new EventData(payloadBytes);
        ///         var applicationProperties = new Dictionary&lt;string, string&gt;();
        ///         applicationProperties["from"] = "csharpClient";
        ///         sendEvent.Properties = applicationProperties;
        ///         events.Add(sendEvent);
        ///     }
        ///         
        ///     await client.SendAsync(events);
        ///     Console.WriteLine("Sent Batch... Size: {0}", events.Count);
        /// }
        /// </code>
        /// </example>
        /// <param name="eventDatas">A batch of events to send to EventHub</param>
        /// <returns>A Task that completes when the send operations is done.</returns>
        /// <seealso cref="SendAsync(EventData, string)"/>
        /// <seealso cref="PartitionSender.SendAsync(EventData)"/>
        public Task SendAsync(IEnumerable<EventData> eventDatas)
        {
            if (eventDatas == null)
            {
                throw Fx.Exception.ArgumentNull(nameof(eventDatas));
            }

            return this.InnerSender.SendAsync(eventDatas, null);
        }

        /// <summary>
        ///  Sends an '<see cref="EventData"/> with a partitionKey to EventHub. All <see cref="EventData"/>'s with a partitionKey are guaranteed to land on the same partition.
        ///  This send pattern emphasize data correlation over general availability and latency.
        ///  <para>There are 3 ways to send to EventHubs, each exposed as a method (along with its batched overload):</para>
        ///  <para>i.   <see cref="SendAsync(EventData)"/> or <see cref="SendAsync(IEnumerable{EventData})"/></para>
        ///  <para>ii.  <see cref="SendAsync(EventData, string)"/> or <see cref="SendAsync(IEnumerable{EventData}, string)"/></para>
        ///  <para>iii. <see cref="PartitionSender.SendAsync(EventData)"/> or <see cref="PartitionSender.SendAsync(IEnumerable{EventData})"/></para>
        ///  Use this type of send if:
        ///  <para>a)  There is a need for correlation of events based on Sender instance; The sender can generate a UniqueId and set it as partitionKey - which on the received Message can be used for correlation</para>
        ///  <para>b) The client wants to take control of distribution of data across partitions.</para>
        ///  Multiple PartitionKeys could be mapped to one Partition. EventHubs service uses a proprietary Hash algorithm to map the PartitionKey to a PartitionId.
        ///  Using this type of send (Sending using a specific partitionKey) could sometimes result in partitions which are not evenly distributed. 
        /// </summary>
        /// <param name="eventData">the <see cref="EventData"/> to be sent.</param>
        /// <param name="partitionKey">the partitionKey will be hashed to determine the partitionId to send the EventData to. On the Received message this can be accessed at <see cref="EventData.SystemProperties.PartitionKey"/>.</param>
        /// <returns>A Task that completes when the send operation is done.</returns>
        /// <seealso cref="SendAsync(EventData)"/>
        /// <seealso cref="PartitionSender.SendAsync(EventData)"/>
        public Task SendAsync(EventData eventData, string partitionKey)
        {
            if (eventData == null || string.IsNullOrEmpty(partitionKey))
            {
                throw Fx.Exception.ArgumentNull(eventData == null ? nameof(eventData) : nameof(partitionKey));
            }

            return this.InnerSender.SendAsync(new[] { eventData }, partitionKey);
        }

        /// <summary>
        /// Send a 'batch of <see cref="EventData"/> with the same partitionKey' to EventHub. All <see cref="EventData"/>'s with a partitionKey are guaranteed to land on the same partition.
        /// Multiple PartitionKey's will be mapped to one Partition.
        /// <para>
        /// There are 3 ways to send to EventHubs, to understand this particular type of send refer to the overload <see cref="SendAsync(EventData, string)"/>,
        /// which is the same type of send and is used to send single <see cref="EventData"/>.
        /// </para>
        /// Sending a batch of <see cref="EventData"/>'s is useful in the following cases:
        /// <para>i.    Efficient send - sending a batch of <see cref="EventData"/> maximizes the overall throughput by optimally using the number of sessions created to EventHubs service.</para>
        /// <para>ii.   Sending multiple events in One Transaction. This is the reason why all events sent in a batch needs to have same partitionKey (so that they are sent to one partition only).</para>
        /// </summary>
        /// <param name="eventDatas">the batch of events to send to EventHub</param>
        /// <param name="partitionKey">the partitionKey will be hashed to determine the partitionId to send the EventData to. On the Received message this can be accessed at <see cref="EventData.SystemProperties.PartitionKey"/>.</param>
        /// <returns>A Task that completes when the send operation is done.</returns>
        /// <seealso cref="SendAsync(EventData)"/>
        /// <see cref="PartitionSender.SendAsync(EventData)"/>
        public Task SendAsync(IEnumerable<EventData> eventDatas, string partitionKey)
        {
            if (eventDatas == null || string.IsNullOrEmpty(partitionKey))
            {
                throw Fx.Exception.ArgumentNull(eventDatas == null ? nameof(eventDatas) : nameof(partitionKey));
            }

            return this.InnerSender.SendAsync(eventDatas, partitionKey);
        }

        /// <summary>
        /// Create a <see cref="PartitionSender"/> which can publish <see cref="EventData"/>'s directly to a specific EventHub partition (sender type iii. in the below list).
        /// <para/>
        /// There are 3 patterns/ways to send to EventHubs:
        /// <para>i.   <see cref="SendAsync(EventData)"/> or <see cref="SendAsync(IEnumerable{EventData})"/></para>
        /// <para>ii.  <see cref="SendAsync(EventData, string)"/> or <see cref="SendAsync(IEnumerable{EventData}, string)"/></para>
        /// <para>iii. <see cref="PartitionSender.SendAsync(EventData)"/> or <see cref="PartitionSender.SendAsync(IEnumerable{EventData})"/></para>
        /// </summary>
        /// <param name="partitionId">partitionId of EventHub to send the <see cref="EventData"/>'s to.</param>
        /// <returns>The created PartitionSender</returns>
        /// <seealso cref="PartitionSender"/>
        public PartitionSender CreatePartitionSender(string partitionId)
        {
            if (string.IsNullOrEmpty(partitionId))
            {
                throw Fx.Exception.ArgumentNull(nameof(partitionId));
            }

            return new PartitionSender(this, partitionId);
        }

        /// <summary>
        /// Create a receiver for a specific EventHub partition from the specific consumer group.
        /// <para/>
        /// NOTE: There can be a maximum number of receivers that can run in parallel per ConsumerGroup per Partition. 
        /// The limit is enforced by the Event Hub service - current limit is 5 receivers in parallel. Having multiple receivers 
        /// reading from offsets that are far apart on the same consumer group / partition combo will have significant performance Impact. 
        /// </summary>
        /// <param name="consumerGroupName">the consumer group name that this receiver should be grouped under.</param>
        /// <param name="partitionId">the partition Id that the receiver belongs to. All data received will be from this partition only.</param>
        /// <param name="startingOffset">the offset to start receiving the events from. To receive from start of the stream use <see cref="PartitionReceiver.StartOfStream"/></param>
        /// <returns>The created PartitionReceiver</returns>
        /// <seealso cref="PartitionReceiver"/>
        public PartitionReceiver CreateReceiver(string consumerGroupName, string partitionId, string startingOffset)
        {
            return this.CreateReceiver(consumerGroupName, partitionId, startingOffset, false);
        }

        /// <summary>
        /// Create the EventHub receiver with given partition id and start receiving from the specified starting offset.
        /// The receiver is created for a specific EventHub Partition from the specific consumer group.
        /// </summary>
        /// <param name="consumerGroupName">the consumer group name that this receiver should be grouped under.</param>
        /// <param name="partitionId">the partition Id that the receiver belongs to. All data received will be from this partition only.</param>
        /// <param name="startOffset">the offset to start receiving the events from. To receive from start of the stream use: <see cref="PartitionReceiver.StartOfStream"/></param>
        /// <param name="offsetInclusive">if set to true, the startingOffset is treated as an inclusive offset - meaning the first event returned is the
        /// one that has the starting offset. Normally first event returned is the event after the starting offset.</param>
        /// <returns>The created PartitionReceiver</returns>
        /// <seealso cref="PartitionReceiver"/>
        public PartitionReceiver CreateReceiver(string consumerGroupName, string partitionId, string startOffset, bool offsetInclusive)
        {
            if (string.IsNullOrWhiteSpace(consumerGroupName) || string.IsNullOrWhiteSpace(partitionId))
            {
                throw Fx.Exception.ArgumentNullOrWhiteSpace(string.IsNullOrWhiteSpace(consumerGroupName) ? nameof(consumerGroupName) : nameof(partitionId));
            }

            return this.OnCreateReceiver(consumerGroupName, partitionId, startOffset, offsetInclusive, null, null);
        }

        /// <summary>
        /// Create the EventHub receiver with given partition id and start receiving from the specified starting offset.
        /// The receiver is created for a specific EventHub Partition from the specific consumer group.
        /// </summary>
        /// <param name="consumerGroupName">the consumer group name that this receiver should be grouped under.</param>
        /// <param name="partitionId">the partition Id that the receiver belongs to. All data received will be from this partition only.</param>
        /// <param name="startTime">the DateTime instant that receive operations will start receive events from. Events received will have <see cref="EventData.SystemProperties.EnqueuedTime"/> later than this Instant.</param>
        /// <returns>The created PartitionReceiver</returns>
        /// <seealso cref="PartitionReceiver"/>
        public PartitionReceiver CreateReceiver(string consumerGroupName, string partitionId, DateTime startTime)
        {
            if (string.IsNullOrWhiteSpace(consumerGroupName) || string.IsNullOrWhiteSpace(partitionId))
            {
                throw Fx.Exception.ArgumentNullOrWhiteSpace(string.IsNullOrWhiteSpace(consumerGroupName) ? nameof(consumerGroupName) : nameof(partitionId));
            }

            return this.OnCreateReceiver(consumerGroupName, partitionId, null, false, startTime, null);
        }

        /// <summary>
        /// Create a Epoch based EventHub receiver with given partition id and start receiving from the beginning of the partition stream.
        /// The receiver is created for a specific EventHub Partition from the specific consumer group.
        /// <para/>
        /// It is important to pay attention to the following when creating epoch based receiver:
        /// <para/>- Ownership enforcement: Once you created an epoch based receiver, you cannot create a non-epoch receiver to the same consumerGroup-Partition combo until all receivers to the combo are closed.
        /// <para/>- Ownership stealing: If a receiver with higher epoch value is created for a consumerGroup-Partition combo, any older epoch receiver to that combo will be force closed.
        /// <para/>- Any receiver closed due to lost of ownership to a consumerGroup-Partition combo will get ReceiverDisconnectedException for all operations from that receiver.
        /// </summary>
        /// <param name="consumerGroupName">the consumer group name that this receiver should be grouped under.</param>
        /// <param name="partitionId">the partition Id that the receiver belongs to. All data received will be from this partition only.</param>
        /// <param name="startingOffset">the offset to start receiving the events from. To receive from start of the stream use <see cref="PartitionReceiver.StartOfStream"/></param>
        /// <param name="epoch">an unique identifier (epoch value) that the service uses, to enforce partition/lease ownership.</param>
        /// <returns>The created PartitionReceiver</returns>
        /// <seealso cref="PartitionReceiver"/>
        public PartitionReceiver CreateEpochReceiver(string consumerGroupName, string partitionId, string startingOffset, long epoch)
        {
            return this.CreateEpochReceiver(consumerGroupName, partitionId, startingOffset, false, epoch);
        }

        /// <summary>
        ///  Create a Epoch based EventHub receiver with given partition id and start receiving from the beginning of the partition stream.
        ///  The receiver is created for a specific EventHub Partition from the specific consumer group.
        ///  <para/> 
        ///  It is important to pay attention to the following when creating epoch based receiver:
        ///  <para/>- Ownership enforcement: Once you created an epoch based receiver, you cannot create a non-epoch receiver to the same consumerGroup-Partition combo until all receivers to the combo are closed.
        ///  <para/>- Ownership stealing: If a receiver with higher epoch value is created for a consumerGroup-Partition combo, any older epoch receiver to that combo will be force closed.
        ///  <para/>- Any receiver closed due to lost of ownership to a consumerGroup-Partition combo will get ReceiverDisconnectedException for all operations from that receiver.
        /// </summary>
        /// <param name="consumerGroupName">the consumer group name that this receiver should be grouped under.</param>
        /// <param name="partitionId">the partition Id that the receiver belongs to. All data received will be from this partition only.</param>
        /// <param name="startingOffset">the offset to start receiving the events from. To receive from start of the stream use <see cref="PartitionReceiver.StartOfStream"/></param>
        /// <param name="offsetInclusive">if set to true, the startingOffset is treated as an inclusive offset - meaning the first event returned is the one that has the starting offset. Normally first event returned is the event after the starting offset.</param>
        /// <param name="epoch">an unique identifier (epoch value) that the service uses, to enforce partition/lease ownership. </param>
        /// <returns>The created PartitionReceiver</returns>
        /// <seealso cref="PartitionReceiver"/>
        public PartitionReceiver CreateEpochReceiver(string consumerGroupName, string partitionId, string startingOffset, bool offsetInclusive, long epoch)
        {
            if (string.IsNullOrWhiteSpace(consumerGroupName) || string.IsNullOrWhiteSpace(partitionId))
            {
                throw Fx.Exception.ArgumentNullOrWhiteSpace(string.IsNullOrWhiteSpace(consumerGroupName) ? nameof(consumerGroupName) : nameof(partitionId));
            }

            return this.OnCreateReceiver(consumerGroupName, partitionId, startingOffset, offsetInclusive, null, epoch);
        }

        /// <summary>
        /// Create a Epoch based EventHub receiver with given partition id and start receiving from the beginning of the partition stream.
        /// The receiver is created for a specific EventHub Partition from the specific consumer group.
        /// <para/>It is important to pay attention to the following when creating epoch based receiver:
        /// <para/>- Ownership enforcement: Once you created an epoch based receiver, you cannot create a non-epoch receiver to the same consumerGroup-Partition combo until all receivers to the combo are closed.
        /// <para/>- Ownership stealing: If a receiver with higher epoch value is created for a consumerGroup-Partition combo, any older epoch receiver to that combo will be force closed.
        /// <para/>- Any receiver closed due to lost of ownership to a consumerGroup-Partition combo will get ReceiverDisconnectedException for all operations from that receiver.
        /// </summary>
        /// <param name="consumerGroupName">the consumer group name that this receiver should be grouped under.</param>
        /// <param name="partitionId">the partition Id that the receiver belongs to. All data received will be from this partition only.</param>
        /// <param name="startTime">the date time instant that receive operations will start receive events from. Events received will have <see cref="EventData.SystemProperties.EnqueuedTime"/> later than this instant.</param>
        /// <param name="epoch">a unique identifier (epoch value) that the service uses, to enforce partition/lease ownership.</param>
        /// <returns>The created PartitionReceiver</returns>
        /// <seealso cref="PartitionReceiver"/>
        public PartitionReceiver CreateEpochReceiver(string consumerGroupName, string partitionId, DateTime startTime, long epoch)
        {
            if (string.IsNullOrWhiteSpace(consumerGroupName) || string.IsNullOrWhiteSpace(partitionId))
            {
                throw Fx.Exception.ArgumentNullOrWhiteSpace(string.IsNullOrWhiteSpace(consumerGroupName) ? nameof(consumerGroupName) : nameof(partitionId));
            }

            return this.OnCreateReceiver(consumerGroupName, partitionId, null, false, startTime, epoch);
        }

        internal EventDataSender CreateEventSender(string partitionId = null)
        {
            return this.OnCreateEventSender(partitionId);
		}

        internal abstract EventDataSender OnCreateEventSender(string partitionId);

        protected abstract PartitionReceiver OnCreateReceiver(string consumerGroupName, string partitionId, string startOffset, bool offsetInclusive, DateTime? startTime, long? epoch);
    }
}
