namespace Microsoft.Azure.EventHubs.Processor.UnitTests
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Threading.Tasks;
    using Xunit;
    using Xunit.Abstractions;

    public class EventProcessorHostTests
    {
        public EventProcessorHostTests()
        {
            string eventHubConnectionString = Environment.GetEnvironmentVariable("EVENTHUBCONNECTIONSTRING");
            if (string.IsNullOrWhiteSpace(eventHubConnectionString))
            {
                throw new InvalidOperationException("EVENTHUBCONNECTIONSTRING environment variable was not found!");
            }

            string storageConnectionString = Environment.GetEnvironmentVariable("EVENTPROCESSORSTORAGECONNECTIONSTRING");
            if (string.IsNullOrWhiteSpace(eventHubConnectionString))
            {
                throw new InvalidOperationException("EVENTPROCESSORSTORAGECONNECTIONSTRING environment variable was not found!");
            }

            this.ConnectionSettings = new ServiceBusConnectionSettings(eventHubConnectionString);
            this.StorageConnectionString = storageConnectionString;
        }

        ServiceBusConnectionSettings ConnectionSettings { get; }

        string StorageConnectionString { get; }

        [Fact]
        async Task RegisterAsync()
        {
            WriteLine($"{DateTime.Now.TimeOfDay} Testing EventProcessorHost");
            var eventProcessorHost = new EventProcessorHost(
                this.ConnectionSettings.Endpoint.Host.Split('.')[0],
                this.ConnectionSettings.EntityPath,
                this.ConnectionSettings.SasKeyName,
                this.ConnectionSettings.SasKey,
                PartitionReceiver.DefaultConsumerGroupName,
                this.StorageConnectionString);

            WriteLine($"{DateTime.Now.TimeOfDay} Calling RegisterEventProcessorAsync");
            var processorOptions = new EventProcessorOptions { ReceiveTimeout = TimeSpan.FromSeconds(30) };
            var processorFactory = new TestEventProcessorFactory();
            processorFactory.OnCreateProcessor += (f, createArgs) =>
            {
                var processor = createArgs.Item2;
                processor.OnOpen += (_, partitionContext) => WriteLine($"{DateTime.Now.TimeOfDay} {partitionContext} TestEventProcessor opened");
                processor.OnClose += (_, closeArgs) => WriteLine($"{DateTime.Now.TimeOfDay} {closeArgs.Item1} TestEventProcessor closing");
                processor.OnProcessError += (_, errorArgs) => WriteLine($"{DateTime.Now.TimeOfDay} {errorArgs.Item1} TestEventProcessor process error {errorArgs.Item2.Message}");
                processor.OnProcessEvents += (_, eventsArgs) => WriteLine($"{DateTime.Now.TimeOfDay} {eventsArgs.Item1} TestEventProcessor process events {eventsArgs.Item2?.Count()}");
            };

            await eventProcessorHost.RegisterEventProcessorFactoryAsync(processorFactory, processorOptions);

            WriteLine($"{DateTime.Now.TimeOfDay} Waiting for events...");
            await Task.Delay(TimeSpan.FromSeconds(20));

            WriteLine($"{DateTime.Now.TimeOfDay} Calling UnregisterEventProcessorAsync");
            await eventProcessorHost.UnregisterEventProcessorAsync();
        }

        [Fact]
        async Task RegisterTwoProcessorHostsAsync()
        {
            WriteLine($"{DateTime.Now.TimeOfDay} Testing with 2 EventProcessorHost instances");
            int hostCount = 2;
            var hosts = new List<EventProcessorHost>();
            for (int i = 0; i < hostCount; i++)
            {
                int index = i;
                WriteLine($"{DateTime.Now.TimeOfDay} Host{index} Creating EventProcessorHost");
                var eventProcessorHost = new EventProcessorHost(
                    this.ConnectionSettings.Endpoint.Host.Split('.')[0],
                    this.ConnectionSettings.EntityPath,
                    this.ConnectionSettings.SasKeyName,
                    this.ConnectionSettings.SasKey,
                    PartitionReceiver.DefaultConsumerGroupName,
                    this.StorageConnectionString);
                hosts.Add(eventProcessorHost);
                WriteLine($"{DateTime.Now.TimeOfDay} Host{index} Calling RegisterEventProcessorAsync");
                var processorOptions = new EventProcessorOptions { ReceiveTimeout = TimeSpan.FromSeconds(30) };
                var processorFactory = new TestEventProcessorFactory();
                processorFactory.OnCreateProcessor += (f, createArgs) =>
                {
                    var processor = createArgs.Item2;
                    processor.OnOpen += (_, partitionContext) => WriteLine($"{DateTime.Now.TimeOfDay} Host{index} {partitionContext} TestEventProcessor opened");
                    processor.OnClose += (_, closeArgs) => WriteLine($"{DateTime.Now.TimeOfDay} Host{index} {closeArgs.Item1} TestEventProcessor closing");
                    processor.OnProcessError += (_, errorArgs) => WriteLine($"{DateTime.Now.TimeOfDay} Host{index} {errorArgs.Item1} TestEventProcessor process error {errorArgs.Item2.Message}");
                    processor.OnProcessEvents += (_, eventsArgs) => WriteLine($"{DateTime.Now.TimeOfDay} Host{index} {eventsArgs.Item1} TestEventProcessor process events {eventsArgs.Item2?.Count()}");
                };

                await eventProcessorHost.RegisterEventProcessorFactoryAsync(processorFactory, processorOptions);
            }

            WriteLine($"{DateTime.Now.TimeOfDay} Waiting for events...");
            await Task.Delay(TimeSpan.FromSeconds(60));

            var shutdownTasks = new List<Task>();
            for (int i = 0; i < hostCount; i++)
            {
                WriteLine($"{DateTime.Now.TimeOfDay} Host{i} Calling UnregisterEventProcessorAsync.");
                shutdownTasks.Add(hosts[i].UnregisterEventProcessorAsync());
            }

            await Task.WhenAll(shutdownTasks);
        }

        static void WriteLine(string message)
        {
            // Currently xunit2 for .net core doesn't seem to have any output mechanism.  If we find one, replace these here:
            Debug.WriteLine(message);
            Console.WriteLine(message);
        }

        class TestEventProcessor : IEventProcessor
        {
            public event EventHandler<PartitionContext> OnOpen;
            public event EventHandler<Tuple<PartitionContext, CloseReason>> OnClose;
            public event EventHandler<Tuple<PartitionContext, IEnumerable<EventData>>> OnProcessEvents;
            public event EventHandler<Tuple<PartitionContext, Exception>> OnProcessError;

            public TestEventProcessor()
            {
            }

            Task IEventProcessor.CloseAsync(PartitionContext context, CloseReason reason)
            {
                this.OnClose?.Invoke(this, new Tuple<PartitionContext, CloseReason>(context, reason));
                return Task.CompletedTask;
            }

            Task IEventProcessor.ProcessErrorAsync(PartitionContext context, Exception error)
            {
                this.OnProcessError?.Invoke(this, new Tuple<PartitionContext, Exception>(context, error));
                return Task.CompletedTask;
            }

            Task IEventProcessor.ProcessEventsAsync(PartitionContext context, IEnumerable<EventData> events)
            {
                this.OnProcessEvents?.Invoke(this, new Tuple<PartitionContext, IEnumerable<EventData>>(context, events));
                EventData lastEvent = events?.LastOrDefault();
                if (lastEvent != null)
                {
                    return context.CheckpointAsync(lastEvent);
                }

                return Task.CompletedTask;
            }

            Task IEventProcessor.OpenAsync(PartitionContext context)
            {
                this.OnOpen?.Invoke(this, context);
                return Task.CompletedTask;
            }
        }

        class TestEventProcessorFactory : IEventProcessorFactory
        {
            public event EventHandler<Tuple<PartitionContext, TestEventProcessor>> OnCreateProcessor;

            IEventProcessor IEventProcessorFactory.CreateEventProcessor(PartitionContext context)
            {
                var processor = new TestEventProcessor();
                this.OnCreateProcessor?.Invoke(this, new Tuple<PartitionContext, TestEventProcessor>(context, processor));
                return processor;
            }
        }
    }
}
