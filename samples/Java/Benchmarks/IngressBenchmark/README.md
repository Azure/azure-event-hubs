# Event Hubs throughput benchmark

Please review the sample code comments. 

To run the sample, you need to edit the [sample code](src/main/java/com/microsoft/azure/eventhubs/samples/send/Send.java) and provide the following information:

```java
    final String namespaceName = "----EventHubsNamespaceName-----";
    final String eventHubName = "----EventHubName-----";
    final String sasKeyName = "-----SharedAccessSignatureKeyName-----";
    final String sasKey = "---SharedAccessSignatureKey----";
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
java -jar ./target/ingressbenchmark-1.0.0-jar-with-dependencies.jar
```