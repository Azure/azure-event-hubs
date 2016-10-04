namespace SampleSender
{
	using System;
	using System.Text;
	using System.Threading.Tasks;
	using Microsoft.Azure.EventHubs;

	public class Program
	{
		private const string EH_CONNECTION_STRING = "{Event Hubs connection string}";

		public static void Main(string[] args)
		{
			Console.WriteLine("Press Ctrl-C to stop the sender process");
			Console.WriteLine("Press Enter to start now");
			Console.ReadLine();

			SendMessageToEh().GetAwaiter().GetResult();
		}

		private static async Task SendMessageToEh()
		{
			var eventHubClient = EventHubClient.Create(EH_CONNECTION_STRING);
			while (true)
			{
				try
				{
					var message = Guid.NewGuid().ToString();
					Console.WriteLine("{0} > Sending message: {1}", DateTime.Now, message);
					await eventHubClient.SendAsync(new EventData(Encoding.UTF8.GetBytes(message)));
				}
				catch (Exception exception)
				{
					Console.ForegroundColor = ConsoleColor.Red;
					Console.WriteLine("{0} > Exception: {1}", DateTime.Now, exception.Message);
					Console.ResetColor();
				}

				await Task.Delay(200);
			}
		}
	}
}
