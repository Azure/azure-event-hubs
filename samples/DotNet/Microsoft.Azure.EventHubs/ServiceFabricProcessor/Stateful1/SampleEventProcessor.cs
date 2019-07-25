using System;
using System.Collections.Generic;
using System.Text;

namespace Stateful1
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.EventHubs;
    using Microsoft.Azure.EventHubs.ServiceFabricProcessor;

    class SampleEventProcessor : IEventProcessor
    {
        private int total = 0;
        private readonly long checkpointInterval = 100L;

        override public Task OpenAsync(CancellationToken cancellationToken, PartitionContext context)
        {
            ServiceEventSource.Current.Message("SAMPLE IEventProcessor.OpenAsync for {0}", context.PartitionId);
            return Task.CompletedTask;
        }

        public override Task CloseAsync(PartitionContext context, CloseReason reason)
        {
            ServiceEventSource.Current.Message("SAMPLE IEventProcessor.CloseAsync for {0} reason {1}", context.PartitionId, reason);
            return Task.CompletedTask;
        }

        public override async Task ProcessEventsAsync(CancellationToken cancellationToken, PartitionContext context, IEnumerable<EventData> events)
        {
            int count = 0;
            EventData last = null;
            foreach (EventData e in events)
            {
                count++;
                this.total++;
                last = e;
                if ((this.total % this.checkpointInterval) == 0)
                {
                    ServiceEventSource.Current.Message("SAMPLE IEventProcessor.ProcessEventsAsync for {0} got {1} total {2} last sequence number {3} lastbody {4}",
                        context.PartitionId, count, this.total, last.SystemProperties.SequenceNumber, System.Text.Encoding.UTF8.GetString(last.Body.ToArray()));
                    await context.CheckpointAsync();
                }
            }
        }

        public override Task ProcessErrorAsync(PartitionContext context, Exception error)
        {
            ServiceEventSource.Current.Message("SAMPLE IEventProcessor.ProcessErrorAsync for {0} error {1}", context.PartitionId, error);
            return Task.CompletedTask;
        }
    }
}
