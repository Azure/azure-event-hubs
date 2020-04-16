# Receive events from Azure Event Hubs using Java

This sample demonstrates how to use the Event Processor Host library with AAD authentication to receive events from Azure Event Hubs.

## Prerequisites

Please refer to the [overview README](../../README.md) for prerequisites and setting up the sample environment, including creating an Event Hubs cloud namespace and an Event Hub.

## Build and run

The sample can be built independently with 

```bash
mvn clean package
```

and then run with (or just from VS Code or another Java IDE)

```bash
java -jar ./target/rbaceventprocessor-1.0.0-jar-with-dependencies.jar
```
