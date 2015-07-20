using Microsoft.ServiceBus;
using Microsoft.ServiceBus.Messaging;
using Microsoft.ServiceBus.Messaging.Amqp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Csharp_ConsumeEventsFromEventHub
{
    public class Worker
    {
        // This method will be called when the thread is started.
        public void DoWork()
        {
            Console.WriteLine("Starting a worker for partition id =" + partitionId);

            var q = subscriberGroup.CreateReceiver(partitionId);

            while (true)
            {
                var message = q.Receive(); /*blocking, but has some timeout... eventually*/

                if (message != null)
                {
                    string myOffset = message.Offset;
                    string body = Encoding.UTF8.GetString(message.GetBytes());
                    Console.WriteLine(String.Format("Received message.PartitionKey={0}, message.Offset={1}\nbody: {2}\n", message.PartitionKey, myOffset, body));
                }
            }
        }
        public void RequestStop()
        {
            workerThread.Abort(); /*force because blocking*/
        }
        // Volatile is used as hint to the compiler that this data
        // member will be accessed by multiple threads.

        private string partitionId;
        private EventHubConsumerGroup subscriberGroup;
        private Thread workerThread;

        public Worker(EventHubConsumerGroup subscriberGroup, string partitionId)
        {
            // TODO: Complete member initialization
            this.subscriberGroup = subscriberGroup;
            this.partitionId = partitionId;
            workerThread = new Thread(this.DoWork);
            workerThread.Start();
        }
    }

    class Program
    {
      static string connectionString = "[YOUR EVENT HUB CONNECTION STRING WITH MANAGE PERMISSIONS]";
      static string eventHubName = "[YOUR EVENT HUB NAME]";

        static string GetAmqpConnectionString()
        {
            ServiceBusConnectionStringBuilder builder = new ServiceBusConnectionStringBuilder(connectionString);
            builder.TransportType = TransportType.Amqp;
            return builder.ToString();
        }

        static void Main(string[] args)
        {

            // Start a worker that consumes messages from the event hub.
            EventHubClient eventHubReceiveClient = EventHubClient.CreateFromConnectionString(GetAmqpConnectionString(), eventHubName);
            var subscriberGroup = eventHubReceiveClient.GetDefaultConsumerGroup();

            EventHubDescription eventHub = NamespaceManager.CreateFromConnectionString(GetAmqpConnectionString()).GetEventHub(eventHubName);

            // Register event processor with each shard to start consuming messages

            List<Worker> allWorkers = new List<Worker>();
            foreach (var partitionId in eventHub.PartitionIds)
            {
                Worker temp = new Worker(subscriberGroup, partitionId);
                allWorkers.Add(temp);
            }

            // Wait for the user to exit this application.
            Console.WriteLine("\nPress ENTER to exit...\n");
            Console.ReadLine();
            foreach(var w in allWorkers)
            {
                w.RequestStop();
            }
        }
    }
}
