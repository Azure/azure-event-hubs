// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SampleSender
{
    using System;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Azure.EventHubs;

    public class Program
    {
        private const string EhConnectionString = "{Event Hubs connection string}";
        private const string EhEntityPath = "{Event Hub path/name}";

        public static void Main(string[] args)
        {
            SendMessagesToEventHubs().Wait();
            Console.ReadLine();
        }

        private static async Task SendMessagesToEventHubs()
        {
            var connectionSettings = new EventHubsConnectionSettings(EhConnectionString)
            {
                EntityPath = EhEntityPath
            };

            var eventHubClient = EventHubClient.Create(connectionSettings);

            for (var i = 0; i < 100; i ++)
            {
                try
                {
                    var message = $"Message {i}";
                    Console.WriteLine($"Sending message: {message}");
                    await eventHubClient.SendAsync(new EventData(Encoding.UTF8.GetBytes(message)));
                }
                catch (Exception exception)
                {
                    Console.WriteLine("{0} > Exception: {1}", DateTime.Now, exception.Message);
                }

                await Task.Delay(10);
            }

            Console.WriteLine("All messages sent. Press any key to exit.");
        }
    }
}
