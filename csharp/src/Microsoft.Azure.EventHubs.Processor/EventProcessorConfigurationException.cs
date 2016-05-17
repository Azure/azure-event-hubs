// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;

    public class EventProcessorConfigurationException : ServiceBusException
    {
        public EventProcessorConfigurationException(string message)
            : this(message, null)
        {
        }

        public EventProcessorConfigurationException(string message, Exception innerException)
            : base(false, message, innerException)
        {
        }
    }
}