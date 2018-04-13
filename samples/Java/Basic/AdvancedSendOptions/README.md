# Send events to Azure Event Hubs using Java

The [Send events to Azure Event Hubs using Java](https://docs.microsoft.com/azure/event-hubs/event-hubs-java-get-started-send) tutorial walks you through ingesting into your event hub using Java with this code.
This sample shows the various options that are available with Events Hubs for the publishers to ingest events.
To run the sample, you need to edit the [sample code](src/main/java/com/microsoft/azure/eventhubs/samples/advancedsendoptions/AdvancedSendOptions.java) and provide the following information:

```java
    final String namespaceName = "----EventHubsNamespaceName-----";
    final String eventHubName = "----EventHubName-----";
    final String sasKeyName = "-----SharedAccessSignatureKeyName-----";
    final String sasKey = "---SharedAccessSignatureKey----";
```

## Prerequisites

Please refer to the [overview README](../../readme.md) for prerequisites and setting up the sample environment, including creating an Event Hubs cloud namespace and an Event Hub. 

## Build and run

The sample can be built independently with 

```bash
mvn clean package
```

and then run with (or just from VS Code or another Java IDE)

```bash
java -jar ./target/send-1.0.0-jar-with-dependencies.jar
```