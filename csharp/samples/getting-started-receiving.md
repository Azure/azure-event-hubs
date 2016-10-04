# Get started with Event Hubs in .NET

## What will be accomplished

In this tutorial, we will write a console application to receive messages from an Event Hub using the **EventProcessorHost**.

## Prerequisites

1. [Visual Studio 2015](http://www.visualstudio.com).

2. An Azure subscription.

3. [An Event Hubs namespace]().
    
## Receive messages from the Event Hub

### Create a console application

1. Launch Visual Studio and create a new Console application.

### Add the Event Hubs NuGet package

1. Right-click the newly created project and select **Manage NuGet Packages**.

2. Click the **Browse** tab, then search for “Microsoft Azure Event Processor Host” and select the **Microsoft Azure Event Processor Host** item. Click **Install** to complete the installation, then close this dialog box.

    ![Select a NuGet package][nuget-pkg]

### Implement the IEventProcessor interface

1. Create a new class called `SimpleEventProcessor'.

2. Add the following `using` statements to the top of the SimpleEventProcessor.cs file.

    ```cs
    using Microsoft.Azure.EventHubs;
	using Microsoft.Azure.EventHubs.Processor;
    ```

3. Implement the `IEventProcessor` interface. The class should look like this:

    ```cs
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
    ```

### Write some code that uses `SimpleEventProcessor` to receive messages from an Event Hub

1. Add the following `using` statements to the top of the Program.cs file.
  
    ```cs
    using Microsoft.Azure.EventHubs;
    using Microsoft.Azure.EventHubs.Processor;
    ```

2. Add constants to the `Program` class for the Event Hubs connection string, Event Hub path, storage container name, storage account name, and storage account key. Replace placeholders with their corresponding values.

    ```cs
    private const string EhConnectionString = "{Event Hubs connection string}";
    private const string EhEntityPath = "{Event Hub path/name}"
    private const string STORAGE_CONTAINER_NAME = "{Storage account container name}";
    private const string STORAGE_ACCOUNT_NAME = "{Storage account name}";
    private const string STORAGE_ACCOUNT_KEY = "{Storage account key}";

    private static readonly string storageConnectionString = string.Format("DefaultEndpointsProtocol=https;AccountName={0};AccountKey={1}", STORAGE_ACCOUNT_NAME, STORAGE_ACCOUNT_KEY);
    ```   

3. Add a new method to the `Program` class like the following:

    ```cs
    private static async Task ReceiveMessagesFromEventHubs()
    {
        var connectionSettings = new EventHubsConnectionSettings(EhConnectionString)
        {
            EntityPath = EhEntityPath
        };

        var eventProcessorHost = new EventProcessorHost(
            PartitionReceiver.DefaultConsumerGroupName,
            connectionSettings,
            storageConnectionString,
            StorageContainerName
        );

        Console.WriteLine("Registering EventProcessor...");
        await eventProcessorHost.RegisterEventProcessorAsync<SimpleEventProcessor>();
    }
    ```

3. Add the following code to the `Main` method:

    ```cs
    // GetAwaiter().GetResult() will avoid System.AggregateException
    ReceiveMessagesFromEventHubs().GetAwaiter().GetResult();

    Console.WriteLine("Receiving. Press enter key to stop worker.");
    Console.ReadLine();
    ```

	Here is what your Program.cs file should look like:

    ```cs
    namespace SampleReceiver
    {
        using System;
        using System.Threading.Tasks;
        using Microsoft.Azure.EventHubs;
        using Microsoft.Azure.EventHubs.Processor;

        public class Program
        {
            private const string EhConnectionString = "{Event Hubs connection string}";
            private const string EhEntityPath = "{Event Hub path/name}"
            private const string STORAGE_CONTAINER_NAME = "{Storage account container name}";
            private const string STORAGE_ACCOUNT_NAME = "{Storage account name}";
            private const string STORAGE_ACCOUNT_KEY = "{Storage account key}";

            private static readonly string storageConnectionString = string.Format("DefaultEndpointsProtocol=https;AccountName={0};AccountKey={1}", StorageAccountName, StorageAccountKey);

            public static void Main(string[] args)
            {
                // GetAwaiter().GetResult() will avoid System.AggregateException
                ReceiveMessagesFromEventHubs().GetAwaiter().GetResult();

                Console.WriteLine("Receiving. Press enter key to stop worker.");
                Console.ReadLine();
            }

            private static async Task ReceiveMessagesFromEventHubs()
            {
                var connectionSettings = new EventHubsConnectionSettings(EhConnectionString)
                {
                    EntityPath = EhEntityPath
                };

                var eventProcessorHost = new EventProcessorHost(
                    PartitionReceiver.DefaultConsumerGroupName,
                    connectionSettings,
                    storageConnectionString,
                    StorageContainerName
                );

                Console.WriteLine("Registering EventProcessor...");
                await eventProcessorHost.RegisterEventProcessorAsync<SimpleEventProcessor>();
            }
        }
    }
    ```
  
4. Run the program, and ensure that there are no errors.
  
Congratulations! You have now recieved messages from an Event Hub.

<!--Image references-->

[nuget-pkg]: ./media/service-bus-dotnet-get-started-with-queues/nuget-package.png

<!--Reference style links - using these makes the source content way more readable than using inline links-->

[github-samples]: https://github.com/Azure-Samples/azure-servicebus-messaging-samples