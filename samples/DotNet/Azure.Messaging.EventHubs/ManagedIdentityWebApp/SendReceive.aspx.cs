using System;
using System.Linq;
using System.Text;
using System.Threading;
using Azure.Messaging.EventHubs;
using Azure.Identity;
using System.Threading.Tasks;

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
            await using (EventHubProducerClient producerClient = new EventHubProducerClient($"{txtNamespace.Text}.servicebus.windows.net", txtEventHub.Text, new DefaultAzureCredential()))
            {
                // create a batch
                EventDataBatch eventBatch = await producerClient.CreateBatchAsync();

                // add events to the batch. only one in this case. 
                eventBatch.TryAdd(new EventData(Encoding.UTF8.GetBytes(txtData.Text)));
                
                // send the batch to the event hub
                await producerClient.SendAsync(eventBatch);

                txtOutput.Text = $"{DateTime.Now} - SENT{Environment.NewLine}" + txtOutput.Text;
            }
        }
        protected async void btnReceive_Click(object sender, EventArgs e)
        {
            await using (var consumerClient = new EventHubConsumerClient(EventHubConsumerClient.DefaultConsumerGroupName, $"{txtNamespace.Text}.servicebus.windows.net", txtEventHub.Text, new DefaultAzureCredential()))
            {
                int eventsRead = 0;
                try
                {
                    using CancellationTokenSource cancellationSource = new CancellationTokenSource();
                    cancellationSource.CancelAfter(TimeSpan.FromSeconds(5));

                    await foreach (PartitionEvent partitionEvent in consumerClient.ReadEventsAsync(cancellationSource.Token))
                    {
                        txtOutput.Text = $"Event Read: { Encoding.UTF8.GetString(partitionEvent.Data.Body.ToArray()) }{ Environment.NewLine}" + txtOutput.Text;
                        eventsRead++;
                    }
                }
                catch (TaskCanceledException ex)
                {
                    txtOutput.Text = $"Number of events read: {eventsRead}{ Environment.NewLine}" + txtOutput.Text;
                }
            }
        }
    }
}