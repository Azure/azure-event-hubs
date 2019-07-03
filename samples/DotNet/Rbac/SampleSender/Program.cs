// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SampleSender
{
    using System;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Azure.EventHubs;
    using Microsoft.Identity.Client;
    using System.Configuration;

    public class Program
    {

        private static bool SetRandomPartitionKey = false;

        public static void Main(string[] args)
        {
            MainAsync(args).GetAwaiter().GetResult();
        }

        private static async Task MainAsync(string[] args)
        {

            string clientId = ConfigurationManager.AppSettings["clientId"];
            string tenantId = ConfigurationManager.AppSettings["tenantId"];
            string eventHubsNamespace = ConfigurationManager.AppSettings["eventHubNamespaceFQDN"];
            string eventHubName = ConfigurationManager.AppSettings["eventHubName"];
            string clientSecret = ConfigurationManager.AppSettings["clientSecret"];

            TokenProvider tp = TokenProvider.CreateAzureActiveDirectoryTokenProvider(
               async (audience, authority, state) =>
               {
                   IConfidentialClientApplication app = ConfidentialClientApplicationBuilder.Create(clientId)
                              .WithTenantId(tenantId)
                              .WithClientSecret(ConfigurationManager.AppSettings["clientSecret"])
                              .Build();

                   var authResult = await app.AcquireTokenForClient(new string[] { $"{audience}/.default" }).ExecuteAsync();
                   return authResult.AccessToken;
               });

            var ehClient = EventHubClient.CreateWithTokenProvider(new Uri($"sb://{eventHubsNamespace}/"), eventHubName, tp);
            await SendMessagesToEventHub(ehClient);

            Console.WriteLine("Press any key to exit.");
            Console.ReadLine();
        }

        
        // Creates an Event Hub client and sends 1000 messages to the event hub.
        private static async Task SendMessagesToEventHub (EventHubClient ehClient)
        {
            var numMessagesToSend = 1000;
            var rnd = new Random();

            for (var i = 0; i < numMessagesToSend; i++)
            {
                try
                {
                    var message = $"Message {i}";

                    // Set random partition key?
                    if (SetRandomPartitionKey)
                    {
                        var pKey = Guid.NewGuid().ToString();
                        await ehClient.SendAsync(new EventData(Encoding.UTF8.GetBytes(message)), pKey);
                        Console.WriteLine($"Sent message: '{message}' Partition Key: '{pKey}'");
                    }
                    else
                    {
                        await ehClient.SendAsync(new EventData(Encoding.UTF8.GetBytes(message)));
                        Console.WriteLine($"Sent message: '{message}'");
                    }
                }
                catch (Exception exception)
                {
                    Console.WriteLine($"{DateTime.Now} > Exception: {exception.Message}");
                }

                await Task.Delay(10);
            }

            await ehClient.CloseAsync();

            Console.WriteLine($"{numMessagesToSend} messages sent.");
        }

       
    }
}
