namespace Microsoft.Azure.EventHubs.UnitTests
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Xunit;

    public class EventHubClientTests
    {
        public EventHubClientTests()
        {
            string connectionString = Environment.GetEnvironmentVariable("EVENTHUBCONNECTIONSTRING");
            if (string.IsNullOrWhiteSpace(connectionString))
            {
                throw new InvalidOperationException("EVENTHUBCONNECTIONSTRING environment variable was not found!");
            }

            this.EventHubClient = EventHubClient.Create(connectionString);
        }

        EventHubClient EventHubClient { get; }

        [Fact]
        async Task EventHubClientSend()
        {
            WriteLine("Sending single Event via EventHubClient.SendAsync(EventData, string)");
            var eventData = new EventData(Encoding.UTF8.GetBytes("Hello EventHub by partitionKey!"));
            await this.EventHubClient.SendAsync(eventData, "SomePartitionKeyHere");
        }

        [Fact]
        async Task EventHubClientSendBatch()
        {
            WriteLine("Sending multiple Events via EventHubClient.SendAsync(IEnumerable<EventData>)");
            var eventData1 = new EventData(Encoding.UTF8.GetBytes("Hello EventHub!"));
            var eventData2 = new EventData(Encoding.UTF8.GetBytes("This is another message in the batch!"));
            eventData2.Properties = new Dictionary<string, object> { ["ContosoEventType"] = "some value here" };
            await this.EventHubClient.SendAsync(new[] { eventData1, eventData2 });
        }

        [Fact]
        async Task PartitionSenderSend()
        {
            WriteLine("Sending single Event via PartitionSender.SendAsync(EventData)");
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

        [Fact]
        async Task PartitionSenderSendBatch()
        {
            WriteLine("Sending single Event via PartitionSender.SendAsync(IEnumerable<EventData>)");
            PartitionSender partitionSender1 = this.EventHubClient.CreatePartitionSender("1");
            try
            {
                var eventData1 = new EventData(Encoding.UTF8.GetBytes("Hello EventHub!"));
                var eventData2 = new EventData(Encoding.UTF8.GetBytes("This is another message in the batch!"));
                eventData2.Properties = new Dictionary<string, object> { ["ContosoEventType"] = "some value here" };
                await partitionSender1.SendAsync(new[] { eventData1, eventData2 });
            }
            finally
            {
                await partitionSender1.CloseAsync();
            }
        }

        [Fact]
        async Task PartitionReceiverReceive()
        {
            WriteLine("Receiving Events via PartitionReceiver.ReceiveAsync");
            TimeSpan originalTimeout = this.EventHubClient.ConnectionSettings.OperationTimeout;
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(5);
            const string partitionId = "1";
            PartitionSender partitionSender = this.EventHubClient.CreatePartitionSender(partitionId);
            PartitionReceiver partitionReceiver = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, partitionId, DateTime.UtcNow.AddMinutes(-10));
            try
            {
                string uniqueEventId = Guid.NewGuid().ToString();
                WriteLine($"Sending an event to Partition {partitionId} with custom property EventId {uniqueEventId}");
                var sendEvent = new EventData(Encoding.UTF8.GetBytes("Hello EventHub!"));
                sendEvent.Properties = new Dictionary<string, object> { ["EventId"] = uniqueEventId };
                await partitionSender.SendAsync(sendEvent);

                bool expectedEventReceived = false;
                do
                {
                    IEnumerable<EventData> eventDatas = await partitionReceiver.ReceiveAsync(10);
                    if (eventDatas == null)
                    {
                        break;
                    }

                    WriteLine($"Received a batch of {eventDatas.Count()} events:");
                    foreach (var eventData in eventDatas)
                    {
                        object objectValue;
                        if (eventData.Properties != null && eventData.Properties.TryGetValue("EventId", out objectValue))
                        {
                            WriteLine($"Received message with EventId {objectValue}");
                            string receivedId = objectValue.ToString();
                            if (receivedId == uniqueEventId)
                            {
                                WriteLine("Success");
                                expectedEventReceived = true;
                                break;
                            }
                        }
                    }
                }
                while (!expectedEventReceived);

                Assert.True(expectedEventReceived, $"Did not receive expected event with EventId {uniqueEventId}");
            }
            finally
            {
                this.EventHubClient.ConnectionSettings.OperationTimeout = originalTimeout;
                await Task.WhenAll(
                    partitionReceiver.CloseAsync(),
                    partitionSender.CloseAsync());
            }
        }

        [Fact]
        async Task PartitionReceiverReceiveBatch()
        {
            const int MaxBatchSize = 5;
            WriteLine("Receiving Events via PartitionReceiver.ReceiveAsync(BatchSize)");
            TimeSpan originalTimeout = this.EventHubClient.ConnectionSettings.OperationTimeout;
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(3);
            const string partitionId = "0";
            PartitionSender partitionSender = this.EventHubClient.CreatePartitionSender(partitionId);
            PartitionReceiver partitionReceiver = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, partitionId, DateTime.UtcNow.AddMinutes(-10));
            try
            {
                int eventCount = 20;
                WriteLine($"Sending {eventCount} events to Partition {partitionId}");
                var sendEvents = new List<EventData>(eventCount);
                for (int i = 0; i < eventCount; i++)
                {
                    sendEvents.Add(new EventData(Encoding.UTF8.GetBytes($"Hello EventHub! Message {i}")));
                }
                await partitionSender.SendAsync(sendEvents);

                int maxReceivedBatchSize = 0;
                while (true)
                {
                    IEnumerable<EventData> partition1Events = await partitionReceiver.ReceiveAsync(MaxBatchSize);
                    int receivedEventCount = partition1Events != null ? partition1Events.Count() : 0;
                    WriteLine($"Received {receivedEventCount} event(s)");

                    if (partition1Events == null)
                    {
                        break;
                    }

                    maxReceivedBatchSize = Math.Max(maxReceivedBatchSize, receivedEventCount);
                }

                Assert.True(maxReceivedBatchSize == MaxBatchSize, $"A max batch size of {MaxBatchSize} events was not honored! Actual {maxReceivedBatchSize}.");
            }
            finally
            {
                this.EventHubClient.ConnectionSettings.OperationTimeout = originalTimeout;
                await Task.WhenAll(
                    partitionReceiver.CloseAsync(),
                    partitionSender.CloseAsync());
            }
        }

        [Fact]
        async Task PartitionReceiverEpochReceive()
        {
            WriteLine("Testing EpochReceiver semantics");
            TimeSpan originalTimeout = this.EventHubClient.ConnectionSettings.OperationTimeout;
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(15);
            var epochReceiver1 = this.EventHubClient.CreateEpochReceiver(PartitionReceiver.DefaultConsumerGroupName, "1", PartitionReceiver.StartOfStream, 1);
            var epochReceiver2 = this.EventHubClient.CreateEpochReceiver(PartitionReceiver.DefaultConsumerGroupName, "1", PartitionReceiver.StartOfStream, 2);
            try
            {
                // Read the events from Epoch 1 Receiver until we're at the end of the stream
                WriteLine("Starting epoch 1 receiver");
                IEnumerable<EventData> events;
                do
                {
                    events = await epochReceiver1.ReceiveAsync(10);
                    int count = events != null ? events.Count() : 0;
                }
                while (events != null);

                WriteLine("Starting epoch 2 receiver");
                var epoch2ReceiveTask = epochReceiver2.ReceiveAsync(10);

                DateTime stopTime = DateTime.UtcNow.AddSeconds(30);
                do
                {
                    events = await epochReceiver1.ReceiveAsync(10);
                    int count = events != null ? events.Count() : 0;
                    WriteLine($"Epoch 1 receiver got {count} event(s)");
                }
                while (DateTime.UtcNow < stopTime);

                throw new InvalidOperationException("Epoch 1 receiver should have encountered an exception by now!");
            }
            catch(ReceiverDisconnectedException disconnectedException)
            {
                WriteLine($"Received expected exception {disconnectedException.GetType()}: {disconnectedException.Message}");

                try
                {
                    await epochReceiver1.ReceiveAsync(10);
                    throw new InvalidOperationException("Epoch 1 receiver should throw ReceiverDisconnectedException here too!");
                }
                catch (ReceiverDisconnectedException e)
                {
                    WriteLine($"Received expected exception {e.GetType()}");
                }
            }
            finally
            {
                await epochReceiver1.CloseAsync();
                await epochReceiver2.CloseAsync();
                this.EventHubClient.ConnectionSettings.OperationTimeout = originalTimeout;
            }
        }

        [Fact]
        async Task PartitionReceiverSetReceiveHandler()
        {
            WriteLine("Receiving Events via PartitionReceiver.SetReceiveHandler()");
            TimeSpan originalTimeout = this.EventHubClient.ConnectionSettings.OperationTimeout;
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(3);
            string partitionId = "1";
            PartitionReceiver partitionReceiver1 = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, partitionId, DateTime.UtcNow.AddMinutes(-10));
            PartitionSender partitionSender = this.EventHubClient.CreatePartitionSender(partitionId);
            try
            {
                string uniqueEventId = Guid.NewGuid().ToString();
                WriteLine($"Sending an event to Partition {partitionId} with custom property EventId {uniqueEventId}");
                var sendEvent = new EventData(Encoding.UTF8.GetBytes("Hello EventHub!"));
                sendEvent.Properties = new Dictionary<string, object> { ["EventId"] = uniqueEventId };
                await partitionSender.SendAsync(sendEvent);

                EventWaitHandle dataReceivedEvent = new EventWaitHandle(false, EventResetMode.ManualReset);
                EventWaitHandle handlerClosedEvent = new EventWaitHandle(false, EventResetMode.ManualReset);
                var handler = new TestPartitionReceiveHandler();
                handler.ErrorReceived += (s, e) => WriteLine($"TestPartitionReceiveHandler.ProcessError {e.GetType().Name}: {e.Message}");
                handler.EventsReceived += (s, eventDatas) =>
                {
                    int count = eventDatas != null ? eventDatas.Count() : 0;
                    WriteLine($"Received {count} event(s):");

                    foreach (var eventData in eventDatas)
                    {
                        object objectValue;
                        if (eventData.Properties != null && eventData.Properties.TryGetValue("EventId", out objectValue))
                        {
                            WriteLine($"Received message with EventId {objectValue}");
                            string receivedId = objectValue.ToString();
                            if (receivedId == uniqueEventId)
                            {
                                WriteLine("Success");
                                dataReceivedEvent.Set();
                                break;
                            }
                        }
                    }
                };

                handler.Closed += (s, error) =>
                {
                    WriteLine($"IPartitionReceiveHandler.CloseAsync called.");
                    handlerClosedEvent.Set();
                };

                partitionReceiver1.SetReceiveHandler(handler);

                if (!dataReceivedEvent.WaitOne(TimeSpan.FromSeconds(20)))
                {
                    throw new InvalidOperationException("Data Received Event was not signaled.");
                }

                WriteLine("Closing PartitionReceiver");
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
                await partitionSender.CloseAsync();
            }
        }

        [Fact]
        async Task GetEventHubRuntimeInformation()
        {
            WriteLine("Getting  EventHubRuntimeInformation");
            var eventHubRuntimeInformation = await this.EventHubClient.GetRuntimeInformationAsync();

            Assert.True(eventHubRuntimeInformation != null, "eventHubRuntimeInformation was null!");
            Assert.True(eventHubRuntimeInformation.PartitionIds != null, "eventHubRuntimeInformation.PartitionIds was null!");
            Assert.True(eventHubRuntimeInformation.PartitionIds.Length != 0, "eventHubRuntimeInformation.PartitionIds.Length was 0!");

            WriteLine("Found partitions:");
            foreach (string partitionId in eventHubRuntimeInformation.PartitionIds)
            {
                WriteLine(partitionId);
            }
        }

        static void WriteLine(string message)
        {
            // Currently xunit2 for .net core doesn't seem to have any output mechanism.  If we find one, replace these here:
            message = DateTime.Now.TimeOfDay + " " + message;
            Debug.WriteLine(message);
            Console.WriteLine(message);
        }

        class TestPartitionReceiveHandler : IPartitionReceiveHandler
        {
            public event EventHandler<IEnumerable<EventData>> EventsReceived;

            public event EventHandler<Exception> ErrorReceived;

            public event EventHandler<Exception> Closed;

            public TestPartitionReceiveHandler()
            {
                this.MaxBatchSize = 10;
            }

            public int MaxBatchSize { get; set; }

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
