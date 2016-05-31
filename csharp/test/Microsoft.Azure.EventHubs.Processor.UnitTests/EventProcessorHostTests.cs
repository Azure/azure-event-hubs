namespace Microsoft.Azure.EventHubs.Processor.UnitTests
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Text;
    using System.Threading;
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
        async Task SingleProcessorHost()
        {
            WriteLine($"Testing with single EventProcessorHost");
            string[] partitionIds = await this.GetPartitionIdsAsync(this.ConnectionSettings);
            WriteLine($"EventHub has {partitionIds.Length} partitions");
            var eventProcessorHost = new EventProcessorHost(
                this.ConnectionSettings.Endpoint.Host.Split('.')[0],
                this.ConnectionSettings.EntityPath,
                this.ConnectionSettings.SasKeyName,
                this.ConnectionSettings.SasKey,
                PartitionReceiver.DefaultConsumerGroupName,
                this.StorageConnectionString);
            try
            {
                WriteLine($"Calling RegisterEventProcessorAsync");
                var processorOptions = new EventProcessorOptions { ReceiveTimeout = TimeSpan.FromSeconds(10) };
                var processorFactory = new TestEventProcessorFactory();

                var partitionReceiveEvents = new ConcurrentDictionary<string, AsyncAutoResetEvent>();
                foreach (var partitionId in partitionIds)
                {
                    partitionReceiveEvents[partitionId] = new AsyncAutoResetEvent(false);
                }

                processorFactory.OnCreateProcessor += (f, createArgs) =>
                {
                    var processor = createArgs.Item2;
                    string partitionId = createArgs.Item1.PartitionId;
                    processor.OnOpen += (_, partitionContext) => WriteLine($"Partition {partitionId} TestEventProcessor opened");
                    processor.OnClose += (_, closeArgs) => WriteLine($"Partition {partitionId} TestEventProcessor closing: {closeArgs.Item2}");
                    processor.OnProcessError += (_, errorArgs) => WriteLine($"Partition {partitionId} TestEventProcessor process error {errorArgs.Item2.Message}");
                    processor.OnProcessEvents += (_, eventsArgs) =>
                    {
                        int eventCount = eventsArgs.Item2 != null ? eventsArgs.Item2.Count() : 0;
                        WriteLine($"Partition {partitionId} TestEventProcessor processing {eventCount} event(s)");
                        if (eventCount > 0)
                        {
                            var receivedEvent = partitionReceiveEvents[partitionId];
                            receivedEvent.Set();
                        }
                    };
                };

                await eventProcessorHost.RegisterEventProcessorFactoryAsync(processorFactory, processorOptions);

                WriteLine($"Sending an event to each partition");
                var sendTasks = new List<Task>();
                foreach (var partitionId in partitionIds)
                {
                    sendTasks.Add(this.SendToPartitionAsync(partitionId, $"{partitionId} event.", this.ConnectionSettings));
                }

                await Task.WhenAll(sendTasks);

                WriteLine($"Verifying an event was received by each partition");
                foreach (var partitionId in partitionIds)
                {
                    var receivedEvent = partitionReceiveEvents[partitionId];
                    bool partitionReceivedMessage = await receivedEvent.WaitAsync(TimeSpan.FromSeconds(30));
                    Assert.True(partitionReceivedMessage, $"Partition {partitionId} didn't receive any message!");
                }

                WriteLine($"Success");
            }
            finally
            {
                WriteLine($"Calling UnregisterEventProcessorAsync");
                await eventProcessorHost.UnregisterEventProcessorAsync();
            }
        }

        [Fact]
        async Task MultipleProcessorHosts()
        {
            WriteLine($"Testing with 2 EventProcessorHost instances");
            string[] partitionIds = await this.GetPartitionIdsAsync(this.ConnectionSettings);
            WriteLine($"EventHub has {partitionIds.Length} partitions");

            int hostCount = 2;
            var hosts = new List<EventProcessorHost>();
            for (int i = 0; i < hostCount; i++)
            {
                int index = i;
                WriteLine($"Host{index} Creating EventProcessorHost");
                var eventProcessorHost = new EventProcessorHost(
                    this.ConnectionSettings.Endpoint.Host.Split('.')[0],
                    this.ConnectionSettings.EntityPath,
                    this.ConnectionSettings.SasKeyName,
                    this.ConnectionSettings.SasKey,
                    PartitionReceiver.DefaultConsumerGroupName,
                    this.StorageConnectionString);
                hosts.Add(eventProcessorHost);
                WriteLine($"Host{index} Calling RegisterEventProcessorAsync");
                var processorOptions = new EventProcessorOptions
                {
                    ReceiveTimeout = TimeSpan.FromSeconds(10),
                    InvokeProcessorAfterReceiveTimeout = true
                };

                var processorFactory = new TestEventProcessorFactory();
                processorFactory.OnCreateProcessor += (f, createArgs) =>
                {
                    var processor = createArgs.Item2;
                    string partitionId = "Partition " + createArgs.Item1.PartitionId;
                    processor.OnOpen += (_, partitionContext) => WriteLine($"Host {index} {partitionId} TestEventProcessor opened");
                    processor.OnClose += (_, closeArgs) => WriteLine($"Host {index} {partitionId} TestEventProcessor closing");
                    processor.OnProcessError += (_, errorArgs) => WriteLine($"Host {index} {partitionId} TestEventProcessor process error {errorArgs.Item2.Message}");
                    processor.OnProcessEvents += (_, eventsArgs) => WriteLine($"Host {index} {partitionId} TestEventProcessor process events " + (eventsArgs.Item2 != null ? eventsArgs.Item2.Count() : 0));
                };

                await eventProcessorHost.RegisterEventProcessorFactoryAsync(processorFactory, processorOptions);
            }

            WriteLine($"Waiting for events...");
            await Task.Delay(TimeSpan.FromSeconds(30));

            var shutdownTasks = new List<Task>();
            for (int i = 0; i < hostCount; i++)
            {
                WriteLine($"Host{i} Calling UnregisterEventProcessorAsync.");
                shutdownTasks.Add(hosts[i].UnregisterEventProcessorAsync());
            }

            await Task.WhenAll(shutdownTasks);
        }

        [Fact]
        async Task InvokeAfterReceiveTimeoutTrue()
        {
            WriteLine($"Testing EventProcessorHost with InvokeProcessorAfterReceiveTimeout=true");

            string[] partitionIds = await this.GetPartitionIdsAsync(this.ConnectionSettings);
            WriteLine($"EventHub has {partitionIds.Length} partitions");

            var emptyBatchReceiveEvents = new ConcurrentDictionary<string, AsyncAutoResetEvent>();
            foreach (var partitionId in partitionIds)
            {
                emptyBatchReceiveEvents[partitionId] = new AsyncAutoResetEvent(false);
            }

            var eventProcessorHost = new EventProcessorHost(
                this.ConnectionSettings.Endpoint.Host.Split('.')[0],
                this.ConnectionSettings.EntityPath,
                this.ConnectionSettings.SasKeyName,
                this.ConnectionSettings.SasKey,
                PartitionReceiver.DefaultConsumerGroupName,
                this.StorageConnectionString);

            var processorOptions = new EventProcessorOptions {
                ReceiveTimeout = TimeSpan.FromSeconds(5),
                InvokeProcessorAfterReceiveTimeout = true
            };

            var processorFactory = new TestEventProcessorFactory();
            processorFactory.OnCreateProcessor += (f, createArgs) =>
            {
                var processor = createArgs.Item2;
                string partitionId = createArgs.Item1.PartitionId;
                processor.OnOpen += (_, partitionContext) => WriteLine($"Partition {partitionId} TestEventProcessor opened");
                processor.OnProcessEvents += (_, eventsArgs) =>
                {
                    int eventCount = eventsArgs.Item2 != null ? eventsArgs.Item2.Count() : 0;
                    WriteLine($"Partition {partitionId} TestEventProcessor processing {eventCount} event(s)");
                    if (eventCount == 0)
                    {
                        var emptyBatchReceiveEvent = emptyBatchReceiveEvents[partitionId];
                        emptyBatchReceiveEvent.Set();
                    }
                };
            };

            await eventProcessorHost.RegisterEventProcessorFactoryAsync(processorFactory, processorOptions);
            try
            {
                WriteLine($"Waiting for each partition to receive an empty batch of events...");
                foreach (var partitionId in partitionIds)
                {
                    var emptyBatchReceiveEvent = emptyBatchReceiveEvents[partitionId];
                    bool emptyBatchReceived = await emptyBatchReceiveEvent.WaitAsync(TimeSpan.FromSeconds(30));
                    Assert.True(emptyBatchReceived, $"Partition {partitionId} didn't receive an empty batch!");
                }
            }
            finally
            {
                WriteLine($"Calling UnregisterEventProcessorAsync");
                await eventProcessorHost.UnregisterEventProcessorAsync();
            }
        }

        [Fact]
        async Task InvokeAfterReceiveTimeoutFalse()
        {
            WriteLine($"Calling RegisterEventProcessorAsync with InvokeProcessorAfterReceiveTimeout=false");
            var eventProcessorHost = new EventProcessorHost(
                this.ConnectionSettings.Endpoint.Host.Split('.')[0],
                this.ConnectionSettings.EntityPath,
                this.ConnectionSettings.SasKeyName,
                this.ConnectionSettings.SasKey,
                PartitionReceiver.DefaultConsumerGroupName,
                this.StorageConnectionString);

            var processorOptions = new EventProcessorOptions
            {
                ReceiveTimeout = TimeSpan.FromSeconds(5),
                InvokeProcessorAfterReceiveTimeout = false
            };

            var emptyBatchReceiveEvent = new AsyncAutoResetEvent(false);
            var processorFactory = new TestEventProcessorFactory();
            processorFactory.OnCreateProcessor += (f, createArgs) =>
            {
                var processor = createArgs.Item2;
                string partitionId = createArgs.Item1.PartitionId;
                processor.OnProcessEvents += (_, eventsArgs) =>
                {
                    int eventCount = eventsArgs.Item2 != null ? eventsArgs.Item2.Count() : 0;
                    WriteLine($"Partition {partitionId} TestEventProcessor processing {eventCount} event(s)");
                    if (eventCount == 0)
                    {
                        emptyBatchReceiveEvent.Set();
                    }
                };
            };

            await eventProcessorHost.RegisterEventProcessorFactoryAsync(processorFactory, processorOptions);
            try
            {
                WriteLine($"Verifying no empty batches arrive...");
                bool waitSucceeded = await emptyBatchReceiveEvent.WaitAsync(TimeSpan.FromSeconds(10));
                Assert.False(waitSucceeded, "No empty batch should have been received!");
            }
            finally
            {
                WriteLine($"Calling UnregisterEventProcessorAsync");
                await eventProcessorHost.UnregisterEventProcessorAsync();
            }
        }

        async Task<string[]> GetPartitionIdsAsync(ServiceBusConnectionSettings connectionSettings)
        {
            var eventHubClient = EventHubClient.Create(connectionSettings);
            try
            {
                var eventHubInfo = await eventHubClient.GetRuntimeInformationAsync();
                return eventHubInfo.PartitionIds;
            }
            finally
            {
                await eventHubClient.CloseAsync();
            }
        }

        async Task SendToPartitionAsync(string partitionId, string messageBody, ServiceBusConnectionSettings connectionSettings)
        {
            var eventHubClient = EventHubClient.Create(connectionSettings);
            try
            {
                var partitionSender = eventHubClient.CreatePartitionSender(partitionId);
                await partitionSender.SendAsync(new EventData(Encoding.UTF8.GetBytes(messageBody)));
            }
            finally
            {
                await eventHubClient.CloseAsync();
            }
        }


        static void WriteLine(string message)
        {
            // Currently xunit2 for .net core doesn't seem to have any output mechanism.  If we find one, replace these here:
            message = DateTime.Now.TimeOfDay + " " + message;
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
