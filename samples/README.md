# Azure Event Hubs samples

**Note:** This repository holds samples for the legacy libraries for Azure Event Hubs for .NET and Java developers. We highly recommend you to upgrade to the newer packages.

.NET legacy packages

- Microsoft.Azure.EventHubs (**legacy**)
- Microsoft.Azure.EventHubs.Processor (**legacy**)
- WindowsAzure.ServiceBus (**legacy**)

Java legacy packages

- com.microsoft.azure:azure-eventhubs (**legacy**)
- com.microsoft.azure:azure-eventhubs-eph (**legacy**)

Samples for **latest** .NET packages for Azure Event Hubs

- [Azure.Messaging.EventHubs](https://docs.microsoft.com/samples/azure/azure-sdk-for-net/azuremessagingeventhubs-samples/)
- [Azure.Messaging.EventHubs.Processor](https://docs.microsoft.com/samples/azure/azure-sdk-for-net/azuremessagingeventhubsprocessor-samples/)

Samples for **latest** Java packages for Azure Event Hubs

- [com.azure:azure-messaging-eventhubs](https://github.com/Azure/azure-sdk-for-java/tree/master/sdk/eventhubs/azure-messaging-eventhubs/src/samples)
- [com.azure:azure-messaging-eventhubs-checkpointstore-blob](https://github.com/Azure/azure-sdk-for-java/tree/master/sdk/eventhubs/azure-messaging-eventhubs-checkpointstore-blob/src/samples)

## Miscellaneous

### Manage Event Hubs resources

The samples under the folder `Management` in this repository show how to manage your Event Hubs resources via CLI, the .NET package `Microsoft.Azure.Management.EventHub` and PowerShell

### proton-c-sender-dotnet-framework-receiver

[This sample](./Miscellaneous/proton-c-sender-dotnet-framework-receiver/README.md) shows how to use Azure Event Hubs with clients that use different protocols. This scenario sends using an Apache Proton C++ client, and receives using the .NET Framework client.
