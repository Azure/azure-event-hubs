namespace SampleReceiver
{
	using System;
	using System.Collections.Generic;
	using System.Text;
	using System.Threading.Tasks;
	using Microsoft.Azure.EventHubs;
	using Microsoft.Azure.EventHubs.Processor;

	public class SimpleEventProcessor : IEventProcessor
	{
		public async Task CloseAsync(PartitionContext context, CloseReason reason)
		{
			Console.WriteLine("Processor Shutting Down. Partition '{0}', Reason: '{1}'.", context.PartitionId, reason);
			if (reason == CloseReason.Shutdown)
			{
				await context.CheckpointAsync();
			}
		}

		public Task OpenAsync(PartitionContext context)
		{
			Console.WriteLine("SimpleEventProcessor initialized.  Partition: '{0}'", context.PartitionId);
			return Task.FromResult<object>(null);
		}

		public Task ProcessErrorAsync(PartitionContext context, Exception error)
		{
			return Task.Factory.StartNew(() => { Console.WriteLine(error.Message); });
		}

		public async Task ProcessEventsAsync(PartitionContext context, IEnumerable<EventData> messages)
		{
			foreach (var eventData in messages)
			{
				var data = Encoding.UTF8.GetString(eventData.Body.Array, eventData.Body.Offset, eventData.Body.Count);
				Console.WriteLine(string.Format("Message received.  Partition: '{0}', Data: '{1}'", context.PartitionId, data));
			}

			await context.CheckpointAsync();
		}
	}
}
