# Role based access sample with Azure Event Hubs Java SDK

For general information on using Role based access (RBAC) with Azure Event Hubs, see the [documentation](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-role-based-access-control).

This sample uses the [Microsoft Authentication Library (MSAL) for Java](https://github.com/AzureAD/microsoft-authentication-library-for-java) to obtain tokens from Azure Active Directory (AAD).

## Prerequisites

Please refer to the Java sample overview README for setting up the sample environment, including creating an Event Hubs cloud namespace and an event hub. 

The specific AAD pattern used in this sample is ["Authenticate an appliction"](https://docs.microsoft.com/en-us/azure/event-hubs/authenticate-application). Please follow the steps described to
create an application (client) id and application (client) secret, obtain your directory (tenant) id, and assign the application the Data Owner role on your event hub.

Once you have performed the previous steps, edit SendReceive.java to provide the necessary information. 

```java
    final java.net.URI namespace = new java.net.URI("----EventHubsNamespace---.servicebus.windows.net"); // to target National clouds, change domain name too
    final String eventhub = "----EventHubName----";
    final String authority = "https://login.windows.net/----replaceWithTenantIdGuid----";
    final String clientId = "----replaceWithClientIdGuid----"; // not needed to run with Managed Identity
    final String clientSecret = "----replaceWithClientSecret----"; // not needed to run with Managed Identity
```

The Azure Event Hubs Java SDK also has limited built-in support for Managed Identity: specifically, when running in a virtual machine with a system-assigned managed identity, the SDK can
obtain and use that identity to perform role based access. This sample can demonstrate that ability when run in an
[appropriately-configured virtual machine](https://docs.microsoft.com/en-us/azure/active-directory/managed-identities-azure-resources/qs-configure-portal-windows-vm) and the managed identity
[has been assigned the Data Owner role on your event hub.](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-managed-service-identity)


## Build and run

The sample can be built independently with 

```bash
mvn clean package
```

and then run with (or just from VS Code or another Java IDE)

```bash
java -jar ./target/send-1.0.0-jar-with-dependencies.jar
```