# Get started sending messages to Event Hubs in .NET Core

## What will be accomplished

This tutorial will walk-through how to create the existing solution **SampleSender** (inside this folder). You can run the solution as-is replacing the EhConnectionString/EhEntityPath with your Event Hub values, or follow this tutorial to create your own.

In this tutorial, we will write a .NET Core console application to send messages to an Event Hub.

## Prerequisites

1. [Visual Studio 2015](http://www.visualstudio.com).

2. [.NET Core Visual Studio 2015 Tooling](http://www.microsoft.com/net/core).

3. An Azure subscription.

4. An Event Hubs namespace.

## Send messages to an Event Hub

To send messages to an Event Hub, we will write a C# console application using Visual Studio.

### Create a console application

1. Launch Visual Studio and create a new .NET Core console application.

### Add the Event Hubs NuGet package

1. Right-click the newly created project and select **Manage NuGet Packages**.

2. Click the **Browse** tab, then search for “Microsoft Azure Event Hubs” and select the **Microsoft Azure Event Hubs** item. Click **Install** to complete the installation, then close this dialog box.

### Write some code to send messages to the Event Hub

1. Add the following `using` statement to the top of the Program.cs file.

    ```cs
    using Microsoft.Azure.EventHubs;
    ```

2. Add constants to the `Program` class for the Event Hubs connection string and entity path (individual Event Hub name). Replace the placeholders in brackets with the proper values that were obtained when creating the Event Hub.

    ```cs
    private const string EhConnectionString = "{Event Hubs connection string}";
    private const string EhEntityPath = "{Event Hub path/name}";
    ```

3. Add a new method to the `Program` class like the following:

    ```cs
    private static async Task SendMessagesToEventHub(int numMessagesToSend)
    {
        // Creates an EventHubsConnectionSettings object from a the connection string, and sets the EntityPath.
        // Typically the connection string should have the Entity Path in it, but for the sake of this simple scenario
        // we are using the connection string from the namespace.
        var connectionSettings = new EventHubsConnectionSettings(EhConnectionString)
        {
            EntityPath = EhEntityPath
        };

        var eventHubClient = EventHubClient.Create(connectionSettings);

        for (var i = 0; i < numMessagesToSend; i++)
        {
            try
            {
                var message = $"Message {i}";
                Console.WriteLine($"Sending message: {message}");
                await eventHubClient.SendAsync(new EventData(Encoding.UTF8.GetBytes(message)));
            }
            catch (Exception exception)
            {
                Console.WriteLine($"{DateTime.Now} > Exception: {exception.Message}");
            }

            await Task.Delay(10);
        }

        Console.WriteLine($"{numMessagesToSend} messages sent.");
    }
    ```

4. Add the following code to the `Main` method in the `Program` class.

    ```cs
    SendMessagesToEventHub(100).Wait();
    Console.WriteLine("Press any key to exit.");
    Console.ReadLine();
    ```

    Here is what your Program.cs should look like.

    ```cs
    namespace SampleSender
    {
        using System;
        using System.Text;
        using System.Threading.Tasks;
        using Microsoft.Azure.EventHubs;

        public class Program
        {
            private const string EhConnectionString = "{Event Hubs connection string}";
            private const string EhEntityPath = "{Event Hub path/name}";

            public static void Main(string[] args)
            {
                SendMessagesToEventHub(100).Wait();
                Console.WriteLine("Press any key to exit.");
                Console.ReadLine();
            }

            // Creates an Event Hub client and sends 100 messages to the event hub.
            private static async Task SendMessagesToEventHub(int numMessagesToSend)
            {
                // Creates an EventHubsConnectionSettings object from a the connection string, and sets the EntityPath.
                // Typically the connection string should have the Entity Path in it, but for the sake of this simple scenario
                // we are using the connection string from the namespace.
                var connectionSettings = new EventHubsConnectionSettings(EhConnectionString)
                {
                    EntityPath = EhEntityPath
                };

                var eventHubClient = EventHubClient.Create(connectionSettings);

                for (var i = 0; i < numMessagesToSend; i++)
                {
                    try
                    {
                        var message = $"Message {i}";
                        Console.WriteLine($"Sending message: {message}");
                        await eventHubClient.SendAsync(new EventData(Encoding.UTF8.GetBytes(message)));
                    }
                    catch (Exception exception)
                    {
                        Console.WriteLine($"{DateTime.Now} > Exception: {exception.Message}");
                    }

                    await Task.Delay(10);
                }

                Console.WriteLine($"{numMessagesToSend} messages sent.");
            }
        }
    }
    ```
  
5. Run the program, and ensure that there are no errors thrown.
  
Congratulations! You have now created an Event Hub and sent messages to it.