# Receive events with the Event Processor Host in .NET Standard

This sample shows how to write a .NET Core console application that receives a set of events from an event hub by using the **Event Processor Host** library. You can run the solution as-is, replacing the strings with your event hub and storage account values. The sample is also [available as a tutorial](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-dotnet-standard-getstarted-receive-eph).

**Note:** This sample uses the legacy Event Hubs library `Microsoft.Azure.EventHubs`. We strongly encourage you to use the new library `Azure.Messaging.EventHubs`. Here is the corresponding sample using the new library: [link](https://github.com/Azure/azure-sdk-for-net/blob/master/sdk/eventhub/Azure.Messaging.EventHubs.Processor/samples/Sample04_ProcessingEvents.md).

## Prerequisites

* [Microsoft Visual Studio 2015 or 2017](http://www.visualstudio.com).
* [.NET Core Visual Studio 2015 or 2017 tools](http://www.microsoft.com/net/core).
* An Azure subscription.
* [An event hub namespace and an event hub](event-hubs-quickstart-namespace-portal.md).
* An Azure Storage account.

## Run the sample

To run the sample, follow these steps:

1. Clone or download this GitHub repo.
2. [Create an Event Hubs namespace and an event hub](event-hubs-quickstart-namespace-portal.md).
3. In Visual Studio, select **File**, then **Open Project/Solution**. Navigate to the \azure-event-hubs\samples\DotNet\Microsoft.Azure.EventHubs\SampleEphReceiver folder.
4. Load the SampleEphReceiver.sln solution file into Visual Studio.
5. Add the [Microsoft.Azure.EventHubs](https://www.nuget.org/packages/Microsoft.Azure.EventHubs/) and [Microsoft.Azure.EventHubs.Processor](https://www.nuget.org/packages/Microsoft.Azure.EventHubs.Processor/) NuGet packages to the project.
6. In Program.cs, replace the following constants with the corresponding values for the event hub connection string, event hub name:
    ```csharp
    private const string EventHubConnectionString = "Event Hubs connection string";
    private const string EventHubName = "Event Hub name";    
    ```
7. Create a Storage account to host a blob container, needed for lease management by the Event Processor Host. 
8. In Program.cs, replace the storage account container name, storage account name, and storage account key (the container will be created if not present):
```
    private const string StorageContainerName = "Storage account container name";
    private const string StorageAccountName = "Storage account name";
    private const string StorageAccountKey = "Storage account key";
```
9. Run the program, and ensure that there are no errors.

Congratulations! You have now received events from an event hub by using the Event Processor Host. To send events, see the [SampleSender](https://github.com/Azure/azure-event-hubs/tree/master/samples/DotNet/Microsoft.Azure.EventHubs/SampleSender) sample.
