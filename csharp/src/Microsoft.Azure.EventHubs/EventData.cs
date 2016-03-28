// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// The data structure encapsulating the Event being sent-to and received-from EventHubs.
    /// Each EventHubs partition can be visualized as a Stream of EventData.
    /// </summary>
    public class EventData
    {
        /// <summary>
        /// Construct EventData to send to EventHub.
        /// Typical pattern to create a Sending EventData is:
        /// <para>i. Serialize the sending ApplicationEvent to be sent to EventHubs into bytes.</para>
        /// <para>ii. If complex serialization logic is involved (for example: multiple types of data) - add a Hint using the <see cref="EventData.Properties"/> for the Consumer.</para>
        /// </summary>
        /// <example>Sample Code:
        /// <code>
        /// EventData eventData = new EventData(telemetryEventBytes);
        /// var applicationProperties = new Dictionary&lt;string, string&gt;();
        /// applicationProperties["eventType"] = "com.microsoft.azure.monitoring.EtlEvent";
        /// eventData.Properties(applicationProperties);
        /// await partitionSender.SendAsync(eventData);
        /// </code>
        /// </example>
        /// <param name="array">The actual payload of data in bytes to be sent to the EventHub.</param>
        public EventData(byte[] array)
            : this(new ArraySegment<byte>(array))
        {
        }

        /// <summary>
        /// Construct EventData to send to EventHub.
        /// Typical pattern to create a Sending EventData is:
        /// <para>i.  Serialize the sending ApplicationEvent to be sent to EventHub into bytes.</para>
        /// <para>ii. If complex serialization logic is involved (for example: multiple types of data) - add a Hint using the <see cref="EventData.Properties"/> for the Consumer.</para>
        /// </summary>
        /// <example>Sample Code:
        /// <code>
        /// EventData eventData = new EventData(new ArraySegment&lt;byte&gt;(eventBytes, offset, count));
        /// var applicationProperties = new Dictionary&lt;string, string&gt;();
        /// applicationProperties["eventType"] = "com.microsoft.azure.monitoring.EtlEvent";
        /// eventData.Properties(applicationProperties);
        /// await partitionSender.SendAsync(eventData);
        /// </code>
        /// </example>
        /// <param name="arraySegment">The payload bytes, offset and length to be sent to the EventHub.</param>
        public EventData(ArraySegment<byte> arraySegment)
        {
            this.Body = arraySegment;
        }

        /// <summary>
        /// Get the actual Payload/Data wrapped by EventData.
        /// This is intended to be used after receiving EventData using <see cref="PartitionReceiver"/>.
        /// </summary>
        public ArraySegment<byte> Body
        {
            get;
        }

        /// <summary>
        /// Application property bag
        /// </summary>
        public IDictionary<string, string> Properties
        {
            get; set;
        }

        /// <summary>
        /// SystemProperties that are populated by EventHubService.
        /// As these are populated by Service, they are only present on a Received EventData.
        /// </summary>
        public SystemPropertiesCollection SystemProperties
        {
            get; internal set;
        }

        public sealed class SystemPropertiesCollection
        {
            EventData eventData;

            SystemPropertiesCollection(EventData eventData)
            {
                this.eventData = eventData;
            }

            public long SequenceNumber
            {
                get; internal set;
            }

            public DateTime EnqueuedTime
            {
                get; internal set;
            }

            public string Offset
            {
                get; internal set;
            }

            public string PartitionKey
            {
                get; internal set;
            }
        }
    }
}
