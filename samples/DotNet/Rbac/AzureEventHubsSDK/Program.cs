using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Azure.EventHubs;
using Microsoft.IdentityModel.Clients.ActiveDirectory;

namespace EventHubsSenderReceiverRbac
{
    class Program
    {
        static readonly string TenantId = ConfigurationManager.AppSettings["tenantId"];
        static readonly string ClientId = ConfigurationManager.AppSettings["clientId"]; 
        static readonly string EventHubNamespace = ConfigurationManager.AppSettings["eventHubNamespaceFQDN"];
        static readonly string EventHubName = ConfigurationManager.AppSettings["eventHubName"];

        static void Main()
        {
            Console.WriteLine("Choose an action:");
            Console.WriteLine("[A] Authenticate via Managed Service Identity and send / receive.");
            Console.WriteLine("[B] Authenticate via interactive logon and send / receive.");
            Console.WriteLine("[C] Authenticate via client secret and send / receive.");
            Console.WriteLine("[D] Authenticate via certificate and send / receive.");

            Char key = Console.ReadKey(true).KeyChar;
            String keyPressed = key.ToString().ToUpper();

            switch (keyPressed)
            {
                case "A":
                    ManagedServiceIdentityScenario(); // Use a native app here.
                    break;
                case "B":
                    UserInteractiveLoginScenario(); // Make sure to give Microsoft.ServiceBus and Microsoft.EventHubs under required permissions
                    break;
                case "C":
                    ClientCredentialsScenario(); // This scenario needs app registration in AAD and iam registration. Only web api will work in AAD app registration.
                    break;
                case "E":
                    ClientAssertionCertScenario();
                    break;
                default:
                    Console.WriteLine("Unknown command, press enter to exit");
                    Console.ReadLine();
                    break;
            }      
        }

        static void ManagedServiceIdentityScenario()
        {
            var ehClient = EventHubClient.CreateWithManagedServiceIdentity(new Uri($"sb://{EventHubNamespace}/"), EventHubName);

            SendReceiveAsync(ehClient).Wait();
        }

        static void UserInteractiveLoginScenario()
        {
            TokenProvider tp = TokenProvider.CreateAadTokenProvider(
                new AuthenticationContext($"https://login.windows.net/{TenantId}"),
                ClientId,
                new Uri("http://eventhubs.microsoft.com"),
                new PlatformParameters(PromptBehavior.Auto),
                UserIdentifier.AnyUser
            );

            var ehClient = EventHubClient.Create(new Uri($"sb://{EventHubNamespace}/"), EventHubName, tp);

            SendReceiveAsync(ehClient).Wait();
        }

        static void ClientAssertionCertScenario()
        {
            X509Certificate2 certificate = GetCertificate();
            ClientAssertionCertificate clientAssertionCertificate = new ClientAssertionCertificate(ClientId, certificate);
            TokenProvider tp = TokenProvider.CreateAadTokenProvider(new AuthenticationContext($"https://login.windows.net/{TenantId}"), clientAssertionCertificate);

            var ehClient = EventHubClient.Create(new Uri($"sb://{EventHubNamespace}/"), EventHubName, tp);

            SendReceiveAsync(ehClient).Wait();
        }

        static void ClientCredentialsScenario()
        {
            ClientCredential clientCredential = new ClientCredential(ClientId, ConfigurationManager.AppSettings["clientSecret"]);
            TokenProvider tp = TokenProvider.CreateAadTokenProvider(new AuthenticationContext($"https://login.windows.net/{TenantId}"), clientCredential);

            var ehClient = EventHubClient.Create(new Uri($"sb://{EventHubNamespace}/"), EventHubName, tp);

            SendReceiveAsync(ehClient).Wait();
        }

        static X509Certificate2 GetCertificate()
        {
            List<StoreLocation> locations = new List<StoreLocation>
                {
                    StoreLocation.CurrentUser,
                    StoreLocation.LocalMachine
                };

            foreach (var location in locations)
            {
                X509Store store = new X509Store(StoreName.My, location);
                try
                {
                    store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);
                    X509Certificate2Collection certificates = store.Certificates.Find(
                        X509FindType.FindByThumbprint, ConfigurationManager.AppSettings["thumbPrint"], true);
                    if (certificates.Count >= 1)
                    {
                        return certificates[0];
                    }
                }
                finally
                {
                    store.Close();
                }
            }

            throw new ArgumentException($"A Certificate with Thumbprint '{ConfigurationManager.AppSettings["thumbPrint"]}' could not be located.");
        }

        static async Task SendReceiveAsync(EventHubClient ehClient)
        {
            Console.WriteLine("Fetching eventhub description to discover partitions");
            var ehDesc = await ehClient.GetRuntimeInformationAsync();
            Console.WriteLine($"Discovered partitions as {string.Join(",", ehDesc.PartitionIds)}");

            var receiveTasks = ehDesc.PartitionIds.Select(async partitionId =>
                    {
                        Console.WriteLine($"Initiating receiver on partition {partitionId}");
                        var receiver = ehClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, partitionId, EventPosition.FromEnd());

                        while(true)
                        {
                            var events = await receiver.ReceiveAsync(1, TimeSpan.FromSeconds(15));
                            if (events == null)
                            {
                                break;
                            }

                            var eventData = events.FirstOrDefault();
                            Console.WriteLine($"Received from partition {partitionId} with message content '" + Encoding.UTF8.GetString(eventData.Body.Array) + "'"); 
                        }

                        await receiver.CloseAsync();
                    }).ToList<Task>();

            await Task.Delay(5000);

            Console.WriteLine("Sending single event");
            await ehClient.SendAsync(new EventData(Encoding.UTF8.GetBytes($"{DateTime.UtcNow}")));
            Console.WriteLine("Send done");

            Console.WriteLine("Waiting for receivers to complete");
            await Task.WhenAll(receiveTasks);
            Console.WriteLine("All receivers completed");

            await ehClient.CloseAsync();

            Console.WriteLine("Press enter to exit.");

            Console.ReadLine();
        }
    }
}
