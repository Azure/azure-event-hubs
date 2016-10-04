namespace SampleReceiver
{
    using System;
    using System.Threading.Tasks;
    using Microsoft.Azure.EventHubs;
    using Microsoft.Azure.EventHubs.Processor;

    public class Program
    {
        private const string EH_CONNECTION_STRING = "{Event Hubs connection string}";
        private const string STORAGE_CONTAINER_NAME = "{Storage account container name}";
        private const string STORAGE_ACCOUNT_NAME = "{Storage account name}";
        private const string STORAGE_ACCOUNT_KEY = "{Storage account key}";

        private static readonly string storageConnectionString = string.Format("DefaultEndpointsProtocol=https;AccountName={0};AccountKey={1}", STORAGE_ACCOUNT_NAME, STORAGE_ACCOUNT_KEY);

        public static void Main(string[] args)
        {
            RunEph().GetAwaiter().GetResult();
            Console.WriteLine("Receiving. Press enter key to stop worker.");
            Console.ReadLine();
        }

        private static async Task RunEph()
        {
            var eventProcessorHost = new EventProcessorHost(
                PartitionReceiver.DefaultConsumerGroupName,
                EH_CONNECTION_STRING,
                storageConnectionString,
                STORAGE_CONTAINER_NAME
            );

            Console.WriteLine("Registering EventProcessor...");
            await eventProcessorHost.RegisterEventProcessorAsync<SimpleEventProcessor>();
        }
    }
}
