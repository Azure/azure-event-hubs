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
        string[] PartitionIds;

        public EventHubClientTests()
        {
            this.connectionString = Environment.GetEnvironmentVariable("EVENTHUBCONNECTIONSTRING");
            if (string.IsNullOrWhiteSpace(connectionString))
            {
                throw new InvalidOperationException("EVENTHUBCONNECTIONSTRING environment variable was not found!");
            }

            this.EventHubClient = EventHubClient.Create(connectionString);
            this.EventHubClient.ConnectionSettings.OperationTimeout = TimeSpan.FromSeconds(15);

            // Discover partition ids.
            var eventHubInfo = this.EventHubClient.GetRuntimeInformationAsync().Result;
            this.PartitionIds = eventHubInfo.PartitionIds;
            WriteLine($"EventHub has {PartitionIds.Length} partitions");
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

            await pReceiver.CloseAsync();
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
        async Task CreateReceiverWithOffset()
        {
            // Randomly pick one of the available partitons.
            var partitionId = this.PartitionIds[new Random().Next(this.PartitionIds.Count())];
            WriteLine($"Randomly picked partition {partitionId}");

            // Send and receive a message to identify the end of stream.
            var lastMessage = await SendAndReceiveSingleEvent(partitionId);

            // Send a new message which is expected to go to the end of stream.
            // We are expecting to receive only this message.
            var eventSent = new EventData(new byte[1]);
            eventSent.Properties = new Dictionary<string, object>();
            eventSent.Properties.Add("stamp", Guid.NewGuid().ToString());
            await this.EventHubClient.CreatePartitionSender(partitionId).SendAsync(eventSent);

            // Create a new receiver which will start reading from the last message on the stream.
            WriteLine($"Creating a new receiver with offset {lastMessage.SystemProperties.Offset}");
            var receiver = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, partitionId, lastMessage.SystemProperties.Offset);
            var receivedMessages = await receiver.ReceiveAsync(100);

            // We should have received only 1 message from this call.
            Assert.True(receivedMessages.Count() == 1, $"Didn't receive 1 message. Received {receivedMessages.Count()} messages(s).");

            // Check stamp.
            Assert.True(receivedMessages.Single().Properties["stamp"].ToString() == eventSent.Properties["stamp"].ToString()
                , "Stamps didn't match on the message sent and received!");

            WriteLine("Received correct message as expected.");

            // Next receive on this partition shouldn't return any more messages.
            receivedMessages = await receiver.ReceiveAsync(100, TimeSpan.FromSeconds(15));
            Assert.True(receivedMessages == null, $"Received messages at the end.");

            await receiver.CloseAsync();
        }

        [Fact]
        async Task CreateReceiverWithDateTime()
        {
            // Randomly pick one of the available partitons.
            var partitionId = this.PartitionIds[new Random().Next(this.PartitionIds.Count())];
            WriteLine($"Randomly picked partition {partitionId}");

            // Send and receive a message to identify the end of stream.
            var lastMessage = await SendAndReceiveSingleEvent(partitionId);

            // Send a new message which is expected to go to the end of stream.
            // We are expecting to receive only this message.
            var eventSent = new EventData(new byte[1]);
            eventSent.Properties = new Dictionary<string, object>();
            eventSent.Properties.Add("stamp", Guid.NewGuid().ToString());
            await this.EventHubClient.CreatePartitionSender(partitionId).SendAsync(eventSent);

            // Create a new receiver which will start reading from the last message on the stream.
            WriteLine($"Creating a new receiver with date-time {lastMessage.SystemProperties.EnqueuedTimeUtc}");
            var receiver = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, partitionId, lastMessage.SystemProperties.EnqueuedTimeUtc);
            var receivedMessages = await receiver.ReceiveAsync(100);

            // We should have received only 1 message from this call.
            Assert.True(receivedMessages.Count() == 1, $"Didn't receive 1 message. Received {receivedMessages.Count()} messages(s).");

            // Check stamp.
            Assert.True(receivedMessages.Single().Properties["stamp"].ToString() == eventSent.Properties["stamp"].ToString()
                , "Stamps didn't match on the message sent and received!");

            WriteLine("Received correct message as expected.");

            // Next receive on this partition shouldn't return any more messages.
            receivedMessages = await receiver.ReceiveAsync(100, TimeSpan.FromSeconds(15));
            Assert.True(receivedMessages == null, $"Received messages at the end.");

            await receiver.CloseAsync();
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
            finally
            {
                this.EventHubClient.ConnectionSettings.OperationTimeout = originalTimeout;
                await partitionSender.CloseAsync();
                await partitionReceiver.CloseAsync();
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
            TimeSpan? nextRetryInterval = retry.GetNextRetryInterval(clientId, new EventHubsException(false), TimeSpan.FromSeconds(60));
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
            PartitionReceiver receiver = null;

            try
            {
                foreach (var receiveTimeoutInSeconds in testValues)
                {
                    WriteLine($"Testing with {receiveTimeoutInSeconds} seconds.");

                    // Start receiving from a future time so that Receive call won't be able to fetch any events.
                    receiver = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, "0", DateTime.UtcNow.AddMinutes(1));

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
                await receiver.CloseAsync();
            }
        }

        [Fact]
        async Task MessageSizeExceededException()
        {
            try
            { 
                WriteLine("Sending large event via EventHubClient.SendAsync(EventData)");
                var eventData = new EventData(new byte[300000]);
                await this.EventHubClient.SendAsync(eventData);
                throw new InvalidOperationException("Send should have failed with " +
                    typeof(MessageSizeExceededException).Name);
            }
            catch (MessageSizeExceededException)
            {
                WriteLine("Caught MessageSizeExceededException as expected");
            }
        }

        [Fact]
        async Task SendReceiveNonexistentEntity()
        {
            // Rebuild connection string with a nonexistent entity.
            var conSettings = new EventHubsConnectionSettings(this.connectionString);
            var conString = this.connectionString.Replace(conSettings.EntityPath, Guid.NewGuid().ToString());
            var ehClient = EventHubClient.Create(conString);

            // Try sending.
            try
            {
                WriteLine("Sending an event to nonexistent entity.");
                var sender = ehClient.CreatePartitionSender("0");
                await sender.SendAsync(new EventData(Encoding.UTF8.GetBytes("this send should fail.")));
                throw new InvalidOperationException("Send should have failed");
            }
            catch (MessagingEntityNotFoundException)
            {
                WriteLine("Caught expected MessagingEntityNotFoundException");
            }

            // Try receiving.
            PartitionReceiver receiver = null;
            try
            {
                WriteLine("Receiving from nonexistent entity.");
                receiver = ehClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, "0", PartitionReceiver.StartOfStream);
                await receiver.ReceiveAsync(1);
                throw new InvalidOperationException("Receive should have failed");
            }
            catch (MessagingEntityNotFoundException)
            {
                WriteLine("Caught expected MessagingEntityNotFoundException");
            }
            finally
            {
                await receiver.CloseAsync();
            }
        }

        [Fact]
        async Task PartitionKeyValidation()
        {
            int NumberOfMessagesToSend = 100;
            var partitionOffsets = new Dictionary<string, string>();

            // Discover the end of stream on each partition.
            WriteLine("Discovering end of stream on each partition.");
            foreach (var partitionId in this.PartitionIds)
            {
                var lastEvent = await SendAndReceiveSingleEvent(partitionId);
                partitionOffsets.Add(partitionId, lastEvent.SystemProperties.Offset);
                WriteLine($"Partition {partitionId} has last message with offset {lastEvent.SystemProperties.Offset}");
            }

            // Now send a set of messages with different partition keys.
            WriteLine($"Sending {NumberOfMessagesToSend} messages.");
            Random rnd = new Random();
            for (int i=0; i<NumberOfMessagesToSend; i++)
            {
                var partitionKey = rnd.Next(10);
                await this.EventHubClient.SendAsync(new EventData(Encoding.UTF8.GetBytes("Hello EventHub!")), partitionKey.ToString());
            }

            // It is time to receive all messages that we just sent.
            // Prepare partition key to partition map while receiving.
            // Validation: All messages of a partition key should be received from a single partition.
            WriteLine("Starting to receive all messages from each partition.");
            var partitionMap = new Dictionary<string, string>();
            int totalReceived = 0;
            foreach (var partitionId in this.PartitionIds)
            {
                PartitionReceiver receiver = null;
                try
                {
                    receiver = this.EventHubClient.CreateReceiver(
                        PartitionReceiver.DefaultConsumerGroupName,
                        partitionId,
                        partitionOffsets[partitionId]);
                    var messagesFromPartition = await ReceiveAllMessages(receiver);
                    WriteLine($"Received {messagesFromPartition.Count} messages from partition {partitionId}.");
                    foreach (var ed in messagesFromPartition)
                    {
                        var pk = ed.SystemProperties.PartitionKey;
                        if (partitionMap.ContainsKey(pk) && partitionMap[pk] != partitionId)
                        {
                            throw new Exception($"Received a message from partition {partitionId} with partition key {pk}, whereas the same key was observed on partition {partitionMap[pk]} before.");
                        }

                        partitionMap[pk] = partitionId;
                    }

                    totalReceived += messagesFromPartition.Count;
                }
                finally
                {
                    await receiver.CloseAsync();
                }
            }

            Assert.True(totalReceived == NumberOfMessagesToSend,
                $"Didn't receive the same number of messages that we sent. Sent: {NumberOfMessagesToSend}, Received: {totalReceived}");
        }

        // Sends single message to given partition and returns it after receiving.
        async Task<EventData> SendAndReceiveSingleEvent(string partitionId)
        {
            var eDataToSend = new EventData(new byte[1]);

            // Stamp this message so we can recognize it when received.
            var stampValue = Guid.NewGuid().ToString();
            var sendEvent = new EventData(Encoding.UTF8.GetBytes("Hello EventHub!"));
            eDataToSend.Properties = new Dictionary<string, object>();
            eDataToSend.Properties.Add("stamp", stampValue);
            PartitionSender partitionSender = this.EventHubClient.CreatePartitionSender(partitionId);
            WriteLine($"Sending single event to partition {partitionId} with stamp {stampValue}");
            await partitionSender.SendAsync(eDataToSend);

            WriteLine($"Receiving all messages from partition {partitionId}");
            PartitionReceiver receiver = null;
            try
            {
                receiver = this.EventHubClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName,
                    partitionId, PartitionReceiver.StartOfStream);
                while (true)
                {
                    var receivedEvents = await receiver.ReceiveAsync(100);
                    if (receivedEvents == null || receivedEvents.Count() == 0)
                    {
                        throw new Exception("Not able to receive stamped message!");
                    }

                    WriteLine($"Received {receivedEvents.Count()} event(s) in batch where last event is sent on {receivedEvents.Last().SystemProperties.EnqueuedTimeUtc}");

                    // Continue until we locate stamped message.
                    foreach (var receivedEvent in receivedEvents)
                    {
                        if (receivedEvent.Properties != null &&
                            receivedEvent.Properties.ContainsKey("stamp") &&
                            receivedEvent.Properties["stamp"].ToString() == eDataToSend.Properties["stamp"].ToString())
                        {
                            return receivedEvent;
                        }
                    }
                }
            }
            finally
            {
                await receiver.CloseAsync();
            }
        }

        // Receives all messages on the given receiver.
        async Task<List<EventData>> ReceiveAllMessages(PartitionReceiver receiver)
        {
            List<EventData> messages = new List<EventData>();

            while (true)
            {
                var receivedEvents = await receiver.ReceiveAsync(100);
                if (receivedEvents == null)
                {
                    // There is no more events to receive.
                    break;
                }

                messages.AddRange(receivedEvents);
            }

            return messages;
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
