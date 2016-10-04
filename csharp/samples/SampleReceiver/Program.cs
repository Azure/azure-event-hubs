namespace SampleReceiver
{
    using System;
    using System.Threading.Tasks;
    using Microsoft.Azure.EventHubs;
    using Microsoft.Azure.EventHubs.Processor;

    public class Program
    {
        private const string EhConnectionString = "{Event Hubs connection string}";
        private const string EhEntityPath = "{Event Hub path/name}";
        private const string StorageContainerName = "{Storage account container name}";
        private const string StorageAccountName = "{Storage account name}";
        private const string StorageAccountKey = "{Storage account key}";

        private static readonly string storageConnectionString = string.Format("DefaultEndpointsProtocol=https;AccountName={0};AccountKey={1}", StorageAccountName, StorageAccountKey);

        public static void Main(string[] args)
        {
            // GetAwaiter().GetResult() will avoid System.AggregateException
            ReceiveMessagesFromEventHubs().GetAwaiter().GetResult();

            Console.WriteLine("Receiving. Press enter key to stop worker.");
            Console.ReadLine();
        }

        private static async Task ReceiveMessagesFromEventHubs()
        {
            var eventProcessorHost = new EventProcessorHost(
                PartitionReceiver.DefaultConsumerGroupName,
                EhConnectionString,
                storageConnectionString,
                StorageContainerName
            );

            Console.WriteLine("Registering EventProcessor...");
            await eventProcessorHost.RegisterEventProcessorAsync<SimpleEventProcessor>();
        }
    }
}
