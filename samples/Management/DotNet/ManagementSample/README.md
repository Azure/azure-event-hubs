# Event Hubs Management Library Sample

This sample uses the .NET Standard Event Hubs management library to show how users can dynamically create Event Hub namespaces as well as entities. The management library can be consumed by both the full .NET Framework and .NET Core applications. For more information on .NET Standard see [.NET Platforms Support](https://docs.microsoft.com/en-us/dotnet/articles/standard/library#net-platforms-support).

The management library allows Create, Read, Update, and Delete operations on the following:

* Namespaces
* Event Hub Entities
* Consumer Groups

## Prerequisites

In order to get started using the Event Hub management libraries, you must authenticate with Azure Active Directory (AAD). AAD requires that you authenticate as a Service Principal which provides access to your Azure resources. For information on creating a Service Principal follow one of these articles:  

* [Use the Azure Portal to create Active Directory application and service principal that can access resources](https://docs.microsoft.com/en-us/azure/azure-resource-manager/resource-group-create-service-principal-portal)
* [Use Azure PowerShell to create a service principal to access resources](https://docs.microsoft.com/en-us/azure/azure-resource-manager/resource-group-authenticate-service-principal)
* [Use Azure CLI to create a service principal to access resources](https://docs.microsoft.com/en-us/azure/azure-resource-manager/resource-group-authenticate-service-principal-cli)

The above tutorials will provide you with an `AppId` (Client ID), `TenantId`, and `ClientSecret` (Authentication Key), all of which will be used to authenticate by the management libraries. You must have 'Owner' permissions under *Role* for the resource group that you wish to run the sample on. Finally, when creating your Active Directory application, if you do not have a sign-on URL to input in the create step, simply input any URL format string e.g. https://contoso.org/exampleapp.

You will also have to install DotNet Core in order to run the sample.

## Running the sample
Populate the `appsettings.json` file with the appropriate values obtained from Azure Active Directory, and run the app using Visual Studio or `dotnet run`. You may find your SubscriptionId by clicking *More services* -> *Subscriptions* in the left hand nav of the Azure Portal.

```json
{
	"TenantId": "",
	"ClientId": "",
	"ClientSecret": "",
	"SubscriptionId": "",
	"DataCenterLocation": "East US"
}
```

## Required NuGet packages
In order to use the `Microsoft.Azure.Management.EventHub` package, you will also need:

* `Microsoft.Azure.Management.ResourceManager` - used to perform operations on resource groups, a required 'container' for Azure resources.
* `Microsoft.IdentityModel.Clients.ActiveDirectory` - used to authenticate with Azure Active Directory.

## Programming pattern

The pattern to manipulate any Event Hubs resource is similar and follows a common protocol:

1. Obtain a token from Azure Active Directory using the `Microsoft.IdentityModel.Clients.ActiveDirectory` library
    ```csharp
    var context = new AuthenticationContext($"https://login.windows.net/{tenantId}");

    var result = await context.AcquireTokenAsync(
        "https://management.core.windows.net/",
        new ClientCredential(clientId, clientSecret)
    );
    ```

1. Create the `EventHubManagementClient` object
    ```csharp
    var creds = new TokenCredentials(token);
    var ehClient = new EventHubManagementClient(creds)
    {
        SubscriptionId = SettingsCache["SubscriptionId"]
    };
    ```

1. Set the CreateOrUpdate parameters to your specified values
    ```csharp
    var ehParams = new Eventhub() { };
    ```

1. Execute the call
    ```csharp
    await ehClient.EventHubs.CreateOrUpdateAsync(resourceGroupName, namespaceName, EventHubName, ehParams);
    ```
