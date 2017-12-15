using Microsoft.ServiceBus;
using Microsoft.ServiceBus.Messaging;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;

// Always add app to IAM roles
// Don't use on deployment slots but only on root
namespace EventHubsMSIDemoWebApp
{
    public partial class EventHubsMSIDemo : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {

        }

        protected void btnSend_Click(object sender, EventArgs e)
        {
            MessagingFactorySettings messagingFactorySettings = new MessagingFactorySettings
            {
                TokenProvider = TokenProvider.CreateManagedServiceIdentityTokenProvider(ServiceAudience.EventHubsAudience),
                TransportType = TransportType.Amqp
            };

            // TODO - Remove after backend is patched with the AuthComponent open fix
            messagingFactorySettings.AmqpTransportSettings.EnableLinkRedirect = false;

            MessagingFactory messagingFactory = MessagingFactory.Create($"sb://{txtNamespace.Text}.servicebus.windows.net/",
                messagingFactorySettings);

            EventHubClient ehClient = messagingFactory.CreateEventHubClient(txtEventHub.Text);
            ehClient.Send(new EventData(Encoding.UTF8.GetBytes(txtData.Text)));
            ehClient.Close();
            messagingFactory.Close();
        }

        protected void btnReceive_Click(object sender, EventArgs e)
        {
            MessagingFactorySettings messagingFactorySettings = new MessagingFactorySettings
            {
                TokenProvider = TokenProvider.CreateManagedServiceIdentityTokenProvider(ServiceAudience.EventHubsAudience),
                TransportType = TransportType.Amqp
            };

            messagingFactorySettings.AmqpTransportSettings.EnableLinkRedirect = false;

            MessagingFactory messagingFactory = MessagingFactory.Create($"sb://{txtNamespace.Text}.servicebus.windows.net/",
                messagingFactorySettings);

            EventHubClient ehClient = messagingFactory.CreateEventHubClient(txtEventHub.Text);
            EventHubConsumerGroup consumerGroup = ehClient.GetDefaultConsumerGroup();
            int partitions = int.Parse(txtPartitions.Text);
            string[] Offsets = new string[partitions];
            if (!string.IsNullOrEmpty(hiddenStartingOffset.Value))
            {
                Offsets = hiddenStartingOffset.Value.Split(',');
            }
            System.Threading.Tasks.Parallel.ForEach(Enumerable.Range(0, int.Parse(txtPartitions.Text)), partitionId =>
            {
                EventHubReceiver receiver = consumerGroup.CreateReceiver($"{partitionId}", Offsets[partitionId] == null ? "-1" : Offsets[partitionId]);
                EventData data = receiver.Receive(TimeSpan.FromSeconds(1));
                if (data != null)
                {
                    Offsets[partitionId] = data.Offset;
                    txtReceivedData.Text += $"PartitionId: {partitionId} Seq#:{data.SequenceNumber} data:{Encoding.UTF8.GetString(data.GetBytes())}{Environment.NewLine}";
                }
                receiver.Close();
            });

            hiddenStartingOffset.Value = string.Join(",", Offsets);
            ehClient.Close();
            messagingFactory.Close();
        }
    }
}