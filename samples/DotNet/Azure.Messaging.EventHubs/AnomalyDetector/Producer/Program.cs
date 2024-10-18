﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Producer
{
    using System;
    using System.Text;
    using System.Threading.Tasks;
    using Azure.Messaging.EventHubs;
    using System.Collections.Generic;
    using System.IO;

    public class Program
    {        
        private const string EventHubConnectionString = "<EVENT HUBS NAMESPACE CONNECTION STRING>";
        private const string EventHubName = "<EVENT HUB NAME>";
        private const string TransactionsDumpFile = "mocktransactions.csv";

        private static EventHubProducerClient producerClient;

        public static int Main()
        {
            return MainAsync().GetAwaiter().GetResult();
        }

        private static async Task<int> MainAsync()
        {
            // create an Event Hubs Producer client using the namespace connection string and the event hub name
            producerClient = new EventHubProducerClient(EventHubConnectionString, EventHubName);

            // send messages to the event hub
            await SendMessagesToEventHubAsync(1000);

            await producerClient.CloseAsync();

            Console.WriteLine("Press [enter] to exit.");
            Console.ReadLine();

            return 0;
        }

        // Creates an Event Hub client and sends messages to the event hub.
        private static async Task SendMessagesToEventHubAsync(int numMessagesToSend)
        {
            var eg = new EventGenerator();

            IEnumerable<Transaction> transactions = eg.GenerateEvents(numMessagesToSend);
            
            if (File.Exists(TransactionsDumpFile))
            {
                // exceptions not handled for brevity
                File.Delete(TransactionsDumpFile);
            }

            File.AppendAllText(
                TransactionsDumpFile, 
                $"CreditCardId,Timestamp,Location,Amount,Type{Environment.NewLine}");

            foreach (var t in transactions)
            {
                try
                {
                    // we don't send the transaction type as part of the message.
                    // that is up to the downstream analytics to figure out!
                    // we just pretty print them here so they can easily be compared with the downstream
                    // analytics results.
                    var message = t.Data.ToJson(); 

                    if (t.Type == TransactionType.Suspect)
                    {
                        var fc = Console.ForegroundColor;
                        Console.ForegroundColor = ConsoleColor.Yellow;

                        Console.WriteLine($"Suspect transaction: {message}");

                        Console.ForegroundColor = fc; // reset to original
                    }
                    else
                    {
                        Console.WriteLine($"Regular transaction: {message}");
                    }

                    var line = $"{t.Data.CreditCardId},{t.Data.Timestamp.ToString("o")},{t.Data.Location},{t.Data.Amount},{t.Type}{Environment.NewLine}";

                    File.AppendAllText(TransactionsDumpFile, line);

                    // prepare a batch of events to send to the event hub. only one event in this case. 
                    EventDataBatch eventBatch = await producerClient.CreateBatchAsync();
                    eventBatch.TryAdd(new EventData(Encoding.UTF8.GetBytes(message)));

                    // send the message to the event hub using the producer object
                    await producerClient.SendAsync(eventBatch);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"{t.ToJson()}{Environment.NewLine}Exception: {ex.Message}");
                }

                await Task.Delay(10);
            }

            Console.WriteLine($"{numMessagesToSend} messages sent.");
        }
    }
}

