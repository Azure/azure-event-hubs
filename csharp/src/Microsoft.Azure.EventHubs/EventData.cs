// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.Amqp;

    /// <summary>
    /// The data structure encapsulating the Event being sent-to and received-from EventHubs.
    /// Each EventHubs partition can be visualized as a Stream of EventData.
    /// </summary>
    public class EventData
    {
        EventData()
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Internal Constructor - intended to be used only by the PartitionReceiver to create EventData out of AmqpMessage
        /// </summary>
        EventData(AmqpMessage amqpMessage)
        {
            throw new NotImplementedException();
        }

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
        /// <param name="data">The actual payload of data in bytes to be sent to the EventHub.</param>
        public EventData(byte[] data)
            : this(new ArraySegment<byte>(data))
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
            : this()
        {
        }

        /// <summary>
        /// Get the actual Payload/Data wrapped by EventData.
        /// This is intended to be used after receiving EventData using <see cref="PartitionReceiver"/>.
        /// </summary>
        public byte[] Body
        {
            // TODO: enforce on-send constructor type 2
            get
            {
                throw new NotImplementedException();
            }
        }

        /// <summary>
        /// Application property bag
        /// </summary>
        public Dictionary<string, string> Properties
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
        /// SystemProperties that are populated by EventHubService.
        /// As these are populated by Service, they are only present on a Received EventData.
        /// </summary>
        public SystemPropertiesCollection SystemProperties
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        AmqpMessage ToAmqpMessage()
        {
            throw new NotImplementedException();
        }

        AmqpMessage ToAmqpMessage(string partitionKey)
        {
            throw new NotImplementedException();
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
                get
                {
                    throw new NotImplementedException();
                }
            }

            public DateTime EnqueuedTime
            {
                get
                {
                    throw new NotImplementedException();
                }
            }

            public string Offset
            {
                get
                {
                    throw new NotImplementedException();
                }
            }

            public string PartitionKey
            {
                get
                {
                    throw new NotImplementedException();
                }
            }
        }
    }
}
