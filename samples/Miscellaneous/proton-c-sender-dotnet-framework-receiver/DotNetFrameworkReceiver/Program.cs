using System;
using System.Text;
using Microsoft.ServiceBus.Messaging;

namespace Proton_DotNet
{
    class Program
    {
        static int Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine("Proton-DotNet.exe connection_string");
                return 2;
            }

            try
            {
                EventHubClient ehc = EventHubClient.CreateFromConnectionString(args[0]);
                EventHubReceiver receiver = ehc.GetDefaultConsumerGroup().CreateReceiver("0");
                while (true)
                {
                    EventData data = receiver.Receive();
                    if (data == null)
                    {
                        break;
                    }

                    string text = Encoding.UTF8.GetString(data.GetBytes());
                    Console.WriteLine(data.SequenceNumber + ":" + text);
                }
                
                ehc.Close();

                return 0;
            }
            catch (Exception exception)
            {
                Console.WriteLine(exception.ToString());
            }

            return 1;
        }
    }
}
