// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;

    /// <summary>
    /// Base Exception for various Event Hubs errors.
    /// </summary>
    public class EventHubsException : Exception
    {
        public EventHubsException(bool isTransient)
        {
            this.IsTransient = isTransient;
        }

        public EventHubsException(bool isTransient, string message)
            : base(message)
        {
            this.IsTransient = isTransient;
        }

        public EventHubsException(bool isTransient, Exception innerException)
            : base(innerException.Message, innerException)
        {
            this.IsTransient = isTransient;
        }

        public EventHubsException(bool isTransient, string message, Exception innerException)
            : base(message, innerException)
        {
            this.IsTransient = isTransient;
        }

        public override string Message
        {
            get
            {
                string baseMessage = base.Message;
                if (string.IsNullOrEmpty(this.EventHubsNamespace))
                {
                    return baseMessage;
                }

                return "{0}, ({1})".FormatInvariant(this.EventHubsNamespace);
            }
        }

        /// <summary>
        /// A boolean indicating if the exception is a transient error or not.
        /// </summary>
        /// <value>returns true when user can retry the operation that generated the exception without additional intervention.</value>
        public bool IsTransient { get; }

        public string EventHubsNamespace { get; internal set; }
    }
}
