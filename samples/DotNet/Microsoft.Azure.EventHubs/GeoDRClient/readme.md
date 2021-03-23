# Microsoft Azure Event Hubs Geo-DR

To learn more about Azure Event Hubs, please visit our [marketing page](https://azure.microsoft.com/services/event-hubs/).

To learn more about our Geo-DR feature in general please follow [this](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-geo-dr) link.

Customers, who want to minimize the disruption in operations caused by regional failures in Azure. The Geo-disaster recovery (Geo-DR) feature shown here aims to provide richer customer controlled failover capabilities for all Event Hubs customers. For an overview on this feature,refer to the article – Enabling Geo-Disaster Recovery for Event Hubs.

This sample shows how to: 

1. Achieve Geo-DR for an Event Hubs namespace. 
2. Create a namespace with live metadata replication between two customer chosen regions

## Getting Started
### Prerequisites

In order to get started using the sample (as it uses the Event Hubs management libraries), you must authenticate with Azure Active Directory (AAD). This requires you to authenticate as a Service Principal, which provides access to your Azure resources. 
To obtain a service principal please do the following steps:

1. Go to the Azure Portal and select Azure Active Directory in the left menu.
2. Create a new Application under App registrations / + New application registration.
	1. The application should be of type Web app / API.
	2. You can provide any URL for your application as sign on URL.
	3. Navigate to your newly created application
3. Application or AppId is the client Id. Note it down as you will need it for the sample.
4. Select keys and create a new key. Note down the Value as you won't be able to access it later.
5. Go back to the root Azure Active Directory and select properties.
	1. Note down the Directory ID as this is your TenantId.
6. You must have ‘Owner’ permissions under Role for the resource group that you wish to run the sample on. Regardless if you want to use an existing namespace or create a new one, make sure to add the newly created application as owner under Access Control (IAM).

For more information on creating a Service Principal, refer to the following articles:

*	[Use the Azure Portal to create Active Directory application and service principal that can access resources](https://docs.microsoft.com/azure/azure-resource-manager/resource-group-create-service-principal-portal)
*	[Use Azure PowerShell to create a service principal to access resources](https://docs.microsoft.com/azure/azure-resource-manager/resource-group-authenticate-service-principal)
*	[Use Azure CLI to create a service principal to access resources](https://docs.microsoft.com/azure/azure-resource-manager/resource-group-authenticate-service-principal-cli)

<!-- The above articles helps you to obtain an AppId (ClientId), TenantId, and ClientSecret (Authentication Key), all of which are required to authenticate the management libraries.  Finally, when creating your Active Directory application, if you do not have a sign-on URL to input in the create step, simply input any URL format string e.g. https://contoso.org/exampleapp -->

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
For creating a paired region. After this, you should see metadata (i.e. Event Hubs, consumer groups, throughput units etc. replicated to the secondary namespace).

*	FailOver
Simulating a failover. After this action, the secondary namespace becomes the primary

*	BreakPairing
For breaking the pairing between a primary and secondary namespace

*	DeleteAlias
For deleting an alias, that contains information about the primary-secondary pairing

*	GetConnectionStrings
In a Geo DR enabled namespace, the Event Hubs should be accessed only via the alias. This is because, the alias can point to either the primary Event Hub or the failed over Event Hub. This way, the user does not have to adjust the connection strings in his/her apps to point to a different Event Hub in the case of a failover.

Examples
*	EventHubsGeoDRManagementSample.exe CreatePairing GeoDRSampleConfig.json
*	EventHubsGeoDRManagementSample.exe FailOver GeoDRSampleConfig.json
*	EventHubsGeoDRManagementSample.exe BreakPairing GeoDRSampleConfig.json
*	EventHubsGeoDRManagementSample.exe DeleteAlias GeoDRSampleConfig.json
*	EventHubsGeoDRManagementSample.exe GetConnectionStrings GeoDRSampleConfig.json

## What is the disaster recovery workflow for Event Hubs?
The following section describes the steps for performing Geo-diaster recovery,

### Step1: Create the namespaces and establish a geo-pairing

1.	Select the active region and create the primary namespace.
2.	Select the passive region and create the secondary namespace. The following guidelines apply to the secondary namespace:
	    a. The secondary namespace must not exist at the time you create the pairing.
	    b. The secondary namespace must be the same type and SKU as the primary namespace.
	    c. The two namespaces cannot be in the same region.
	    d. Changing the names of an alias is not allowed.
	    e. Changing the secondary namespace is not allowed.
3.	Create an alias and provide the primary and secondary namespaces to complete the pairing.
4.	Get the required connection strings on the alias to connect to your Event Hubs.
5.	Once the namespaces are paired with an alias, the metadata is replicated periodically in both namespaces.

**Note:** Creating a pairing, failing over, breaking the pairing, deleting the alias have all retries build in. All before mentioned operations will retry 10 times with 10 minutes in between each attempt.

### Step2: Initiate a failover
After this step, the seconday namespace becomes the primary namespace.

1.	Initiate a fail-over. This step is only performed on the secondary namespace. The geo-pairing is broken and the alias now points to the secondary namespace. **Note:** The Failover can take a few minutes to complete.
2.	Senders and receivers still connect to the Event Hubs using the alias. The failover does not disrupt the connection.
3.	Because the pairing is broken, the old primary namespace no longer has a replication status associated with it.
4.	The metadata synchronization between the primary and secondary namespaces also stops

### Step3: Other operations (optional)
You can optionally choose to break the geo-pairing or delete the alias. This step stops the meta-data synchronization between the primary and the secondary namespaces. Sometimes it can take 1 - 2 minutes for the pairing to be disabled.

**Note:** To delete the alias, you must break the geo-pairing first. Once the breaking of pairs succeeds, you can proceed with deleting the alias. At this point, the connection strings for the alias are also deleted. If you want to delete the alias after a failover you will need to adjust this line of code:

    await client.DisasterRecoveryConfigs.DeleteAsync(config.PrimaryResourceGroupName, config.PrimaryNamespace, config.Alias);

to look like:

    await client.DisasterRecoveryConfigs.DeleteAsync(config.SecondaryResourceGroupName, config.SecondaryNamespace, config.Alias);

## How to provide feedback
See our [Contributor guidelines](https://github.com/Azure/azure-event-hubs/blob/master/.github/CONTRIBUTING.md)

