// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;

    public sealed class ExceptionReceivedEventArgs
    {
        internal ExceptionReceivedEventArgs(string hostname, Exception exception, string action)
        {
            this.Hostname = hostname;
            this.Exception = exception;
            this.Action = action;
        }

        /// <summary>
        /// Allows distinguishing the error source if multiple hosts in a single process.
        /// </summary>
        /// <value>The name of the host that experienced the exception.</value>
        public string Hostname { get; }

        /// <summary>
        /// The exception that was thrown.
        /// </summary>
        public Exception Exception { get; }

        /// <summary>
        /// A short string that indicates what general activity threw the exception.
        /// See EventProcessorHostActionString for a list of possible values.
        /// </summary>
        public string Action { get; }
    }
}