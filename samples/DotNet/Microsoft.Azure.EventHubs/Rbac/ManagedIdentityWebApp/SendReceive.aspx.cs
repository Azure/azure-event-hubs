using System;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Azure.EventHubs;

// Always add app to IAM roles
// Don't use on deployment slots but only on root
namespace ManagedIdentityWebApp
{
    public partial class SendReceive : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {

        }

        protected void btnSend_Click(object sender, EventArgs e)
        {
            EventHubClient ehClient = EventHubClient.CreateWithManagedIdentity(new Uri($"sb://{txtNamespace.Text}.servicebus.windows.net/"), txtEventHub.Text);
            ehClient.SendAsync(new EventData(Encoding.UTF8.GetBytes(txtData.Text))).GetAwaiter().GetResult();
            txtOutput.Text = $"{DateTime.Now} - SENT{Environment.NewLine}" + txtOutput.Text;
            ehClient.Close();
        }

        protected void btnReceive_Click(object sender, EventArgs e)
        {
            EventHubClient ehClient = EventHubClient.CreateWithManagedIdentity(new Uri($"sb://{txtNamespace.Text}.servicebus.windows.net/"), txtEventHub.Text);
            int partitions = int.Parse(txtPartitions.Text);
            string[] lastOffsets = new string[partitions];

            if (!string.IsNullOrEmpty(hiddenStartingOffset.Value))
            {
                lastOffsets = hiddenStartingOffset.Value.Split(',');
            }

            var totalReceived = 0;

            Parallel.ForEach(Enumerable.Range(0, int.Parse(txtPartitions.Text)), partitionId =>
            {
                var receiver = ehClient.CreateReceiver(PartitionReceiver.DefaultConsumerGroupName, $"{partitionId}", lastOffsets[partitionId] == null ? EventPosition.FromStart() : EventPosition.FromOffset(lastOffsets[partitionId]));
                var messages = receiver.ReceiveAsync(int.MaxValue, TimeSpan.FromSeconds(15)).GetAwaiter().GetResult();

                if (messages != null)
                {
                    foreach (var message in messages)
                    {
                        txtOutput.Text = $"{DateTime.Now} - RECEIVED PartitionId: {partitionId} Seq#:{message.SystemProperties.SequenceNumber} data:{Encoding.UTF8.GetString(message.Body.Array)}{Environment.NewLine}" + txtOutput.Text;
                        lastOffsets[partitionId] = message.SystemProperties.Offset;
                    }

                    Interlocked.Add(ref totalReceived, messages.Count());
                }

                receiver.Close();
            });

            txtOutput.Text = $"{DateTime.Now} - RECEIVED TOTAL = {totalReceived}{Environment.NewLine}" + txtOutput.Text;

            if (totalReceived > 0)
            {
                hiddenStartingOffset.Value = string.Join(",", lastOffsets);
            }

            ehClient.Close();
        }
    }
}