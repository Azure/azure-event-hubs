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
    /// map to an EventSource Guid based on the Name (Microsoft-Azure-EventHubs).
    /// </summary>
    [EventSource(Name = "Microsoft-Azure-EventHubs")]
    public class EventHubsEventSource : EventSource
    {
        public static EventHubsEventSource Log { get; } = new EventHubsEventSource();

        EventHubsEventSource() { }

        [Event(1, Level = EventLevel.Informational, Message = "Creating EventHubClient (Namespace '{0}'; EventHub '{1}').")]
        public void EventHubClientCreateStart(string nameSpace, string eventHubName)
        {
            if (IsEnabled())
            {
                WriteEvent(1, nameSpace, eventHubName);
            }
        }

        [Event(2, Level = EventLevel.Informational, Message = "Done creating EventHubClient")]
        public void EventHubClientCreateStop()
        {
            if (IsEnabled())
            {
                WriteEvent(2);
            }
        }

        [Event(3, Level = EventLevel.Informational, Message = "Sending {0} message(s) to partitionId '{1}'.")]
        public void EventSendStart(int count, string partitionId)
        {
            if (IsEnabled())
            {
                WriteEvent(3, count, partitionId);
            }
        }

        [Event(4, Level = EventLevel.Informational, Message = "Done sending message(s).")]
        public void EventSendStop()
        {
            if (IsEnabled())
            {
                WriteEvent(4);
            }
        }

        [Event(5, Level = EventLevel.Error, Message = "Error sending message(s): {0}.")]
        public void EventSendException(string error)
        {
            if (IsEnabled())
            {
                WriteEvent(5, error);
            }
        }

        [Event(6, Level = EventLevel.Error, Message = "Throwing Exception: {0}")]
        public void ThrowingExceptionError(string error)
        {
            if (IsEnabled())
            {
                WriteEvent(6, error);
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
