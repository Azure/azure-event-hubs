using System;
using System.Collections.Generic;
using System.Fabric;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.ServiceFabric.Services.Communication.Runtime;
using Microsoft.ServiceFabric.Services.Runtime;
using Microsoft.Azure.EventHubs.ServiceFabricProcessor;

namespace Stateful1
{
    /// <summary>
    /// An instance of this class is created for each service replica by the Service Fabric runtime.
    /// </summary>
    internal sealed class Stateful1 : StatefulService
    {
        bool keepgoing = true;

        public Stateful1(StatefulServiceContext context)
            : base(context)
        {
        }

        protected override async Task RunAsync(CancellationToken cancellationToken)
        {
            EventProcessorOptions options = new EventProcessorOptions();
            options.OnShutdown = OnShutdown;
            string eventHubConnectionString = "------------------------------EVENT HUB CONNECTION STRING GOES HERE ----------------------------";
            ServiceFabricProcessor processorService = new ServiceFabricProcessor(this.Context.ServiceName, this.Context.PartitionId, this.StateManager, this.Partition,
                new SampleEventProcessor(), eventHubConnectionString, "$Default", options);

            Task processing = processorService.RunAsync(cancellationToken);
            // If there is nothing else to do, application can simply await on the task here instead of polling keepgoing
            while (this.keepgoing)
            {
                // Do other stuff here
                Thread.Sleep(1000);
            }
            await processing;
            // The await may throw if there was an error.
        }

        private void OnShutdown(Exception e)
        {
            ServiceEventSource.Current.Message("SAMPLE OnShutdown got {0}", (e == null) ? "NO ERROR" : e.ToString());
            this.keepgoing = false;
        }
    }
}
