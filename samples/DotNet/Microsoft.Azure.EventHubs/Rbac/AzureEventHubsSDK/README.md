# Role based access sample with Microsoft.Azure.EventHubs SDK #

For more information on Role based access (RBAC) and how to run this sample follow [this](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-role-based-access-control) link.

**Note**: This sample uses the legacy Event Hubs library `Microsoft.Azure.EventHubs`. 
We strongly recommend you to use the current version of the library `Azure.Messaging.EventHubs`. 

Authorization was one of the most consistent areas of feedback for the Azure SDKs from our customers. Libraries for different Azure services did not have a consistent approach to authorization, roles, etc. The APIs also did not offer a good, approachable, and consistent story for integrating with Azure Active Directory principals. As such, many developers felt that the learning curve was difficult.

The [Azure.Identity](https://github.com/Azure/azure-sdk-for-net/tree/master/sdk/identity/Azure.Identity#azure-identity-client-library-for-net) library is intended to address that feedback and provide an approachable and flexible authentication experience across the Azure SDKs, including the `Azure.Messaging.EventHubs` one.
