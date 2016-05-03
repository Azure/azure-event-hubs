namespace Microsoft.Azure.EventHubs.UnitTests
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
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
            }
        }
    }
}
