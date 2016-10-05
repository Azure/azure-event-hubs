// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SampleEphReceiver
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

        private static readonly string StorageConnectionString = string.Format("DefaultEndpointsProtocol=https;AccountName={0};AccountKey={1}", StorageAccountName, StorageAccountKey);

        private static EventProcessorHost eventProcessorHost;

        public static void Main(string[] args)
        {
            StartHost().Wait();

            Console.WriteLine("Receiving. Press enter key to stop worker.");
            Console.ReadLine();
            StopHost().Wait();
        }

        private static Task StartHost()
        {
            eventProcessorHost = new EventProcessorHost(
                PartitionReceiver.DefaultConsumerGroupName,
                EhConnectionString,
                StorageConnectionString,
                StorageContainerName);

            Console.WriteLine("Registering EventProcessor...");
            return eventProcessorHost.RegisterEventProcessorAsync<SimpleEventProcessor>();
        }

        private static Task StopHost()
        {
            return eventProcessorHost?.UnregisterEventProcessorAsync();
        }
    }
}
