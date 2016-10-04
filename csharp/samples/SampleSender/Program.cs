﻿namespace SampleSender
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

            // GetAwaiter().GetResult() will avoid System.AggregateException
            SendMessageToEventHubs().GetAwaiter().GetResult();
        }

        private static async Task SendMessageToEventHubs()
        {
            var connectionSettings = new EventHubsConnectionSettings(EhConnectionString)
            {
                EntityPath = EhEntityPath
            };

            var eventHubClient = EventHubClient.Create(connectionSettings);
            while (true)
            {
                try
                {
                    var message = Guid.NewGuid().ToString();
                    Console.WriteLine("{0} > Sending message: {1}", DateTime.Now, message);
                    await eventHubClient.SendAsync(new EventData(Encoding.UTF8.GetBytes(message)));
                }
                catch (Exception exception)
                {
                    Console.WriteLine("{0} > Exception: {1}", DateTime.Now, exception.Message);
                }

                await Task.Delay(200);
            }
        }
    }
}
