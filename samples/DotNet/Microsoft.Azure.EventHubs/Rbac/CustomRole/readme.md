# Receive events with the Event Processor Host in .NET Standard with a custom role which grants Listen claim for Event Hubs and blob claims for Storage accounts.


## Run the sample

**Note:** This sample uses the legacy Event Hubs library `Microsoft.Azure.EventHubs`. We strongly recommend you to use the current library `Azure.Messaging.EventHubs`. 

See [sample](https://github.com/Azure/azure-sdk-for-net/blob/master/sdk/identity/Azure.Identity/samples/DefiningCustomCredentialTypes.md#authenticating-with-the-on-behalf-of-flow) that uses the new library to authenticate after the environment variables for client id and secret are set. 

To run the sample in this folder, which uses the legacy library, follow these steps:

1. Clone or download this GitHub repo.
2. [Create an Event Hubs namespace and an Event Hub](https://docs.microsoft.com/azure/event-hubs/event-hubs-create).
3. Create a Storage account to host a blob container, needed for lease management by the Event Processor Host.
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

6. Update the sample with Event Hubs namespace and Storage account name.
7. Run [Sender application](https://github.com/Azure/azure-event-hubs/tree/serkar.AddCustomRbacEhSample/samples/DotNet/Microsoft.Azure.EventHubs/SampleSender) to send some number of messages to your Event Hub.
8. Run the CustomRole sample to receive those events back.


## Prerequisites

* [Microsoft Visual Studio 2015 or 2017](http://www.visualstudio.com).
* [.NET Core SDK](http://www.microsoft.com/net/core).
* An Azure subscription.
* [An Event Hub namespace and an Event Hub](event-hubs-quickstart-namespace-portal.md).
* An Azure Storage account.


