// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;

    public class EventProcessorRuntimeException : ServiceBusException
    {
        public EventProcessorRuntimeException(string message, string action)
            : this(message, action, null)
        {
        }

        public EventProcessorRuntimeException(string message, string action, Exception innerException)
            : base(true, message, innerException)
        {
            this.Action = action;
        }

        public string Action { get; }
    }
}