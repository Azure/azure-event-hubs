using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Azure.Messaging.EventHubs;

namespace WindTurbineDataGenerator
{
    internal class Program
    {
        private const string EventHubConnectionString =
            "<EVENT HUBS NAMESPACE CONNECTION STRING>";

        private const string EventHubName = "<EVENT HUB NAME>";
        
        private static int Main()
        {
            Console.WriteLine("Starting wind turbine generator. Press <ENTER> to exit");

            // Start generation of events
            var cts = new CancellationTokenSource();

            var t0 = StartEventGenerationAsync(cts.Token);

            Console.ReadLine();
            cts.Cancel();

            var t1 = Task.Delay(TimeSpan.FromSeconds(3));
            Task.WhenAny(t0, t1).GetAwaiter().GetResult();
           
            return 0;
        }

        private static async Task StartEventGenerationAsync(CancellationToken cancellationToken)
        {
            var random = new Random((int)DateTimeOffset.UtcNow.Ticks);

            // create an Event Hubs Producer client using the namespace connection string and the event hub name
            EventHubProducerClient producerClient = new EventHubProducerClient(EventHubConnectionString, EventHubName);

            while (!cancellationToken.IsCancellationRequested)
            {
                try
                {
                    // Simulate sending data from 100 weather sensors
                    // prepare a batch of events to send to the event hub. 
                    EventDataBatch eventBatch = await producerClient.CreateBatchAsync();
                    for (int i = 0; i < 100; i++)
                    {
                        int scaleFactor = random.Next(0, 25);
                        var windTurbineMeasure = GenerateTurbineMeasure("Turbine_" + i, scaleFactor);
                        EventData evData = SerializeWindTurbineToEventData(windTurbineMeasure);
                        // add the event to the batch
                        eventBatch.TryAdd(evData);
                    }

                    Console.Write(".");

                    // send the batch of events to the event hub
                    await producerClient.SendAsync(eventBatch);
                }
                catch (Exception ex)
                {
                    Debug.WriteLine("Error generating turbine data. Exception: {0}", ex);
                    Console.Write("E");
                }

                await Task.Delay(1000, cancellationToken);
            }
        }       

        private static WindTurbineMeasure GenerateTurbineMeasure(string turbineId, int scaleFactor)
        {
            return new WindTurbineMeasure
            {
                DeviceId = turbineId,
                MeasureTime = DateTime.UtcNow,
                GeneratedPower = 2.5F * scaleFactor,   // in MegaWatts/hour
                WindSpeed = 15 * scaleFactor,          // miles per hour
                TurbineSpeed = 0.3F * scaleFactor      // RPMs
            };
        }

        private static EventData SerializeWindTurbineToEventData(WindTurbineMeasure wtm)
        {
            var messageString = JsonConvert.SerializeObject(wtm);
            var bytes = Encoding.ASCII.GetBytes(messageString);
            return new EventData(bytes);
        }
    }
}
