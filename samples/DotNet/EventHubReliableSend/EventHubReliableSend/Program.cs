//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace EventHubReliableSend
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceBus;
    using Microsoft.ServiceBus.Messaging;

    class Program
    {
        const string ConnString = "<Event Hubs Connection String>";
        const string EhName = "<Event Hubs Name>";

        // Increasing number of clients help to distribute load accross multiple TCP channels.
        const int NumberOfParallelClients = 10;

        // Try planning 1MB/sec load per partition.
        const int PartitionCount = 4;

        static List<SenderClient> senderClients = new List<SenderClient>();
        static CancellationTokenSource cancellationTokenSource = new CancellationTokenSource();

        static NamespaceManager nm;
        static Timer monitorTimer;
        static ManualResetEventSlim monitorLock = new ManualResetEventSlim(false);

        static void Main(string[] args)
        {
            var csb = new ServiceBusConnectionStringBuilder(ConnString);
            csb.TransportType = TransportType.Amqp;

            // Create factories.
            nm = NamespaceManager.CreateFromConnectionString(ConnString);

            // Crate Event Hub.
            try
            {
                nm.CreateEventHubIfNotExists(new EventHubDescription(EhName)
                    {
                        PartitionCount = PartitionCount,
                    });
            }
            catch (Exception ex)
            {
                Console.WriteLine("Something went wrong while creating Event Hub {0}. Check the following exception details.", EhName);
                throw;
            }

            // Spawn number of sender clients as requested.
            for (int clientInd=0; clientInd<NumberOfParallelClients; clientInd++)
            {
                // Use dedicated factory for each sender. This will help to distribute load accross remote service nodes.
                var newSenderClient = new SenderClient(clientInd, csb, EhName, cancellationTokenSource.Token);
                senderClients.Add(newSenderClient);
            }

            // Set a timer so we can track the total number of events sent from each client.
            // Schedule the call for every 10 seconds.
            monitorTimer = new Timer(
                new TimerCallback(MonitorClients), null,
                TimeSpan.FromSeconds(10),
                TimeSpan.FromSeconds(10));

            // Keep sending until user interrupts.
            Console.WriteLine("Press ENTER to stop sending on all Event Hub clients.");
            Console.ReadLine();

            // Signal and wait for all sender tasks to complete.
            Console.WriteLine("Signaling the clients to stop sending.");
            cancellationTokenSource.Cancel();
            Task.WaitAll(senderClients.Select(sc => sc.SendTask).ToArray());
            Console.WriteLine("All clients successfully completed sending.");

            monitorTimer.Dispose();
        }

        static void MonitorClients(object obj)
        {
            if (monitorLock.IsSet)
            {
                // There is one already running.
                return;
            }

            monitorLock.Set();
            int totalSent = 0;

            // Log total number of events sent from each client.
            Console.WriteLine("\nEvent Count Metrics:");
            Console.WriteLine("====================");
            senderClients.ForEach(sc => 
                {
                    var thisSent = sc.totalNumberOfEventsSent;
                    Console.WriteLine("Client-{0}: {1} events sent", sc.ClientInd, thisSent);
                    totalSent += thisSent;
                });

            Console.WriteLine("Total Sent: {0}", totalSent);

            // Log partition metrics which will tell us how fast we're sending.
            try
            {
                Console.WriteLine("\nIncoming Throughput Metrics:");
                Console.WriteLine("=============================");
                for (int partitionId=0; partitionId<PartitionCount; partitionId++)
                {
                    var partition = nm.GetEventHubPartition(EhName, EventHubConsumerGroup.DefaultGroupName, partitionId.ToString());
                    Console.WriteLine("Partition {0}: {1} KBytes/sec", partitionId, Math.Round((double)partition.IncomingBytesPerSecond / 1024, 1));
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Caught exception while getting partition throughput metrics: {1}", ex.Message);
            }

            monitorLock.Reset();
        }
    }
}
