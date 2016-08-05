// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using Microsoft.Azure.Amqp;
    using Microsoft.Azure.Amqp.Serialization;
    using Microsoft.Azure.EventHubs.Amqp;

    [AmqpContract(Name = AmqpConstants.Vendor + ":eventhub-runtime-info:map", Encoding = EncodingType.Map)]
    public class EventHubRuntimeInformation
    {
        [AmqpMember(Name = AmqpClientConstants.EntityNameKey)]
        public string Path { get; set; }

        [AmqpMember(Name = AmqpClientConstants.ManagementEntityTypeKey)]
        internal string Type { get; set; }

        [AmqpMember(Name = AmqpClientConstants.ManagementEventHubCreatedAt)]
        public DateTime CreatedAt { get; set; }

        [AmqpMember(Name = AmqpClientConstants.ManagementEventHubPartitionCount)]
        public int PartitionCount { get; set; }

        [AmqpMember(Name = AmqpClientConstants.ManagementEventHubPartitionIds)]
        public string[] PartitionIds { get; set; }
    }
}