# Send events to Azure Event Hubs in .NET Standard

This sample shows how to write a .NET Core console application that sends a set of events to an event hub. You can run the solution as-is, replacing the `EhConnectionString` and `EhEntityPath` strings with your event hub values. The sample is also [available as a tutorial](https://docs.microsoft.com/azure/event-hubs/event-hubs-dotnet-standard-getstarted-send).

## Prerequisites

* [Microsoft Visual Studio 2015 or 2017](http://www.visualstudio.com).
* [.NET Core Visual Studio 2015 or 2017 tools](http://www.microsoft.com/net/core).
* An Azure subscription.
* [An event hub namespace and an event hub](event-hubs-quickstart-namespace-portal.md).

## Run the sample

To run the sample, follow these steps:

1. Clone or download this GitHub repo.
2. [Create an Event Hubs namespace and an event hub](event-hubs-quickstart-namespace-portal.md).
3. In Visual Studio, select **File**, then **Open Project/Soultion**. Navigate to the \azure-event-hubs\samples\DotNet\Microsoft.Azure.EventHubs\SampleSender folder.
4. Load the SampleSender.sln solution file into Visual Studio.
5. Add the [Microsoft.Azure.EventHubs](https://www.nuget.org/packages/Microsoft.Azure.EventHubs/) NuGet package to the project.
6. In Program.cs, replace the placeholders in brackets with the proper values that were obtained when creating the event hub. Make sure that the `Event Hubs connection string` is the namespace-level connection string, and not the event hub string:
    ```csharp
    private const string EhConnectionString = "Event Hubs connection string";
    private const string EhEntityPath = "Event Hub name";
    ```
7. Run the program, and ensure that there are no errors.

Congratulations! You have now sent events to an event hub. To receive these events, see the [SampleEphReceiver](https://github.com/Azure/azure-event-hubs/tree/master/samples/DotNet/Microsoft.Azure.EventHubs/SampleEphReceiver) sample.

