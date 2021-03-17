# Role based access sample with Microsoft.Azure.EventHubs SDK #

For more information on Role based access (RBAC) and how to run this sample follow [this](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-role-based-access-control) link.

**Note**: This sample uses the legacy Event Hubs library `Microsoft.Azure.EventHubs`. We strongly recommend you to use the new library `Azure.Messaging.EventHubs`. 

Leveraging `Azure.Identity`, the new library simplifies the authentication workflow with `DefaultAzureCredential`. Under the hood, `DefaultAzureCredential` combines commonly used credentials, such as managed identity, authentication information set via environment variables or from a logged in Visual Studio Code Azure Account, to authenticate. It attempts to authenticate using the credentials in a specific order. See [here](https://github.com/Azure/azure-sdk-for-net/tree/master/sdk/identity/Azure.Identity#azure-identity-client-library-for-net) for more details.