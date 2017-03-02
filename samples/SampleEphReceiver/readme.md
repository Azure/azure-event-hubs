# Get started receiving messages with the EventProcessorHost in .NET Core

**Note**: This sample is also available as a tutorial [here](https://docs.microsoft.com/azure/event-hubs/event-hubs-dotnet-standard-getstarted-receive-eph).

This sample shows how to write a .NET Core console application that receives messages from an Event Hub using the **EventProcessorHost**. You can run the solution as-is, replacing the placeholder strings with your Event Hub and storage account values, or you can follow the steps in the [tutorial](https://docs.microsoft.com/azure/event-hubs/event-hubs-dotnet-standard-getstarted-receive-eph) to create your own.

## Prerequisites

1. [Visual Studio 2015](http://www.visualstudio.com).
2. [.NET Core Visual Studio 2015 tools](http://www.microsoft.com/net/core).
3. An Azure subscription.
4. An Event Hubs namespace.
5. An Azure storage account.

### Create an Azure Storage account
1. Log on to the [Azure portal](https://portal.azure.com).
2. In the left navigation pane of the portal, click **New**, then click **Storage**, and then click **Storage Account**.
3. Complete the fields in the storage account blade and then click **Create**.
4. After you see the **Deployments Succeeded** message, click the name of the new storage account and in the **Essentials** blade, click **Blobs**. When the **Blob service** blade opens, click **+ Container** at the top. Name the container **archive**, then close the **Blob service** blade.
5. Click **Access keys** in the left blade and copy the name of the storage container, the storage account, and the value of **key1**. Save these values to Notepad or some other temporary location.

### Create an Event Hubs namespace and an Event Hub

The next step is to use the [Azure portal](https://portal.azure.com) to create a namespace of type Event Hubs, and obtain the management credentials your application needs to communicate with the Event Hub. To create a namespace and Event Hub, follow the procedure in [this article](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-create).
    