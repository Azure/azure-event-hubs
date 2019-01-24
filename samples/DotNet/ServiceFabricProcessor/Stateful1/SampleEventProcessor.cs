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
        private Stopwatch stopwatch = null;
        private readonly long checkpointInterval = 10000L;

        override public Task OpenAsync(CancellationToken cancellationToken, PartitionContext context)
        {
            ServiceEventSource.Current.Message("DUMMY IEventProcessor.OpenAsync for {0}", context.PartitionId);
            return Task.CompletedTask;
        }

        public override Task CloseAsync(PartitionContext context, CloseReason reason)
        {
            ServiceEventSource.Current.Message("DUMMY IEventProcessor.CloseAsync for {0} reason {1}", context.PartitionId, reason);
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
                    ServiceEventSource.Current.Message("DUMMY IEventProcessor.ProcessEventsAsync for {0} got {1} total {2} lastseq {3} lastbody {4}",
                        context.PartitionId, count, this.total, last.SystemProperties.SequenceNumber, System.Text.Encoding.UTF8.GetString(last.Body.ToArray()));
                    await context.CheckpointAsync();
                    if (this.stopwatch == null)
                    {
                        this.stopwatch = new Stopwatch();
                        this.stopwatch.Start();
                    }
                    else
                    {
                        this.stopwatch.Stop();
                        ServiceEventSource.Current.Message("DUMMY IEventProcessor.ProcessEventsAsync for {0} messages per second {1}", context.PartitionId,
                            (this.checkpointInterval * 1000L / this.stopwatch.ElapsedMilliseconds));
                        this.stopwatch.Reset();
                        this.stopwatch.Start();
                    }
                }
            }
        }

        public override Task ProcessErrorAsync(PartitionContext context, Exception error)
        {
            ServiceEventSource.Current.Message("DUMMY IEventProcessor.ProcessErrorAsync for {0} error {1}", context.PartitionId, error);
            return Task.CompletedTask;
        }
    }
}
