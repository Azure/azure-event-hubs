# Microsoft Azure Event Hubs Geo-DR (preview)

To learn more about Azure Event Hubs, please visit our [marketing page](https://azure.microsoft.com/services/event-hubs/).

Customers want to minimize the disruption in operations caused by regional failures (transient or permanent) in Azure. The Geo-disaster recovery (Geo-DR) feature shown here aims to provide richer customer controlled failover capabilities for all Event Hubs customers. For an overview on this feature,refer to the article – Enabling Geo-Disaster Recovery for Event Hubs.

This sample shows how to 
a.	Achieve Geo-DR for an Event Hubs namespace. 
b.	Create a namespace with live metadata replication between two customer chosen regions

## Getting Started
### Prerequisites

In order to get started using the sample (as it uses the Event Hubs management libraries), you must authenticate with Azure Active Directory (AAD). This requires you to authenticate as a Service Principal, which provides access to your Azure resources. For information on creating a Service Principal, refer to the following article:

*	[Use the Azure Portal to create Active Directory application and service principal that can access resources](https://docs.microsoft.com/azure/azure-resource-manager/resource-group-create-service-principal-portal)
*	[Use Azure PowerShell to create a service principal to access resources](https://docs.microsoft.com/azure/azure-resource-manager/resource-group-authenticate-service-principal)
*	[Use Azure CLI to create a service principal to access resources](https://docs.microsoft.com/azure/azure-resource-manager/resource-group-authenticate-service-principal-cli)

The above articles helps you to obtain an AppId (ClientId), TenantId, and ClientSecret (Authentication Key), all of which are required to authenticate the management libraries. You must have ‘Owner’ permissions under Role for the resource group that you wish to run the sample on. Finally, when creating your Active Directory application, if you do not have a sign-on URL to input in the create step, simply input any URL format string e.g. https://contoso.org/exampleapp

### Required NuGet packages

1.	Microsoft.Azure.Management.EventHub
2.	Microsoft.IdentityModel.Clients.ActiveDirectory - used to authenticate with AAD

### Running the sample

1.	This requires Visual Studio 2017
2.	Provision the required resources using the template – Deploy Geo-DR resources for a namespace.
3.	Populate GeoDRSampleConfig.json accordingly. This file is included in the Visual Studio solution.
4.	Build the solution
5.	The exe takes two arguments: <Geo DR action> <Config file with Azure resource details>

The Geo DR actions could be

*	CreatePairing
For creating a paired region. After this, you should see metadata (i.e. event hubs, consumer groups, throughput units etc. replicated to the secondary namespace).

*	FailOver
Simulating a failover. After this action, the secondary namespace becomes the primary

*	BreakPairing
For breaking the pairing between a primary and secondary namespace

*	DeleteAlias
For deleting an alias, that contains information about the primary-secondary pairing

*	GetConnectionStrings
In a Geo DR enabled namespace, the Event Hubs can be accessed only via the alias. This is because, the alias can point to either the primary event hub or the failed over event hub. This way, the user does not have to adjust the connection strings in his/her apps to point to a different event hub in the case of a failover.

Examples
*	EventHubsGeoDRManagementSample.exe CreatePairing GeoDRSampleConfig.json
*	EventHubsGeoDRManagementSample.exe FailOver GeoDRSampleConfig.json
*	EventHubsGeoDRManagementSample.exe BreakPairing GeoDRSampleConfig.json
*	EventHubsGeoDRManagementSample.exe DeleteAlias GeoDRSampleConfig.json
*	EventHubsGeoDRManagementSample.exe GetConnectionStrings GeoDRSampleConfig.json

## How to provide feedback
See our [Contributor guidelines](https://github.com/Azure/azure-event-hubs/blob/master/.github/CONTRIBUTING.md)

