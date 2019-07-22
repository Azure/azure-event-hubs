//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace EventHubReliableSend
{
    using System;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceBus;
    using Microsoft.ServiceBus.Messaging;

    class SenderClient
    {
        CancellationToken cancelToken;
        EventHubClient ehClient;
        Task sendTask;
        Random rnd;

        public int totalNumberOfEventsSent = 0;

        public SenderClient(int clientInd, ServiceBusConnectionStringBuilder csb, string ehName, CancellationToken cancelToken)
        {
            this.ClientInd = clientInd;
            this.cancelToken = cancelToken;
            this.rnd = new Random();

            // Create Event Hubs client.
            var mf = MessagingFactory.CreateFromConnectionString(csb.ToString());
            this.ehClient = mf.CreateEventHubClient(ehName);

            // Starts sends
            this.sendTask = StartSendsAsync();
        }

        public Task SendTask { get => sendTask; }

        public int ClientInd { get; }

        async Task StartSendsAsync()
        {
            bool sleepBeforeNextSend = false;

            Console.WriteLine("Client-{0}: Starting to send events.", this.ClientInd);

            while (!this.cancelToken.IsCancellationRequested)
            {
                try
                {
                    // Prepare next batch to send.
                    var newBatch = GenerateNextBatch();

                    // Send batch now.
                    await this.ehClient.SendBatchAsync(newBatch.ToEnumerable());
                    var ret = Interlocked.Add(ref totalNumberOfEventsSent, newBatch.Count);
                }
                catch (Exception ex)
                {
                    if (ex is ServerBusyException)
                    {
                        Console.WriteLine("Client-{0}: Going a little faster than what your namespace TU setting allows. Slowing down now.", this.ClientInd);
                        sleepBeforeNextSend = true; 
                    }
                    else
                    {
                        // Log the rest of the exceptions.
                        Console.WriteLine("Client-{0}: Caught exception while attempting to send: {1}", this.ClientInd, ex.Message);
                    }
                }

                if (sleepBeforeNextSend)
                {
                    await Task.Delay(10000);
                    sleepBeforeNextSend = false;
                }
            }

            Console.WriteLine("Client-{0}: Stopped sending events. Total number of events sent: {1}", this.ClientInd, totalNumberOfEventsSent);
        }

        EventDataBatch GenerateNextBatch()
        {
            var ehBatch = this.ehClient.CreateBatch();
            while (true)
            {
                // Send random size payloads.
                var payload = new byte[this.rnd.Next(1024)];
                this.rnd.NextBytes(payload);

                var ed = new EventData(payload);
                ed.Properties["clientind"] = this.ClientInd;

                if (!ehBatch.TryAdd(ed))
                {
                    break;
                }
            }

            return ehBatch;
        }
    }
}
