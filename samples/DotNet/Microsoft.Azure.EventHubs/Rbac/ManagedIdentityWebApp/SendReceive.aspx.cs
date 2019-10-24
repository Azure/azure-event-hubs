using System;
using System.Linq;
using System.Text;
using System.Threading;
using Azure.Messaging.EventHubs;
using Azure.Identity;

// Always add app to IAM roles
// Don't use on deployment slots but only on root
namespace ManagedIdentityWebApp
{
    public partial class SendReceive : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {

        }

        protected async void btnSend_Click(object sender, EventArgs e)
        {
            EventHubClient client = new EventHubClient($"{txtNamespace.Text}.servicebus.windows.net", txtEventHub.Text, new DefaultAzureCredential());
            await using (EventHubProducer producer = client.CreateProducer())
            {
                var eventsToPublish = new EventData[]
                {
                    new EventData(Encoding.UTF8.GetBytes(txtData.Text))
                };

                await producer.SendAsync(eventsToPublish);
                txtOutput.Text = $"{DateTime.Now} - SENT{Environment.NewLine}" + txtOutput.Text;
            }
        }

        protected async void btnReceive_Click(object sender, EventArgs e)
        {
            EventHubClient client = new EventHubClient($"{txtNamespace.Text}.servicebus.windows.net", txtEventHub.Text, new DefaultAzureCredential());
            string firstPartition = (await client.GetPartitionIdsAsync()).First();
            var totalReceived = 0;
            var receiver = client.CreateConsumer(EventHubConsumer.DefaultConsumerGroupName, firstPartition, EventPosition.Earliest);
            var messages = receiver.ReceiveAsync(int.MaxValue, TimeSpan.FromSeconds(15)).GetAwaiter().GetResult();

            if (messages != null)
            {
                foreach (var message in messages)
                {
                    txtOutput.Text = $"{DateTime.Now} - RECEIVED PartitionId: {firstPartition} data:{Encoding.UTF8.GetString(message.Body.ToArray())}{Environment.NewLine}" + txtOutput.Text;
                }

                Interlocked.Add(ref totalReceived, messages.Count());
            }

            receiver.Close();
            txtOutput.Text = $"{DateTime.Now} - RECEIVED TOTAL = {totalReceived}{Environment.NewLine}" + txtOutput.Text;
        }
    }
}