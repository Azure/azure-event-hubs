# Receive events with the Event Processor Host in .NET Standard with a custom role which grants Listen claim for Event Hubs and blob claims for Storage accounts.

## Prerequisites

* [Microsoft Visual Studio 2015 or 2017](http://www.visualstudio.com).
* [.NET Core Visual Studio 2015 or 2017 tools](http://www.microsoft.com/net/core).
* An Azure subscription.
* [An event hub namespace and an event hub](event-hubs-quickstart-namespace-portal.md).
* An Azure Storage account.

## Run the sample

To run the sample, follow these steps:

1. Clone or download this GitHub repo.
2. [Create an Event Hubs namespace and an event hub](https://docs.microsoft.com/azure/event-hubs/event-hubs-create).
3. Create a Storage account to host a blob container, needed for lease management by the Event Processor Host.
3. [Create a new custom role](https://docs.microsoft.com/en-us/azure/role-based-access-control/custom-roles) with the definition below.
4. [Create a new AAD (Azure Active Directory) application](https://docs.microsoft.com/en-us/azure/active-directory/develop/howto-create-service-principal-portal).
5. Assign AAD application to both Event Hubs namespace and Storage account with the custom role you just created.

```{
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
}```

6. Update the sample with Event Hubs namespace and Storage account name.
7. Run [Sender application](https://github.com/Azure/azure-event-hubs/tree/serkar.AddCustomRbacEhSample/samples/DotNet/Microsoft.Azure.EventHubs/SampleSender) to send some number of messages to your event hub.
8. Run the CustomRole sample to receive those events back.
