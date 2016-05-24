// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    abstract class PartitionPump
    {   
	    protected PartitionPump(EventProcessorHost host, Lease lease)
        {
            this.Host = host;
            this.Lease = lease;
            this.ProcessingAsyncLock = new AsyncLock();
            this.PumpStatus = PartitionPumpStatus.Uninitialized;
        }

        protected EventProcessorHost Host { get; }

        protected Lease Lease { get; }

        protected IEventProcessor Processor { get; private set; }

        protected PartitionContext PartitionContext { get; private set; }

        protected AsyncLock ProcessingAsyncLock { get; }


        internal void SetLease(Lease newLease)
        {
            this.PartitionContext.Lease = newLease;
        }

        public async Task OpenAsync()
        {
            this.PumpStatus = PartitionPumpStatus.Opening;

            this.PartitionContext = new PartitionContext(this.Host, this.Lease.PartitionId, this.Host.EventHubPath, this.Host.ConsumerGroupName);
            this.PartitionContext.Lease = this.Lease;

            if (this.PumpStatus == PartitionPumpStatus.Opening)
            {
                string action = EventProcessorHostActionStrings.CreatingEventProcessor;
                try
                {
                    this.Processor = this.Host.ProcessorFactory.CreateEventProcessor(this.PartitionContext);
                    action = EventProcessorHostActionStrings.OpeningEventProcessor;
                    await this.Processor.OpenAsync(this.PartitionContext);
                }
                catch (Exception e)
                {
                    // If the processor won't create or open, only thing we can do here is pass the buck.
                    // Null it out so we don't try to operate on it further.
                    this.Processor = null;
                    this.Host.LogPartitionError(this.PartitionContext.PartitionId, "Failed " + action, e);
                    this.Host.EventProcessorOptions.NotifyOfException(this.Host.HostName, e, action);
                    this.PumpStatus = PartitionPumpStatus.OpenFailed;
                }
            }

            if (this.PumpStatus == PartitionPumpStatus.Opening)
            {
                await this.OnOpenAsync();
            }
        }

        protected abstract Task OnOpenAsync();

        protected internal PartitionPumpStatus PumpStatus { get; protected set; }

        internal bool IsClosing
        {
            get
            {
                return (this.PumpStatus == PartitionPumpStatus.Closing || this.PumpStatus == PartitionPumpStatus.Closed);
            }
        }

        public async Task CloseAsync(CloseReason reason)
        {
            this.PumpStatus = PartitionPumpStatus.Closing;
            this.Host.LogPartitionInfo(this.PartitionContext.PartitionId, "pump shutdown for reason " + reason);

            await this.OnClosingAsync(reason);

            if (this.Processor != null)
            {
                try
                {
                    using (await this.ProcessingAsyncLock.LockAsync())
                    {
                        // When we take the lock, any existing ProcessEventsAsync call has finished.
                        // Because the client has been closed, there will not be any more
                        // calls to onEvents in the future. Therefore we can safely call CloseAsync.
                        await this.Processor.CloseAsync(this.PartitionContext, reason);
                    }
                }
                catch (Exception e)
                {
                    this.Host.LogPartitionError(this.PartitionContext.PartitionId, "Failure closing processor", e);
                    // If closing the processor has failed, the state of the processor is suspect.
                    // Report the failure to the general error handler instead.
                    this.Host.EventProcessorOptions.NotifyOfException(this.Host.HostName, e, "Closing Event Processor");
                }
            }

            this.PumpStatus = PartitionPumpStatus.Closed;
        }

        protected abstract Task OnClosingAsync(CloseReason reason);

        protected async Task ProcessEventsAsync(IEnumerable<EventData> events)
        {
            // Assumes that javaClient will call with null on receive timeout. Currently it doesn't call at all.
            // See note on EventProcessorOptions.
            if (events == null && this.Host.EventProcessorOptions.InvokeProcessorAfterReceiveTimeout == false)
            {
                return;
            }

            try
            {
                // Synchronize to serialize calls to the processor.
                // The handler is not installed until after OpenAsync returns, so onEvents cannot conflict with OpenAsync.
                // There could be a conflict between ProcessEventsAsync and CloseAsync, however. All calls to CloseAsync are
                // protected by synchronizing too.
                using (await this.ProcessingAsyncLock.LockAsync())
                {
                    await this.Processor.ProcessEventsAsync(this.PartitionContext, events);

                    EventData last = events.LastOrDefault();
                    if (last != null)
                    {
                        this.Host.LogPartitionInfo(
                            this.PartitionContext.PartitionId,
                            "Updating offset in partition context with end of batch " + last.SystemProperties.Offset + "/" + last.SystemProperties.SequenceNumber);
                        this.PartitionContext.SetOffsetAndSequenceNumber(last);
                    }
                }
            }
            catch (Exception e)
            {
                // TODO -- do we pass errors from IEventProcessor.onEvents to IEventProcessor.onError?
                // Depending on how you look at it, that's either pointless (if the user's code throws, the user's code should already know about it) or
                // a convenient way of centralizing error handling.
                // In the meantime, just trace it.
                this.Host.LogPartitionError(this.PartitionContext.PartitionId, "Got exception from ProcessEventsAsync", e);
            }
        }

        protected Task ProcessErrorAsync(Exception error)
        {
            // This handler is called when client calls the error handler we have installed.
            // JavaClient can only do that when execution is down in javaClient. Therefore no onEvents
            // call can be in progress right now. JavaClient will not get control back until this handler
            // returns, so there will be no calls to onEvents until after the user's error handler has returned.
            return this.Processor.ProcessErrorAsync(this.PartitionContext, error);
        }
    }
}