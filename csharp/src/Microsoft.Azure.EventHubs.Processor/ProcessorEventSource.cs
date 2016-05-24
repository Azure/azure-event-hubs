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

        // TODO: Add Keywords if desired.
        //public class Keywords   // This is a bitvector
        //{
        //    public const EventKeywords Amqp = (EventKeywords)0x0001;
        //    public const EventKeywords Debug = (EventKeywords)0x0002;
        //}
    }
}
