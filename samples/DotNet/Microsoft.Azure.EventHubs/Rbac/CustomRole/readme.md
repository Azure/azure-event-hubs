# Receive events with the Event Processor Host in .NET Standard with a custom role which grants Listen claim for Event Hubs and blob claims for Storage accounts.

**Note:** The sample in this folder uses the legacy Event Hubs library `Microsoft.Azure.EventHubs`. We strongly recommend you to use the current library `Azure.Messaging.EventHubs`. This file has instructions for both.

## Prerequisites

- [Microsoft Visual Studio 2015 or 2017](http://www.visualstudio.com).
- [.NET Core SDK](http://www.microsoft.com/net/core).
- An [Azure subscription](https://azure.microsoft.com/free/).

## Set up for custom role

1. [Create an Event Hubs namespace and an Event Hub](https://docs.microsoft.com/azure/event-hubs/event-hubs-create).
2. Create a Storage account to host a blob container, needed to store checkpoints and information required for balance load among partitions.
3. [Create a new custom role](https://docs.microsoft.com/en-us/azure/role-based-access-control/custom-roles) with the definition below.
4. [Create a new AAD (Azure Active Directory) application](https://docs.microsoft.com/en-us/azure/active-directory/develop/howto-create-service-principal-portal).
5. Assign AAD application to both Event Hubs namespace and Storage account with the custom role you just created.

```
{
    "Name": "Custom role for RBAC sample",
    "Id": "8ddab47f-cf99-4b04-8fc3-1d2d857fb931",
	"Description": "Test role",
	"IsCustom": true,
	"Actions": [
		"Microsoft.Storage/*"
],
	"NotActions": [],
	"DataActions": [
		"Microsoft.EventHub/namespaces/messages/receive/action",
		"Microsoft.Storage/*"
],
	"NotDataActions": [],
	"AssignableScopes": [
	  "/subscriptions/your-subscription-id"
	]
}
```

## Run the sample for legacy library Microsoft.Azure.EventHubs

After following the [set up steps](#set-up-for-custom-role), to run the sample in this folder to receive events, which uses the legacy library, follow these steps:

1. Clone or download this GitHub repo.
2. Run [Sender application](https://github.com/Azure/azure-event-hubs/tree/serkar.AddCustomRbacEhSample/samples/DotNet/Microsoft.Azure.EventHubs/SampleSender) to send some number of events to your Event Hub instance.
3. Run the CustomRole sample to receive those events back.

## Run the sample for current library Azure.Messaging.EventHubs

After following the [set up steps](#set-up-for-custom-role), to the run the sample to receive events using the current library, follow these steps:

1. Follow the sample in [Publish events using Azure.Messaging.EventHubs](https://github.com/Azure/azure-sdk-for-net/blob/master/sdk/eventhub/Azure.Messaging.EventHubs/samples/Sample04_PublishingEvents.md) to send some number of events to your Event Hub instance.
2. Follow the sample in [Processing events with identity-based authorization using Azure.Messaging.EventHubs.Processor](https://github.com/Azure/azure-sdk-for-net/blob/master/sdk/eventhub/Azure.Messaging.EventHubs.Processor/samples/Sample05_IdentityAndSharedAccessCredentials.md#processing-events-with-identity-based-authorization) to receive the events. To make use of the custom role, set the below environment variables that will be picked up by the `DefaultAzureCredential` used in the sample.
   - AZURE_TENANT_ID
   - AZURE_CLIENT_ID
   - AZURE_CLIENT_SECRET
