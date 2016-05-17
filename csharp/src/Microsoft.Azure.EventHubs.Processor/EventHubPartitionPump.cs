// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Threading.Tasks;

    class EventHubPartitionPump : PartitionPump
    {
        EventHubClient eventHubClient;
        PartitionReceiver partitionReceiver;
        PartitionReceiveHandler partitionReceiveHandler;

        public EventHubPartitionPump(EventProcessorHost host, Lease lease)
            : base(host, lease)
        {
        }

        protected override async Task OnOpenAsync()
        {
            bool openedOK = false;
            int retryCount = 0;
            Exception lastException = null;
            do
            {
                try
                {
                    await OpenClientsAsync();
                    openedOK = true;
                }
                catch (Exception e)
                {
                    lastException = e;
                    if (e is ReceiverDisconnectedException)
	        	    {
                        // TODO Assuming this is due to a receiver with a higher epoch.
                        // Is there a way to be sure without checking the exception text?
                        this.Host.LogWithHostAndPartition(EventLevel.Warning, this.PartitionContext, "Receiver disconnected on create, bad epoch?", e);
                        // If it's a bad epoch, then retrying isn't going to help.
                        break;
                    }
	        	    else
	        	    {
                        this.Host.LogWithHostAndPartition(EventLevel.Warning, this.PartitionContext, "Failure creating client or receiver, retrying", e);
                        retryCount++;
                    }
                }
            }
            while (!openedOK && (retryCount < 5));

            if (!openedOK)
            {
                // IEventProcessor.onOpen is called from the base PartitionPump and must have returned in order for execution to reach here, 
                // so we can report this error to it instead of the general error handler.
                await this.Processor.ProcessErrorAsync(this.PartitionContext, lastException);
                this.PumpStatus = PartitionPumpStatus.OpenFailed;
            }

            if (this.PumpStatus == PartitionPumpStatus.Opening)
            {
                this.partitionReceiveHandler = new PartitionReceiveHandler(this);
                // IEventProcessor.OnOpen is called from the base PartitionPump and must have returned in order for execution to reach here, 
                // meaning it is safe to set the handler and start calling IEventProcessor.OnEvents.
                // Set the status to running before setting the javaClient handler, so the IEventProcessor.OnEvents can never race and see status != running.
                this.PumpStatus = PartitionPumpStatus.Running;
                this.partitionReceiver.SetReceiveHandler(this.partitionReceiveHandler);
            }

            if (this.PumpStatus == PartitionPumpStatus.OpenFailed)
            {
                this.PumpStatus = PartitionPumpStatus.Closing;
                await this.CleanUpClientsAsync();
                this.PumpStatus = PartitionPumpStatus.Closed;
            }
        }

        async Task OpenClientsAsync() // throws ServiceBusException, IOException, InterruptedException, ExecutionException
        {
            // Create new client
            this.Host.LogWithHostAndPartition(EventLevel.Informational, this.PartitionContext, "Creating EH client");
		    this.eventHubClient = EventHubClient.Create(this.Host.EventHubConnectionString);

            // Create new receiver and set options
            string startingOffset = await this.PartitionContext.GetInitialOffsetAsync();
            long epoch = this.Lease.Epoch;
            this.Host.LogWithHostAndPartition(EventLevel.Informational, this.PartitionContext, "Opening EH receiver with epoch " + epoch + " at offset " + startingOffset);
            this.partitionReceiver = this.eventHubClient.CreateEpochReceiver(this.PartitionContext.ConsumerGroupName, this.PartitionContext.PartitionId, startingOffset, epoch);
            
            this.partitionReceiver.PrefetchCount = this.Host.EventProcessorOptions.PrefetchCount;

            this.Host.LogWithHostAndPartition(EventLevel.Informational, this.PartitionContext, "EH client and receiver creation finished");
        }

        async Task CleanUpClientsAsync() // swallows all exceptions
        {
            if (this.partitionReceiver != null)
            {
                // Taking the lock means that there is no ProcessEventsAsync call in progress.
                using (await this.ProcessingAsyncLock.LockAsync())
                {
                    // Disconnect the processor from the receiver we're about to close.
                    // Fortunately this is idempotent -- setting the handler to null when it's already been
                    // nulled by code elsewhere is harmless!
                    this.partitionReceiver.SetReceiveHandler(null);
                }

                this.Host.LogWithHostAndPartition(EventLevel.Informational, this.PartitionContext, "Closing EH receiver");
                await this.partitionReceiver.CloseAsync();
                this.partitionReceiver = null;
            }

            if (this.eventHubClient != null)
            {
                this.Host.LogWithHostAndPartition(EventLevel.Informational, this.PartitionContext, "Closing EH client");
                await this.eventHubClient.CloseAsync();
                this.eventHubClient = null;
            }
        }

        protected override async Task OnClosingAsync(CloseReason reason)
        {
            // If an open operation is stuck, this lets us shut down anyway.
            // TODO: Cancel any inflight work.

            if (this.partitionReceiver != null)
            {
                // Disconnect any processor from the receiver so the processor won't get
                // any more calls. But a call could be in progress right now. 
                this.partitionReceiver.SetReceiveHandler(null);

                // Close the EH clients. Errors are swallowed, nothing we could do about them anyway.
                await CleanUpClientsAsync();
            }
        }

        class PartitionReceiveHandler : IPartitionReceiveHandler
        {
            readonly EventHubPartitionPump eventHubPartitionPump;
            public PartitionReceiveHandler(EventHubPartitionPump eventHubPartitionPump)
            {
                this.eventHubPartitionPump = eventHubPartitionPump;
            }

            public Task ProcessEventsAsync(IEnumerable<EventData> events)
            {
                // This method is called on the thread that the Java EH client uses to run the pump.
                // There is one pump per EventHubClient. Since each PartitionPump creates a new EventHubClient,
                // using that thread to call OnEvents does no harm. Even if OnEvents is slow, the pump will
                // get control back each time OnEvents returns, and be able to receive a new batch of messages
                // with which to make the next OnEvents call. The pump gains nothing by running faster than OnEvents.
                return this.eventHubPartitionPump.ProcessEventsAsync(events);
            }

            public async Task ProcessErrorAsync(Exception error)
            {
                if (error == null)
                {
                    error = new Exception("No error info supplied by EventHub client");
                }

                if (error is ReceiverDisconnectedException)
			    {
                    this.eventHubPartitionPump.Host.LogWithHostAndPartition(EventLevel.Warning, this.eventHubPartitionPump.PartitionContext,
                        "EventHub client disconnected, probably another host took the partition");
                }
			    else
			    {
                    this.eventHubPartitionPump.Host.LogWithHostAndPartition(EventLevel.Error, this.eventHubPartitionPump.PartitionContext, "EventHub client error: ", error);
                    await this.eventHubPartitionPump.ProcessErrorAsync(error);
                }

                this.eventHubPartitionPump.PumpStatus = PartitionPumpStatus.Errored;
            }

            public Task CloseAsync(Exception error)
            {
                if (error == null)
                {
                    error = new Exception("normal shutdown"); // TODO -- is this true?
                }

                this.eventHubPartitionPump.Host.LogWithHostAndPartition(EventLevel.Error, this.eventHubPartitionPump.PartitionContext, "EventHub client closed: ", error);
                this.eventHubPartitionPump.PumpStatus = PartitionPumpStatus.Errored;
                return Task.CompletedTask;
            }
        }
    }
}