namespace Microsoft.Azure.EventHubs.Processor.UnitTests
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;

    public class EventProcessorHostTests
    {
        public EventProcessorHostTests(string eventHubConnectionString, string storageConnectionString)
        {
            this.ConnectionSettings = new ServiceBusConnectionSettings(eventHubConnectionString);
            this.StorageConnectionString = storageConnectionString;
        }

        ServiceBusConnectionSettings ConnectionSettings { get; }

        string StorageConnectionString { get; }

        public static async Task RunAsync(string connectionString, string storageConnectionString)
        {
            var eventProcessorHostTests = new EventProcessorHostTests(connectionString, storageConnectionString);
            await TestRunner.RunAsync(() => eventProcessorHostTests.RegisterAsync());
        }

        async Task RegisterAsync()
        {
            Console.WriteLine(DateTime.Now.TimeOfDay + " Testing EventProcessorHost.");
            var eventProcessorHost = new EventProcessorHost(
                this.ConnectionSettings.Endpoint.Host.Split('.')[0],
                this.ConnectionSettings.EntityPath,
                this.ConnectionSettings.SasKeyName,
                this.ConnectionSettings.SasKey,
                PartitionReceiver.DefaultConsumerGroupName,
                this.StorageConnectionString);

            var registerTask = eventProcessorHost.RegisterEventProcessorAsync<TestEventProcessor>();

            await Task.Delay(TimeSpan.FromMinutes(1));

            await eventProcessorHost.UnregisterEventProcessorAsync();
        }

        class TestEventProcessor : IEventProcessor
        {
            public TestEventProcessor()
            {
                Console.WriteLine("TestEventProcessor..ctor called");
            }

            Task IEventProcessor.CloseAsync(PartitionContext context, CloseReason reason)
            {
                Console.WriteLine("TestEventProcessor.CloseAsync({0}, {1})", context, reason);
                return Task.CompletedTask;
            }

            Task IEventProcessor.ProcessErrorAsync(PartitionContext context, Exception error)
            {
                Console.WriteLine("TestEventProcessor.OnError({0}, {1}: {2})", context, error.GetType().Name, error.Message);
                return Task.CompletedTask;
            }

            Task IEventProcessor.ProcessEventsAsync(PartitionContext context, IEnumerable<EventData> messages)
            {
                throw new NotImplementedException();
            }

            Task IEventProcessor.OpenAsync(PartitionContext context)
            {
                Console.WriteLine("TestEventProcessor.OpenAsync({0})", context);
                return Task.CompletedTask;
            }
        }
    }
}
