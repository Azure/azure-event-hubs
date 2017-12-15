using System;
using System.Collections.Generic;
using System.Configuration;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;
using Microsoft.IdentityModel.Clients.ActiveDirectory;
using Microsoft.ServiceBus;
using Microsoft.ServiceBus.Messaging;
using TransportType = Microsoft.ServiceBus.Messaging.TransportType;

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
            Console.WriteLine("[A] Connect via Managed Service Identiry and send / receive.");
            Console.WriteLine("[B] Connect via interactive logon and send / receive.");
            Console.WriteLine("[C] Connect via using username and password and send / receive.");
            Console.WriteLine("[D] Connect via utilizing Certificates and send / receive.");
            //Console.WriteLine("[E] Connect via utilizing Certificate Assertion.");

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
                    UserPasswordCredentialScenario(); // Needs native application
                    break;
                case "D":
                    ClientCredentialsCertScenario(); // This scenario needs app registration in AAD and iam registration. Only web api will work in AAD app registration.
                    break;
                default:
                    Console.WriteLine("Unknown command, press enter to exit");
                    Console.ReadLine();
                    break;
            }      
            //ClientAssertionCertScenario(); // This will be released soon.

        }

        static void ManagedServiceIdentityScenario()
        {
            MessagingFactorySettings messagingFactorySettings = new MessagingFactorySettings
            {
                TokenProvider = TokenProvider.CreateManagedServiceIdentityTokenProvider(ServiceAudience.EventHubsAudience),
                TransportType = TransportType.Amqp
            };

            SendReceive(messagingFactorySettings);
        }

        static void UserInteractiveLoginScenario()
        {
            MessagingFactorySettings messagingFactorySettings = new MessagingFactorySettings
            {
                TokenProvider = TokenProvider.CreateAadTokenProvider(
                    new AuthenticationContext($"https://login.windows.net/{TenantId}"),
                    ClientId,
                    new Uri("http://eventhubs.microsoft.com"),
                    new PlatformParameters(PromptBehavior.SelectAccount),
                    ServiceAudience.EventHubsAudience
                ),
                TransportType = TransportType.Amqp
            };

            SendReceive(messagingFactorySettings);
        }

        static void UserPasswordCredentialScenario()
        {
            UserPasswordCredential userPasswordCredential = new UserPasswordCredential(
                ConfigurationManager.AppSettings["userName"],
                ConfigurationManager.AppSettings["password"]
                );
            MessagingFactorySettings messagingFactorySettings = new MessagingFactorySettings
            {
                TokenProvider = TokenProvider.CreateAadTokenProvider(
                    new AuthenticationContext($"https://login.windows.net/{TenantId}"),
                    ClientId,
                    userPasswordCredential,
                    ServiceAudience.EventHubsAudience
                ),
                TransportType = TransportType.Amqp
            };

            SendReceive(messagingFactorySettings);
        }

        static void ClientAssertionCertScenario()
        {
            X509Certificate2 certificate = GetCertificate();
            ClientAssertionCertificate clientAssertionCertificate = new ClientAssertionCertificate(ClientId, certificate);
            MessagingFactorySettings messagingFactorySettings = new MessagingFactorySettings
            {
                TokenProvider = TokenProvider.CreateAadTokenProvider(
                    new AuthenticationContext($"https://login.windows.net/{TenantId}"),
                    clientAssertionCertificate,
                    ServiceAudience.EventHubsAudience
                    ),
                TransportType = TransportType.Amqp
            };

            SendReceive(messagingFactorySettings);
        }

        static void ClientCredentialsCertScenario()
        {
            ClientCredential clientCredential = new ClientCredential(ClientId, ConfigurationManager.AppSettings["clientSecret"]);
            MessagingFactorySettings messagingFactorySettings = new MessagingFactorySettings
            {
                TokenProvider = TokenProvider.CreateAadTokenProvider(
                    new AuthenticationContext($"https://login.windows.net/{TenantId}"),
                    clientCredential,
                    ServiceAudience.EventHubsAudience
                ),
                TransportType = TransportType.Amqp
            };

            SendReceive(messagingFactorySettings);
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

        static void SendReceive(MessagingFactorySettings messagingFactorySettings)
        {
            MessagingFactory messagingFactory = MessagingFactory.Create($"sb://{EventHubNamespace}/",
                messagingFactorySettings);

            EventHubClient ehClient = messagingFactory.CreateEventHubClient(EventHubName);
            EventHubConsumerGroup consumerGroup = ehClient.GetDefaultConsumerGroup();

            var receiveTask = Task.Factory.StartNew(() =>
                {
                    string[] partitionIds = { "0", "1" };
                    Parallel.ForEach(partitionIds, partitionId =>
                    {
                        EventHubReceiver receiver = consumerGroup.CreateReceiver(partitionId, EventHubConsumerGroup.StartOfStream);
                        while(true)
                        {
                            EventData data = receiver.Receive(TimeSpan.FromSeconds(10));
                            if (data == null)
                                break;
                            Console.WriteLine($"Received from partition {partitionId} : " + Encoding.UTF8.GetString(data.GetBytes()));                            
                        }
                        receiver.Close();
                    });
            });

            System.Threading.Thread.Sleep(5000);

            ehClient.Send(new EventData(Encoding.UTF8.GetBytes($"{DateTime.UtcNow}")));

            Task.WaitAll(receiveTask);

            ehClient.Close();
            messagingFactory.Close();
            Console.WriteLine("Send / Receive complete. Press enter to exit.");
            Console.ReadLine();
        }
    }
}
