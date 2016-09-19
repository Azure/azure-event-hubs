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
        string connectionString;

        public EventHubClientTests()
        {
            this.connectionString = Environment.GetEnvironmentVariable("EVENTHUBCONNECTIONSTRING");
            if (string.IsNullOrWhiteSpace(connectionString))
            {
                throw new InvalidOperationException("EVENTHUBCONNECTIONSTRING environment variable was not found!");
            }

            this.EventHubClient = EventHubClient.Create(connectionString);
        }

        EventHubClient EventHubClient { get; }

        [Fact]
        async Task CloseSenderClient()
        {
            var pSender = this.EventHubClient.CreatePartitionSender("0");
            var pReceiver = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, "0", DateTime.UtcNow);

            WriteLine("Sending single event to partition 0");
            var eventData = new EventData(Encoding.UTF8.GetBytes("Hello EventHub!"));
            await pSender.SendAsync(eventData);

            WriteLine("Closing partition sender");
            await pSender.CloseAsync();

            try
            {
                WriteLine("Sending another event to partition 0 on the closed sender, this should fail");
                eventData = new EventData(Encoding.UTF8.GetBytes("Hello EventHub!"));
                await pSender.SendAsync(eventData);
                throw new InvalidOperationException("Send should have failed");
            }
            catch (ObjectDisposedException)
            {
                WriteLine("Caught ObjectDisposedException as expected");
            }
        }

        [Fact]
        async Task CloseReceiverClient()
        {
            var pSender = this.EventHubClient.CreatePartitionSender("0");
            var pReceiver = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, "0", DateTime.UtcNow);

            WriteLine("Sending single event to partition 0");
            var eventData = new EventData(Encoding.UTF8.GetBytes("Hello EventHub!"));
            await pSender.SendAsync(eventData);

            WriteLine("Receiving the event.");
            var events = await pReceiver.ReceiveAsync(1);
            Assert.True(events != null && events.Count() == 1, "Failed to receive 1 event");

            WriteLine("Closing partition receiver");
            await pReceiver.CloseAsync();

            try
            {
                WriteLine("Receiving another event from partition 0 on the closed receiver, this should fail");
                await pReceiver.ReceiveAsync(1);
                throw new InvalidOperationException("Receive should have failed");
            }
            catch (ObjectDisposedException)
            {
                WriteLine("Caught ObjectDisposedException as expected");
            }
        }

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
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(15);
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
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(15);
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
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(15);
            string partitionId = "1";
            PartitionReceiver partitionReceiver = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, partitionId, DateTime.UtcNow.AddMinutes(-10));
            PartitionSender partitionSender = this.EventHubClient.CreatePartitionSender(partitionId);
            try
            {
                string uniqueEventId = Guid.NewGuid().ToString();
                WriteLine($"Sending an event to Partition {partitionId} with custom property EventId {uniqueEventId}");
                var sendEvent = new EventData(Encoding.UTF8.GetBytes("Hello EventHub!"));
                sendEvent.Properties = new Dictionary<string, object> { ["EventId"] = uniqueEventId };
                await partitionSender.SendAsync(sendEvent);

                EventWaitHandle dataReceivedEvent = new EventWaitHandle(false, EventResetMode.ManualReset);
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
                
                partitionReceiver.SetReceiveHandler(handler);

                if (!dataReceivedEvent.WaitOne(TimeSpan.FromSeconds(20)))
                {
                    throw new InvalidOperationException("Data Received Event was not signaled.");
                }
            }
            catch (Exception)
            {
                await partitionReceiver.CloseAsync();
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

        [Fact]
        void ValidateRetryPolicy()
        {
            String clientId = "someClientEntity";
            RetryPolicy retry = RetryPolicy.GetRetryPolicy(RetryPolicyType.Default);

            retry.IncrementRetryCount(clientId);
            TimeSpan? firstRetryInterval = retry.GetNextRetryInterval(clientId, new ServerBusyException(string.Empty), TimeSpan.FromSeconds(60));
            WriteLine("firstRetryInterval: " + firstRetryInterval);
            Assert.True(firstRetryInterval != null);

            retry.IncrementRetryCount(clientId);
            TimeSpan? secondRetryInterval = retry.GetNextRetryInterval(clientId, new ServerBusyException(string.Empty), TimeSpan.FromSeconds(60));
            WriteLine("secondRetryInterval: " + secondRetryInterval);

            Assert.True(secondRetryInterval != null);
            Assert.True(secondRetryInterval?.TotalMilliseconds > firstRetryInterval?.TotalMilliseconds);

            retry.IncrementRetryCount(clientId);
            TimeSpan? thirdRetryInterval = retry.GetNextRetryInterval(clientId, new ServerBusyException(string.Empty), TimeSpan.FromSeconds(60));
            WriteLine("thirdRetryInterval: " + thirdRetryInterval);

            Assert.True(thirdRetryInterval != null);
            Assert.True(thirdRetryInterval?.TotalMilliseconds > secondRetryInterval?.TotalMilliseconds);

            retry.IncrementRetryCount(clientId);
            TimeSpan? fourthRetryInterval = retry.GetNextRetryInterval(clientId, new ServerBusyException(string.Empty), TimeSpan.FromSeconds(60));
            WriteLine("fourthRetryInterval: " + fourthRetryInterval);

            Assert.True(fourthRetryInterval != null);
            Assert.True(fourthRetryInterval?.TotalMilliseconds > thirdRetryInterval?.TotalMilliseconds);

            retry.IncrementRetryCount(clientId);
            TimeSpan? fifthRetryInterval = retry.GetNextRetryInterval(clientId, new ServerBusyException(string.Empty), TimeSpan.FromSeconds(60));
            WriteLine("fifthRetryInterval: " + fifthRetryInterval);

            Assert.True(fifthRetryInterval != null);
            Assert.True(fifthRetryInterval?.TotalMilliseconds > fourthRetryInterval?.TotalMilliseconds);

            retry.IncrementRetryCount(clientId);
            TimeSpan? sixthRetryInterval = retry.GetNextRetryInterval(clientId, new ServerBusyException(string.Empty), TimeSpan.FromSeconds(60));
            WriteLine("sixthRetryInterval: " + sixthRetryInterval);

            Assert.True(sixthRetryInterval != null);
            Assert.True(sixthRetryInterval?.TotalMilliseconds > fifthRetryInterval?.TotalMilliseconds);

            retry.IncrementRetryCount(clientId);
            TimeSpan? seventhRetryInterval = retry.GetNextRetryInterval(clientId, new ServerBusyException(string.Empty), TimeSpan.FromSeconds(60));
            WriteLine("seventhRetryInterval: " + seventhRetryInterval);

            Assert.True(seventhRetryInterval != null);
            Assert.True(seventhRetryInterval?.TotalMilliseconds > sixthRetryInterval?.TotalMilliseconds);

            retry.IncrementRetryCount(clientId);
            TimeSpan? nextRetryInterval = retry.GetNextRetryInterval(clientId, new ServiceBusException(false), TimeSpan.FromSeconds(60));
            Assert.True(nextRetryInterval == null);

            retry.ResetRetryCount();
            retry.IncrementRetryCount(clientId);
            TimeSpan? firstRetryIntervalAfterReset = retry.GetNextRetryInterval(clientId, new ServerBusyException(string.Empty), TimeSpan.FromSeconds(60));
            Assert.True(firstRetryInterval.Equals(firstRetryIntervalAfterReset));

            retry = RetryPolicy.GetRetryPolicy(RetryPolicyType.NoRetry);
            retry.IncrementRetryCount(clientId);
            TimeSpan? noRetryInterval = retry.GetNextRetryInterval(clientId, new ServerBusyException(string.Empty), TimeSpan.FromSeconds(60));
            Assert.True(noRetryInterval == null);
        }

        [Fact]
        async Task ReceiveTimeout()
        {
            var testValues = new[] { 10, 30, 120 };

            TimeSpan originalTimeout = this.EventHubClient.ConnectionSettings.OperationTimeout;

            try
            {
                foreach (var receiveTimeoutInSeconds in testValues)
                {
                    WriteLine($"Testing with {receiveTimeoutInSeconds} seconds.");

                    // Start receiving from a future time so that Receive call won't be able to fetch any events.
                    var receiver = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, "0", DateTime.UtcNow.AddMinutes(1));

                    var startTime = DateTime.Now;
                    await receiver.ReceiveAsync(1, TimeSpan.FromSeconds(receiveTimeoutInSeconds));

                    // Receive call should have waited more than receive timeout.
                    Assert.True(DateTime.Now > startTime.AddSeconds(receiveTimeoutInSeconds));

                    // Timeout should not be late more than 5 seconds.
                    // This is just a logical buffer for timeout behavior validation.
                    Assert.True(DateTime.Now < startTime.AddSeconds(receiveTimeoutInSeconds + 5));
                }
            }
            finally
            {
                this.EventHubClient.ConnectionSettings.OperationTimeout = originalTimeout;
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

            public TestPartitionReceiveHandler()
            {
                this.MaxBatchSize = 10;
            }

            public int MaxBatchSize { get; set; }

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
