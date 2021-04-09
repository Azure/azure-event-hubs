# Azure Event Hubs samples

This repository holds samples for the below libraries that can be used to interact with Azure Event Hubs for .NET and Java developers.

.NET packages

- Microsoft.Azure.EventHubs (**legacy**)
- Microsoft.Azure.EventHubs.Processor (**legacy**)
- WindowsAzure.ServiceBus (**legacy**)
- Microsoft.Azure.Management.EventHub

Java packages

- com.microsoft.azure:azure-eventhubs (**legacy**)
- com.microsoft.azure:azure-eventhubs-eph (**legacy**)

We highly recommend to upgrading newer libraries for Azure Event Hubs that are available as of February 2020.

Samples for **latest** .NET packages for Azure Event Hubs

- [Azure.Messaging.EventHubs](https://docs.microsoft.com/samples/azure/azure-sdk-for-net/azuremessagingeventhubs-samples/)
- [Azure.Messaging.EventHubs.Processor](https://docs.microsoft.com/samples/azure/azure-sdk-for-net/azuremessagingeventhubsprocessor-samples/)

Samples for **latest** Java packages for Azure Event Hubs

- [com.azure:azure-messaging-eventhubs](https://github.com/Azure/azure-sdk-for-java/tree/master/sdk/eventhubs/azure-messaging-eventhubs/src/samples)
- [com.azure:azure-messaging-eventhubs-checkpointstore-blob](https://github.com/Azure/azure-sdk-for-java/tree/master/sdk/eventhubs/azure-messaging-eventhubs-checkpointstore-blob/src/samples)

## Miscellaneous

### proton-c-sender-dotnet-framework-receiver

[This sample](./Miscellaneous/proton-c-sender-dotnet-framework-receiver/README.md) shows how to use Azure Event Hubs with clients that use different protocols. This scenario sends using an Apache Proton C++ client, and receives using the .NET Framework client.
