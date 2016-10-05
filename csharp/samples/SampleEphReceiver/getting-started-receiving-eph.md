# Get started receiving messages with the EventProcessorHost in .NET Core

## What will be accomplished

This tutorial will walk-through how to create the existing solution **SampleEphReceiver** (inside this folder). You can run the solution as-is replacing the EhConnectionString/EhEntityPath/StorageAccount settings with your Event Hub and storage account values, or follow this tutorial to create your own.

In this tutorial, we will write a .NET Core console application to receive messages from an Event Hub using the **EventProcessorHost**.

## Prerequisites

1. [Visual Studio 2015](http://www.visualstudio.com).

2. [.NET Core Visual Studio 2015 Tooling](http://www.microsoft.com/net/core).

3. An Azure subscription.

4. An Event Hubs namespace.
    
## Receive messages from the Event Hub

### Create a console application

1. Launch Visual Studio and create a new .NET Core console application.

### Add the Event Hubs NuGet package

1. Right-click the newly created project and select **Manage NuGet Packages**.

2. Click the **Browse** tab, then search for “Microsoft Azure Event Processor Host” and select the **Microsoft Azure Event Processor Host** item. Click **Install** to complete the installation, then close this dialog box.

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
            public Task CloseAsync(PartitionContext context, CloseReason reason)
            {
                Console.WriteLine($"Processor Shutting Down. Partition '{context.PartitionId}', Reason: '{reason}'.");
                return Task.FromResult<object>(null);
            }

            public Task OpenAsync(PartitionContext context)
            {
                Console.WriteLine($"SimpleEventProcessor initialized.  Partition: '{context.PartitionId}'");
                return Task.FromResult<object>(null);
            }

            public Task ProcessErrorAsync(PartitionContext context, Exception error)
            {
                Console.WriteLine($"Error on Partition: {context.PartitionId}, Error: {error.Message}");
                return Task.FromResult<object>(null);
            }

            public async Task ProcessEventsAsync(PartitionContext context, IEnumerable<EventData> messages)
            {
                foreach (var eventData in messages)
                {
                    var data = Encoding.UTF8.GetString(eventData.Body.Array, eventData.Body.Offset, eventData.Body.Count);
                    Console.WriteLine($"Message received.  Partition: '{context.PartitionId}', Data: '{data}'");
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
3. Add a local variable for the `EventProcessorHost`, like the following:

    ```cs
    private static EventProcessorHost eventProcessorHost;
    ```

4. Add a new method to the `Program` class called `StartHost`, like the following:

    ```cs
    private static Task StartHost()
    {
        eventProcessorHost = new EventProcessorHost(
            PartitionReceiver.DefaultConsumerGroupName,
            EhConnectionString,
            storageConnectionString,
            StorageContainerName
        );

        Console.WriteLine("Registering EventProcessor...");
        return eventProcessorHost.RegisterEventProcessorAsync<SimpleEventProcessor>();
    }
    ```

5. Add a new method to the `Program` class called `StopHost`, like the following:

    ```cs
    private static Task StopHost()
    {
        return eventProcessorHost?.UnregisterEventProcessorAsync();
    }
    ```

6. Add the following code to the `Main` method:

    ```cs
    StartHost().Wait();

    Console.WriteLine("Receiving. Press enter key to stop worker.");
    Console.ReadLine();
    StopHost().Wait();
    ```

	Here is what your Program.cs file should look like:

    ```cs
    namespace SampleEphReceiver
    {
        using System;
        using System.Threading.Tasks;
        using Microsoft.Azure.EventHubs;
        using Microsoft.Azure.EventHubs.Processor;

        public class Program
        {
            private const string EhConnectionString = "{Event Hubs connection string}";
            private const string EhEntityPath = "{Event Hub path/name}";
            private const string StorageContainerName = "{Storage account container name}";
            private const string StorageAccountName = "{Storage account name}";
            private const string StorageAccountKey = "{Storage account key}";

            private static readonly string storageConnectionString = string.Format("DefaultEndpointsProtocol=https;AccountName={0};AccountKey={1}", StorageAccountName, StorageAccountKey);

            private static EventProcessorHost eventProcessorHost;

            public static void Main(string[] args)
            {
                StartHost().Wait();

                Console.WriteLine("Receiving. Press enter key to stop worker.");
                Console.ReadLine();
                StopHost().Wait();
            }

            private static Task StartHost()
            {
                eventProcessorHost = new EventProcessorHost(
                    PartitionReceiver.DefaultConsumerGroupName,
                    EhConnectionString,
                    storageConnectionString,
                    StorageContainerName
                );

                Console.WriteLine("Registering EventProcessor...");
                return eventProcessorHost.RegisterEventProcessorAsync<SimpleEventProcessor>();
            }

            private static Task StopHost()
            {
                return eventProcessorHost?.UnregisterEventProcessorAsync();
            }
        }
    }
    ```
  
7. Run the program, and ensure that there are no errors.
  
Congratulations! You have now recieved messages from an Event Hub.