// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;

    public sealed class EventProcessorOptions
    {
        Action<ExceptionReceivedEventArgs> exceptionHandler;
        
        /// <summary>
        /// Returns an EventProcessorOptions instance with all options set to the default values.
        /// The default values are:
        /// <para>MaxBatchSize: 10</para>
        /// <para>ReceiveTimeOut: 1 minute</para>
        /// <para>PrefetchCount: 300</para>
        /// <para>InitialOffsetProvider: uses the last offset checkpointed, or StartOfStream</para>
        /// <para>InvokeProcessorAfterReceiveTimeout: false</para>
        /// </summary>
        /// <value>an EventProcessorOptions instance with all options set to the default values</value>
        public static EventProcessorOptions DefaultOptions
        {
            get
            {
                return new EventProcessorOptions();
            }
        }

        public EventProcessorOptions()
        {
            this.MaxBatchSize = 10;
            this.PrefetchCount = 300;
            this.ReceiveTimeout = TimeSpan.FromMinutes(1);
        }

        /// <summary>
        /// Sets a handler which receives notification of general exceptions.
        /// <para>Exceptions which occur while processing events from a particular Event Hub partition are delivered
        /// to the onError method of the event processor for that partition. This handler is called on occasions
        /// when there is no event processor associated with the throwing activity, or the event processor could
        /// not be created.</para>
        /// </summary>
        /// <param name="exceptionHandler">Handler which is called when an exception occurs. Set to null to stop handling.</param>
        public void SetExceptionHandler(Action<ExceptionReceivedEventArgs> exceptionHandler)
        {
            this.exceptionHandler = exceptionHandler;
        }

        /// <summary>
        /// Returns the maximum size of an event batch that IEventProcessor.ProcessEventsAsync will be called with
        /// </summary>
        public int MaxBatchSize { get; set; }

        /// <summary>
        /// Gets or sets the timeout length for receive operations.
        /// </summary>
        public TimeSpan ReceiveTimeout { get; set; }

        /// <summary>
        /// Gets or sets the current prefetch count for the underlying client.
        /// The default is 300.
        /// </summary>
        public int PrefetchCount { get; set; }

        /// <summary>
        /// Returns the current function used to determine the initial offset at which to start receiving
        /// events for a partition.
        /// <para>A null return indicates that it is using the internal provider, which uses the last checkpointed
        /// offset value (if present) or StartOfSTream (if not).</para>
        /// </summary>
        public Func<string, string> InitialOffsetProvider { get; set; }

        /// <summary>
        /// Returns whether the EventProcessorHost will call IEventProcessor.OnEvents(null) when a receive
        /// timeout occurs (true) or not (false).
        /// </summary>
        public bool InvokeProcessorAfterReceiveTimeout { get; set; }

        internal void NotifyOfException(string hostname, string partitionId, Exception exception, string action)
        {
            this.exceptionHandler?.Invoke(new ExceptionReceivedEventArgs(hostname, partitionId, exception, action));
        }
    }
}