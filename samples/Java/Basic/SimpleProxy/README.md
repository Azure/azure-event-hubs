# Send/Receive events to Azure Event Hubs using Proxy

To run the sample, you need to edit the [sample code](src/main/java/com/microsoft/azure/eventhubs/samples/simpleproxy/SimpleProxy.java) and provide the following information:

```java
    final String namespaceName = "----EventHubsNamespaceName-----";
    final String eventHubName = "----EventHubName-----";
    final String sasKeyName = "-----SharedAccessSignatureKeyName-----";
    final String sasKey = "---SharedAccessSignatureKey----";
```

For proxy, install squid proxy server and set the setting below

```java
   String proxyIpAddressStr = "---proxyhostname---";
   int proxyPort = 3128;
```

#Set the Proxy Authentication details

Note: The jdk setting for disabling authentication schemes (-Djdk.http.auth.tunneling.disabledSchemes) has NO effect on BasicAuthentication support for this package version. i.e., no matter what the value of the flag -Djdk.http.auth.tunneling.disabledSchemes is - BasicAuthentication is used. azure-eventhubs package depends on the package, qpid-proton-j-extensions for authenticating with Proxy, and this package doesn't hook into the 'disabledSchemesjdk` setting.

## Prerequisites

Please refer to the [overview README](../../readme.md) for prerequisites and setting up the sample environment, including creating an Event Hubs cloud namespace and an Event Hub. 

## Build and run

The sample can be built independently with 

```bash
mvn clean package
```

and then run with (or just from VS Code or another Java IDE)

```bash
java -jar ./target/simpleproxy-1.0.0-jar-with-dependencies.jar
```