// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace ControlAndDataPlane
{
    using System;
    using System.Configuration;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Identity.Client;
    using Microsoft.ServiceBus;
    using Microsoft.ServiceBus.Messaging;

    class Program
    {
        static readonly string TenantId = ConfigurationManager.AppSettings["tenantId"];
        static readonly string ClientId = ConfigurationManager.AppSettings["clientId"];
        static readonly string ClientSecret = ConfigurationManager.AppSettings["clientSecret"];
        static readonly string EventHubNamespace = ConfigurationManager.AppSettings["eventHubNamespaceFQDN"];

        public static async Task Main(string[] args)
        {
            // Create token provider so that we can use it at both management and runtime clients.
            TokenProvider tokenProvider = TokenProvider.CreateAzureActiveDirectoryTokenProvider(
                async (audience, authority, state) =>
                {
                    IConfidentialClientApplication app = ConfidentialClientApplicationBuilder.Create(ClientId)
                               .WithAuthority(authority)
                               .WithClientSecret(ClientSecret)
                               .Build();

                    var authResult = await app.AcquireTokenForClient(new string[] { $"{audience}/.default" }).ExecuteAsync();
                    return authResult.AccessToken;
                },
                ServiceAudience.EventHubsAudience,
                $"https://login.microsoftonline.com/{TenantId}");

            var eventHubName = "testeh-" + Guid.NewGuid().ToString();

            // Create NamespaceManger and EventHubClient with Azure Active Directory token provider.
            var ehUri = new Uri($"sb://{EventHubNamespace}/");
            var namespaceManager = new NamespaceManager(ehUri, tokenProvider);
            var messagingFactory = MessagingFactory.Create(ehUri, 
                new MessagingFactorySettings()
                {
                    TokenProvider = tokenProvider,
                    TransportType = TransportType.Amqp
                });
            var ehClient = messagingFactory.CreateEventHubClient(eventHubName);

            // Create a new event hub.
            Console.WriteLine($"Creating event hub {eventHubName}");
            await namespaceManager.CreateEventHubAsync(eventHubName);

            // Send and receive a message.
            await SendReceiveAsync(ehClient);

            // Delete event hub.
            Console.WriteLine($"Deleting event hub {eventHubName}");
            await namespaceManager.DeleteEventHubAsync(eventHubName);

            Console.WriteLine("Press enter to exit");
            Console.ReadLine();
        }

        static async Task SendReceiveAsync(EventHubClient ehClient)
        {
            Console.WriteLine("Fetching eventhub description to discover partitions");
            var ehDesc = await ehClient.GetRuntimeInformationAsync();
            Console.WriteLine($"Discovered partitions as {string.Join(",", ehDesc.PartitionIds)}");

            var defaultConsumerGroup = ehClient.GetDefaultConsumerGroup();
            var receiveTasks = ehDesc.PartitionIds.Select(async partitionId =>
                {
                    Console.WriteLine($"Initiating receiver on partition {partitionId}");
                    var receiver = await defaultConsumerGroup.CreateReceiverAsync(partitionId, EventPosition.FromStart());

                    while(true)
                    {
                        var events = await receiver.ReceiveAsync(1, TimeSpan.FromSeconds(15));
                        if (events == null || events.Count() == 0)
                        {
                            break;
                        }

                        Console.WriteLine($"Received from partition {partitionId} with message content '"
                            + Encoding.UTF8.GetString(events.First().GetBytes()) + "'");
                    }

                    await receiver.CloseAsync();
                }).ToList<Task>();

            await Task.Delay(5000);

            Console.WriteLine("Sending single event");
            await ehClient.SendAsync(new EventData(Encoding.UTF8.GetBytes("Hello!")));
            Console.WriteLine("Send done");

            Console.WriteLine("Waiting for receivers to complete");
            await Task.WhenAll(receiveTasks);
            Console.WriteLine("All receivers completed");
        }
    }
}
