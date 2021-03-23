# Send events to Azure Event Hubs in .NET Standard

This sample shows how to write a .NET Core console application that sends a set of events to an Event Hub. You can run the solution as-is, replacing the `EhConnectionString` and `EhEntityPath` strings with your Event Hub values. The sample is also [available as a tutorial](https://docs.microsoft.com/azure/event-hubs/event-hubs-dotnet-standard-getstarted-send).

**Note:** This sample uses the legacy Event Hubs library `Microsoft.Azure.EventHubs`. We strongly encourage you to use the current library `Azure.Messaging.EventHubs`. See the [corresponding sample](https://github.com/Azure/azure-sdk-for-net/blob/master/sdk/eventhub/Azure.Messaging.EventHubs/samples/Sample04_PublishingEvents.md) that uses the current library.


## Prerequisites

* [Microsoft Visual Studio 2015 or 2017](http://www.visualstudio.com).
* [.NET Core SDK](http://www.microsoft.com/net/core).
* An Azure subscription.
* [An Event Hub namespace and an Event Hub](event-hubs-quickstart-namespace-portal.md).

## Run the sample

To run the sample, follow these steps:

1. Clone or download this GitHub repo.
2. [Create an Event Hubs namespace and an Event Hub](https://docs.microsoft.com/azure/event-hubs/event-hubs-create).
3. In Visual Studio, select **File**, then **Open Project/Soultion**. Navigate to the \azure-event-hubs\samples\DotNet\Microsoft.Azure.EventHubs\SampleSender folder.
4. Load the SampleSender.sln solution file into Visual Studio.
5. Add the [Microsoft.Azure.EventHubs](https://www.nuget.org/packages/Microsoft.Azure.EventHubs/) NuGet package to the project.
6. In Program.cs, replace the placeholders in brackets with the proper values that were obtained when creating the Event Hub. Make sure that the `Event Hubs connection string` is the namespace-level connection string, and not the Event Hub string:
    ```csharp
    private const string EhConnectionString = "Event Hubs connection string";
    private const string EhEntityPath = "Event Hub name";
    ```
7. Run the program, and ensure that there are no errors.

Congratulations! You have now sent events to an Event Hub. To receive these events, see the [SampleEphReceiver](https://github.com/Azure/azure-event-hubs/tree/master/samples/DotNet/Microsoft.Azure.EventHubs/SampleEphReceiver) sample.

