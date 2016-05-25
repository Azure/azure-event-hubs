// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Diagnostics.Tracing;

    /// <summary>
    /// EventSource for Microsoft-Azure-EventHubs traces.
    /// 
    /// When defining Start/Stop tasks, the StopEvent.Id must be exactly StartEvent.Id + 1.
    /// 
    /// Do not explicity include the Guid here, since EventSource has a mechanism to automatically
    /// map to an EventSource Guid based on the Name (Microsoft-Azure-EventHubs-Processor).
    /// </summary>
    [EventSource(Name = "Microsoft-Azure-EventHubs-Processor")]
    public class ProcessorEventSource : EventSource
    {
        public static ProcessorEventSource Log { get; } = new ProcessorEventSource();

        ProcessorEventSource() { }

        [Event(1, Level = EventLevel.Informational, Message = "{0}: created. Namespace: {1}, EventHub: {2}.")]
        public void EventProcessorHostCreated(string hostId, string namespaceName, string path)
        {
            if (IsEnabled())
            {
                WriteEvent(1, hostId, namespaceName, path);
            }
        }

        [Event(2, Level = EventLevel.Informational, Message = "{0}: closing.")]
        public void EventProcessorHostCloseStart(string hostId)
        {
            if (IsEnabled())
            {
                WriteEvent(2, hostId);
            }
        }

        [Event(3, Level = EventLevel.Informational, Message = "{0}: closed.")]
        public void EventProcessorHostCloseStop(string hostId)
        {
            if (IsEnabled())
            {
                WriteEvent(3, hostId);
            }
        }

        [Event(4, Level = EventLevel.Error, Message = "{0}: close failed: {1}.")]
        public void EventProcessorHostCloseError(string hostId, string error)
        {
            if (IsEnabled())
            {
                WriteEvent(4, hostId, error);
            }
        }

        [Event(5, Level = EventLevel.Informational, Message = "{0}: opening. Factory:{1}.")]
        public void EventProcessorHostOpenStart(string hostId, string factoryType)
        {
            if (IsEnabled())
            {
                WriteEvent(5, hostId, factoryType ?? string.Empty);
            }
        }

        [Event(6, Level = EventLevel.Informational, Message = "{0}: opened.")]
        public void EventProcessorHostOpenStop(string hostId)
        {
            if (IsEnabled())
            {
                WriteEvent(6, hostId);
            }
        }

        [Event(7, Level = EventLevel.Error, Message = "{0}: open failed: {1}.")]
        public void EventProcessorHostOpenError(string hostId, string error)
        {
            if (IsEnabled())
            {
                WriteEvent(7, hostId, error);
            }
        }

        [Event(8, Level = EventLevel.Informational, Message = "{0}: {1}")]
        public void EventProcessorHostInfo(string hostId, string details)
        {
            if (IsEnabled())
            {
                WriteEvent(8, hostId, details);
            }
        }

        [Event(9, Level = EventLevel.Warning, Message = "{0}: Warning: {1}. {2}")]
        public void EventProcessorHostWarning(string hostId, string details, string error)
        {
            if (IsEnabled())
            {
                WriteEvent(9, hostId, details, error ?? string.Empty);
            }
        }

        [Event(10, Level = EventLevel.Error, Message = "{0}: Error: {1}. {2}")]
        public void EventProcessorHostError(string hostId, string details, string error)
        {
            if (IsEnabled())
            {
                WriteEvent(10, hostId, details, error ?? string.Empty);
            }
        }



        [Event(11, Level = EventLevel.Informational, Message = "{0}: Partition {1}: Pump closing. Reason:{2}.")]
        public void PartitionPumpCloseStart(string hostId, string partitionId, string reason)
        {
            if (IsEnabled())
            {
                WriteEvent(11, hostId, partitionId, reason);
            }
        }

        [Event(12, Level = EventLevel.Informational, Message = "{0}: Partition {1}: Pump closed.")]
        public void PartitionPumpCloseStop(string hostId, string partitionId)
        {
            if (IsEnabled())
            {
                WriteEvent(12, hostId, partitionId);
            }
        }

        [Event(13, Level = EventLevel.Error, Message = "{0}: Partition {1}: Pump close error: {2}.")]
        public void PartitionPumpCloseError(string hostId, string partitionId, string error)
        {
            if (IsEnabled())
            {
                WriteEvent(13, hostId, partitionId, error ?? string.Empty);
            }
        }

        [Event(14, Level = EventLevel.Informational, Message = "{0}: Partition {1}: Saving checkpoint at Offset:{2}/SequenceNumber:{3}.")]
        public void PartitionPumpCheckpointStart(string hostId, string partitionId, string offset, long sequenceNumber)
        {
            if (IsEnabled())
            {
                WriteEvent(14, hostId, partitionId, offset ?? string.Empty, sequenceNumber);
            }
        }

        [Event(15, Level = EventLevel.Informational, Message = "{0}: Partition {1}: Done saving checkpoint.")]
        public void PartitionPumpCheckpointStop(string hostId, string partitionId)
        {
            if (IsEnabled())
            {
                WriteEvent(15, hostId, partitionId);
            }
        }

        [Event(16, Level = EventLevel.Error, Message = "{0}: Partition {1}: Error saving checkpoint: {2}.")]
        public void PartitionPumpCheckpointError(string hostId, string partitionId, string error)
        {
            if (IsEnabled())
            {
                WriteEvent(16, hostId, partitionId, error ?? string.Empty);
            }
        }

        [Event(17, Level = EventLevel.Informational, Message = "{0}: Partition {1}: Creating EventHubClient and PartitionReceiver with Epoch:{2} Offset: {3}.")]
        public void PartitionPumpCreateClientsStart(string hostId, string partitionId, long epoch, string startOffset)
        {
            if (IsEnabled())
            {
                WriteEvent(17, hostId, partitionId, epoch, startOffset ?? string.Empty);
            }
        }

        [Event(18, Level = EventLevel.Informational, Message = "{0}: Partition {1}: Done creating EventHubClient and PartitionReceiver.")]
        public void PartitionPumpCreateClientsStop(string hostId, string partitionId)
        {
            if (IsEnabled())
            {
                WriteEvent(18, hostId, partitionId);
            }
        }

        [Event(19, Level = EventLevel.Informational, Message = "{0}: Partition {1}: IEventProcessor opening. Type: {2}.")]
        public void PartitionPumpOpenProcessorStart(string hostId, string partitionId, string processorType)
        {
            if (IsEnabled())
            {
                WriteEvent(19, hostId, partitionId, processorType);
            }
        }

        [Event(20, Level = EventLevel.Informational, Message = "{0}: Partition {1}: IEventProcessor opened.")]
        public void PartitionPumpOpenProcessorStop(string hostId, string partitionId)
        {
            if (IsEnabled())
            {
                WriteEvent(20, hostId, partitionId);
            }
        }

        [Event(21, Level = EventLevel.Informational, Message = "{0}: Partition {1}: IEventProcessor closing.")]
        public void PartitionPumpCloseProcessorStart(string hostId, string partitionId)
        {
            if (IsEnabled())
            {
                WriteEvent(21, hostId, partitionId);
            }
        }

        [Event(22, Level = EventLevel.Informational, Message = "{0}: Partition {1}: IEventProcessor closed.")]
        public void PartitionPumpCloseProcessorStop(string hostId, string partitionId)
        {
            if (IsEnabled())
            {
                WriteEvent(22, hostId, partitionId);
            }
        }


        // TODO: Add Keywords if desired.
        //public class Keywords   // This is a bitvector
        //{
        //    public const EventKeywords Amqp = (EventKeywords)0x0001;
        //    public const EventKeywords Debug = (EventKeywords)0x0002;
        //}
    }
}
