using System;
using System.Configuration;
using System.Text;
using System.Threading.Tasks;
using Microsoft.IdentityModel.Clients.ActiveDirectory;
using Microsoft.ServiceBus;
using Microsoft.ServiceBus.Messaging;

namespace EventHubsSenderReceiverRbac
{
    class Program
    {
        static void Main()
        {
            string tenantId = ConfigurationManager.AppSettings["tenantId"];
            string clientId = ConfigurationManager.AppSettings["clientId"];
            string eventHubNamespace = ConfigurationManager.AppSettings["eventHubNamespaceFQDN"];
            string eventHubName = ConfigurationManager.AppSettings["eventHubName"];

            MessagingFactorySettings messageFactorySettings = new MessagingFactorySettings
            {
                TokenProvider = TokenProvider.CreateAadTokenProvider(
                    new AuthenticationContext($"https://login.windows.net/{tenantId}"),
                    clientId,
                    new Uri("http://eventhubs.microsoft.com"),
                    new PlatformParameters(PromptBehavior.SelectAccount),
                    ServiceAudience.EventHubsAudience
                ),
                TransportType = TransportType.Amqp
            };

            // TODO - Remove after backend is patched with the AuthComponent open fix
            messageFactorySettings.AmqpTransportSettings.EnableLinkRedirect = false;

            MessagingFactory messagingFactory = MessagingFactory.Create($"sb://{eventHubNamespace}/",
                messageFactorySettings);

            EventHubClient ehClient = messagingFactory.CreateEventHubClient(eventHubName);
            ehClient.Send(new EventData(Encoding.UTF8.GetBytes($"{DateTime.UtcNow}")));

            EventHubConsumerGroup consumerGroup = ehClient.GetDefaultConsumerGroup();

            // TODO - uncomment once GetRuntimeInformation supports AuthZ
            // string[] partitionIds = ehClient.GetRuntimeInformation().PartitionIds
            string[] partitionIds = {"0", "1"};
            Parallel.ForEach(partitionIds, partitionId =>
            {
                EventHubReceiver receiver = consumerGroup.CreateReceiver(partitionId);
                EventData data = receiver.Receive();
                Console.WriteLine(Encoding.UTF8.GetString(data.GetBytes()));
                receiver.Close();
            });

            ehClient.Close();
        }
    }
}
