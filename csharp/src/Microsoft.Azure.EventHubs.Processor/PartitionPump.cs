// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
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
                    ProcessorEventSource.Log.PartitionPumpInvokeProcessorOpenStart(this.Host.Id, this.PartitionContext.PartitionId, this.Processor.GetType().ToString());
                    await this.Processor.OpenAsync(this.PartitionContext);
                    ProcessorEventSource.Log.PartitionPumpInvokeProcessorOpenStop(this.Host.Id, this.PartitionContext.PartitionId);
                }
                catch (Exception e)
                {
                    // If the processor won't create or open, only thing we can do here is pass the buck.
                    // Null it out so we don't try to operate on it further.
                    ProcessorEventSource.Log.PartitionPumpError(this.Host.Id, this.PartitionContext.PartitionId, "Failed " + action, e.ToString());
                    this.Processor = null;
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
            ProcessorEventSource.Log.PartitionPumpCloseStart(this.Host.Id, this.PartitionContext.PartitionId, reason.ToString());
            this.PumpStatus = PartitionPumpStatus.Closing;
            try
            {
                await this.OnClosingAsync(reason);

                if (this.Processor != null)
                {
                    using (await this.ProcessingAsyncLock.LockAsync())
                    {
                        // When we take the lock, any existing ProcessEventsAsync call has finished.
                        // Because the client has been closed, there will not be any more
                        // calls to onEvents in the future. Therefore we can safely call CloseAsync.
                        ProcessorEventSource.Log.PartitionPumpInvokeProcessorCloseStart(this.Host.Id, this.PartitionContext.PartitionId, reason.ToString());
                        await this.Processor.CloseAsync(this.PartitionContext, reason);
                        ProcessorEventSource.Log.PartitionPumpInvokeProcessorCloseStop(this.Host.Id, this.PartitionContext.PartitionId);
                    }
                }
            }
            catch (Exception e)
            {
                ProcessorEventSource.Log.PartitionPumpCloseError(this.Host.Id, this.PartitionContext.PartitionId, e.ToString());
                // If closing the processor has failed, the state of the processor is suspect.
                // Report the failure to the general error handler instead.
                this.Host.EventProcessorOptions.NotifyOfException(this.Host.HostName, e, "Closing Event Processor");
            }

            this.PumpStatus = PartitionPumpStatus.Closed;
            ProcessorEventSource.Log.PartitionPumpCloseStop(this.Host.Id, this.PartitionContext.PartitionId);
        }

        protected abstract Task OnClosingAsync(CloseReason reason);

        protected async Task ProcessEventsAsync(IEnumerable<EventData> events)
        {
            // Assumes that .NET Core client will call with null on receive timeout.
            if (events == null && this.Host.EventProcessorOptions.InvokeProcessorAfterReceiveTimeout == false)
            {
                return;
            }

            // Synchronize to serialize calls to the processor.
            // The handler is not installed until after OpenAsync returns, so ProcessEventsAsync cannot conflict with OpenAsync.
            // There could be a conflict between ProcessEventsAsync and CloseAsync, however. All calls to CloseAsync are
            // protected by synchronizing too.
            using (await this.ProcessingAsyncLock.LockAsync())
            {
                int eventCount = events != null ? events.Count() : 0;
                ProcessorEventSource.Log.PartitionPumpInvokeProcessorEventsStart(this.Host.Id, this.PartitionContext.PartitionId, eventCount);
                try
                {
                    await this.Processor.ProcessEventsAsync(this.PartitionContext, events);
                }
                catch (Exception e)
                {
                    // TODO -- do we pass errors from IEventProcessor.ProcessEventsAsync to IEventProcessor.ProcessErrorAsync?
                    // Depending on how you look at it, that's either pointless (if the user's code throws, the user's code should already know about it) or
                    // a convenient way of centralizing error handling.
                    // For the meantime just trace it.
                    ProcessorEventSource.Log.PartitionPumpInvokeProcessorEventsError(this.Host.Id, this.PartitionContext.PartitionId, e.ToString());
                }
                finally
                {
                    ProcessorEventSource.Log.PartitionPumpInvokeProcessorEventsStop(this.Host.Id, this.PartitionContext.PartitionId);
                }

                EventData last = events.LastOrDefault();
                if (last != null)
                {
                    ProcessorEventSource.Log.PartitionPumpInfo(
                        this.Host.Id,
                        this.PartitionContext.PartitionId,
                        "Updating offset in partition context with end of batch " + last.SystemProperties.Offset + "/" + last.SystemProperties.SequenceNumber);
                    this.PartitionContext.SetOffsetAndSequenceNumber(last);
                }
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