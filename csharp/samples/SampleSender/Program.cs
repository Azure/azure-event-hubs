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
            Console.WriteLine("Press Ctrl-C to stop the sender process");
            Console.WriteLine("Press Enter to start now");
            Console.ReadLine();

            SendMessagesToEventHubs().Wait();
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
                    Console.WriteLine("{0} > Sending message: {1}", DateTime.Now, message);
                    await eventHubClient.SendAsync(new EventData(Encoding.UTF8.GetBytes(message)));
                }
                catch (Exception exception)
                {
                    Console.WriteLine("{0} > Exception: {1}", DateTime.Now, exception.Message);
                }

                await Task.Delay(10);
            }
        }
    }
}
