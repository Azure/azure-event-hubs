# Get started sending messages to Event Hubs in .NET Core

**Note**: This sample is also available as a tutorial [here](https://docs.microsoft.com/azure/event-hubs/event-hubs-dotnet-standard-getstarted-send).

This sample shows how to write a .NET Core console application that sends a set of messages to an Event Hub. You can run the solution as-is, replacing the `EhConnectionString` and `EhEntityPath` strings with your Event Hub values, or you can follow the steps in the [tutorial](https://docs.microsoft.com/azure/event-hubs/event-hubs-dotnet-standard-getstarted-send) to create your own.

## Prerequisites

1. [Visual Studio 2015](http://www.visualstudio.com).
2. [.NET Core Visual Studio 2015 tool](http://www.microsoft.com/net/core).
3. An Azure subscription.
4. An Event Hubs namespace.

### Create an Event Hubs namespace and an Event Hub

The first step is to use the [Azure portal](https://portal.azure.com) to create a namespace of type Event Hubs, and obtain the management credentials your application needs to communicate with the Event Hub. To create a namespace and Event Hub, follow the procedure in [this article](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-create).
