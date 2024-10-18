# Receive events with the Event Processor Host in .NET Standard with a custom role which grants Listen claim for Event Hubs and blob claims for Storage accounts.

Here is the custom role definition you can use to run this sample. Once created, assign your custom role to Storage and Event Hubs resources.

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
