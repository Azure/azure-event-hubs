# Receive events from Azure Event Hubs using Java

**Note**: This sample is available as a tutorial [here](https://docs.microsoft.com/azure/event-hubs/event-hubs-java-get-started-receive-eph).

The tutorial walks you through the consuming/receiving events from event hub. The sample uses Event Processor Host for consuming the events. Events in an event hub are distributed in various partitions to achieve parallelism while consuming. EPH simplifies processing events from event hub by managing persistent checkpoints and parallel receives. 

To run the sample, you need to edit the [sample code](src/main/java/com/microsoft/azure/eventhubs/samples/eventprocessorsample/EventProcessorSample.java) and provide the following information:

```java
final String namespaceName = "----EventHubsNamespaceName-----";
final String eventHubName = "----EventHubName-----";

final String sasKeyName = "-----SharedAccessSignatureKeyName-----";
final String sasKey = "---SharedAccessSignatureKey----";

final String storageAccountName = "---StorageAccountName----"
final String storageAccountKey = "---StorageAccountKey----";
```

## Prerequisites

Please refer to the [overview README](../../README.md) for prerequisites and setting up the sample environment, including creating an Event Hubs cloud namespace and an Event Hub.

## Build and run

The sample can be built independently with 

```bash
mvn clean package
```

and then run with (or just from VS Code or another Java IDE)

```bash
java -jar ./target/eventprocessorsample-1.0.0-jar-with-dependencies.jar
```
