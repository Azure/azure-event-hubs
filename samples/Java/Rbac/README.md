# Role based access sample with Azure Event Hubs Java SDK

For general information on using Role based access (RBAC) with Azure Event Hubs, see the [documentation](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-role-based-access-control).

This sample uses the [Microsoft Authentication Library (MSAL) for Java](https://github.com/AzureAD/microsoft-authentication-library-for-java) to obtain tokens from Azure Active Directory (AAD).

## Prerequisites

Please refer to the [overview README](../readme.md) for setting up the sample environment, including creating an Event Hubs cloud namespace and an event hub. 

The specific AAD pattern used in this sample is ["Authenticate an appliction"](https://docs.microsoft.com/en-us/azure/event-hubs/authenticate-application). Please follow the steps described to
create an application (client) id and application (client) secret, obtain your directory (tenant) id, and give the application Data Owner access to your event hub.

Once you have performed the previous steps, edit SendReceive.java to provide the necessary information. 

```java
    final java.net.URI namespace = new java.net.URI("YourEventHubsNamespace.servicebus.windows.net");
    final String eventhub = "Your event hub";
    final String authority = "https://login.windows.net/replaceWithTenantIdGuid";
    final String clientId = "replaceWithClientIdGuid";
    final String clientSecret = "replaceWithClientSecret";
```

## Build and run

The sample can be built independently with 

```bash
mvn clean package
```

and then run with (or just from VS Code or another Java IDE)

```bash
java -jar ./target/send-1.0.0-jar-with-dependencies.jar
```