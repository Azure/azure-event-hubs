﻿namespace Microsoft.Azure.EventHubs.UnitTests
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    public class EventHubClientTests
    {
        public EventHubClientTests(string connectionString)
        {
            this.EventHubClient = EventHubClient.Create(connectionString);
        }

        EventHubClient EventHubClient { get; }

        public static async Task RunAsync(string connectionString)
        {
            var eventHubClientTests = new EventHubClientTests(connectionString);
            await TestRunner.RunAsync(() => eventHubClientTests.SendAsync());
            await TestRunner.RunAsync(() => eventHubClientTests.SendBatchAsync());
            await TestRunner.RunAsync(() => eventHubClientTests.PartitionSenderSendAsync());
            await TestRunner.RunAsync(() => eventHubClientTests.PartitionReceiverReceiveAsync());
            //await TestRunner.RunAsync(() => eventHubClientTests.PartitionReceiverEpochReceiveAsync());
            await TestRunner.RunAsync(() => eventHubClientTests.PartitionReceiverSetReceiveHandlerAsync());
            await TestRunner.RunAsync(() => eventHubClientTests.GetEventHubRuntimeInformationAsync());
        }

        async Task SendAsync()
        {
            Console.WriteLine(DateTime.Now.TimeOfDay + " Sending single Event via EventHubClient.SendAsync(EventData, string)");
            var eventData = new EventData(Encoding.UTF8.GetBytes("Hello EventHub by partitionKey!"));
            await this.EventHubClient.SendAsync(eventData, "SomePartitionKeyHere");
        }

        async Task SendBatchAsync()
        {
            Console.WriteLine(DateTime.Now.TimeOfDay + " Sending multiple Events via EventHubClient.SendAsync(IEnumerable<EventData>)");
            var eventData1 = new EventData(Encoding.UTF8.GetBytes("Hello EventHub!"));
            var eventData2 = new EventData(Encoding.UTF8.GetBytes("This is another message in the batch!"));
            eventData2.Properties = new Dictionary<string, object> { ["ContosoEventType"] = "some value here" };
            await this.EventHubClient.SendAsync(new[] { eventData1, eventData2 });
        }

        async Task PartitionSenderSendAsync()
        {
            Console.WriteLine(DateTime.Now.TimeOfDay + " Sending single Event via PartitionSender.SendAsync(EventData)");
            PartitionSender partitionSender1 = this.EventHubClient.CreatePartitionSender("1");
            try
            {
                var eventData = new EventData(Encoding.UTF8.GetBytes("Hello again EventHub Partition 1!"));
                await partitionSender1.SendAsync(eventData);
            }
            finally
            {
                await partitionSender1.CloseAsync();
            }
        }

        async Task PartitionReceiverReceiveAsync()
        {
            Console.WriteLine(DateTime.Now.TimeOfDay + " Receiving Events via PartitionReceiver.ReceiveAsync");
            TimeSpan originalTimeout = this.EventHubClient.ConnectionSettings.OperationTimeout;
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(3);
            PartitionReceiver partitionReceiver1 = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, "1", DateTime.UtcNow.AddHours(-2));
            try
            {
                while (true)
                {
                    IEnumerable<EventData> partition1Events = await partitionReceiver1.ReceiveAsync();
                    if (partition1Events == null)
                    {
                        break;
                    }

                    Console.WriteLine($"Receive a batch of {partition1Events.Count()} events:");
                    foreach (var eventData in partition1Events)
                    {
                        ArraySegment<byte> body = eventData.Body;
                        Console.WriteLine($"Received event '{Encoding.UTF8.GetString(body.Array, body.Offset, body.Count)}' {eventData.SystemProperties.EnqueuedTimeUtc}");
                    }
                }
            }
            finally
            {
                await partitionReceiver1.CloseAsync();
                this.EventHubClient.ConnectionSettings.OperationTimeout = originalTimeout;
            }
        }
        async Task PartitionReceiverEpochReceiveAsync()
        {
            Console.WriteLine($"{DateTime.Now.TimeOfDay} Testing EpochReceiver semantics");
            TimeSpan originalTimeout = this.EventHubClient.ConnectionSettings.OperationTimeout;
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(15);
            var epochReceiver1 = this.EventHubClient.CreateEpochReceiver(PartitionReceiver.DefaultConsumerGroupName, "1", PartitionReceiver.StartOfStream, 1);
            var epochReceiver2 = this.EventHubClient.CreateEpochReceiver(PartitionReceiver.DefaultConsumerGroupName, "1", PartitionReceiver.StartOfStream, 2);
            try
            {
                // Read the events from Epoch 1 Receiver until we're at the end of the stream
                IEnumerable<EventData> events;
                do
                {
                    events = await epochReceiver1.ReceiveAsync();
                    int count = events != null ? events.Count() : 0;
                }
                while (events != null);

                Console.WriteLine($"{DateTime.Now.TimeOfDay} Start up epoch 2 receiver");
                var epoch2ReceiveTask = epochReceiver2.ReceiveAsync();

                DateTime stopTime = DateTime.UtcNow.AddSeconds(30);
                do
                {
                    events = await epochReceiver1.ReceiveAsync();
                    int count = events != null ? events.Count() : 0;
                    Console.WriteLine($"{DateTime.Now.TimeOfDay} epoch 1 receiver got {count} event(s)");
                }
                while (DateTime.UtcNow < stopTime);

                throw new InvalidOperationException("Epoch 1 receiver should have encountered an exception by now!");
            }
            catch(ReceiverDisconnectedException disconnectedException)
            {
                Console.WriteLine($"{DateTime.Now.TimeOfDay} Received expected exception {disconnectedException.GetType()}: {disconnectedException.Message}");

                try
                {
                    await epochReceiver1.ReceiveAsync();
                    throw new InvalidOperationException("Epoch 1 receiver should throw ReceiverDisconnectedException here too!");
                }
                catch (ReceiverDisconnectedException e)
                {
                    Console.WriteLine($"{DateTime.Now.TimeOfDay} Received expected exception {e.GetType()}");
                }
            }
            finally
            {
                await epochReceiver1?.CloseAsync();
                await epochReceiver2?.CloseAsync();
                this.EventHubClient.ConnectionSettings.OperationTimeout = originalTimeout;
            }
        }

        async Task PartitionReceiverSetReceiveHandlerAsync()
        {
            Console.WriteLine(DateTime.Now.TimeOfDay + " Receiving Events via PartitionReceiver.SetReceiveHandler()");
            TimeSpan originalTimeout = this.EventHubClient.ConnectionSettings.OperationTimeout;
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(3);
            PartitionReceiver partitionReceiver1 = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, "1", DateTime.UtcNow.AddHours(-2));
            try
            {
                EventWaitHandle dataReceivedEvent = new EventWaitHandle(false, EventResetMode.ManualReset);
                var handler = new TestPartitionReceiveHandler();
                handler.EventsReceived += (s, e) =>
                {
                    Console.WriteLine("Receive a batch of {0} events:", e != null ? e.Count() : 0);
                    if (e != null)
                    {
                        foreach (var eventData in e)
                        {
                            ArraySegment<byte> body = eventData.Body;
                            Console.WriteLine($"Received event '{Encoding.UTF8.GetString(body.Array, body.Offset, body.Count)}' {eventData.SystemProperties.EnqueuedTimeUtc}");
                        }
                    }

                    dataReceivedEvent.Set();
                };

                EventWaitHandle handlerClosedEvent = new EventWaitHandle(false, EventResetMode.ManualReset);
                handler.Closed += (s, error) =>
                {
                    Console.WriteLine($"IPartitionReceiveHandler.CloseAsync called.");
                    handlerClosedEvent.Set();
                };

                partitionReceiver1.SetReceiveHandler(handler);

                if (!dataReceivedEvent.WaitOne(TimeSpan.FromSeconds(20)))
                {
                    throw new InvalidOperationException("Data Received Event was not signalled.");
                }

                Console.WriteLine("Closing PartitionReceiver");
                await partitionReceiver1.CloseAsync();
                if (!handlerClosedEvent.WaitOne(TimeSpan.FromSeconds(20)))
                {
                    throw new InvalidOperationException("Handle Closed Event was not signalled.");
                }
            }
            catch (Exception)
            {
                await partitionReceiver1.CloseAsync();
                throw;
            }
            finally
            {
                this.EventHubClient.ConnectionSettings.OperationTimeout = originalTimeout;
            }
        }

        async Task GetEventHubRuntimeInformationAsync()
        {
            Console.WriteLine(DateTime.Now.TimeOfDay + " Getting  EventHubRuntimeInformation");
            var eventHubRuntimeInformation = await this.EventHubClient.GetRuntimeInformationAsync();

            if (eventHubRuntimeInformation == null || eventHubRuntimeInformation.PartitionIds == null || eventHubRuntimeInformation.PartitionIds.Length == 0)
            {
                throw new InvalidOperationException("Failed to get partition ids!");
            }

            Console.WriteLine("Found partitions:");
            foreach (string partitionId in eventHubRuntimeInformation.PartitionIds)
            {
                Console.WriteLine(partitionId);
            }
        }

        class TestPartitionReceiveHandler : IPartitionReceiveHandler
        {
            public event EventHandler<IEnumerable<EventData>> EventsReceived;

            public event EventHandler<Exception> ErrorReceived;

            public event EventHandler<Exception> Closed;

            Task IPartitionReceiveHandler.CloseAsync(Exception error)
            {
                this.Closed?.Invoke(this, error);
                return Task.CompletedTask;
            }

            Task IPartitionReceiveHandler.ProcessErrorAsync(Exception error)
            {
                this.ErrorReceived?.Invoke(this, error);
                return Task.CompletedTask;
            }

            Task IPartitionReceiveHandler.ProcessEventsAsync(IEnumerable<EventData> events)
            {
                this.EventsReceived?.Invoke(this, events);
                return Task.CompletedTask;
            }
        }
    }
}
