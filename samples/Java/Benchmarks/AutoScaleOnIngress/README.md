# Automatically scale up Event Hubs throughput units

The [Automatically scale up Azure Event Hubs throughput units](https://docs.microsoft.com/azure/event-hubs/event-hubs-auto-inflate) article discusses how an event hub can automatically adapt to high event flow rates. This sample illustrates this functionality with a Java sender that will produce enough events to trigger the auto-scale function to kick in. 

The sample is interactive and will ask for a throughput unit threshold to hit and for an event hub connection string.

## Prerequisites

Please refer to the [overview README](../../README.md) for prerequisites and setting up the sample environment, including creating an Event Hubs cloud namespace and an Event Hub.

## Build and run

The sample can be built independently with 

```bash
mvn clean package
```

and then run with (or just from VS Code or another Java IDE)

```bash
java -jar ./target/autoscaleoningress-1.0.0-jar-with-dependencies.jar
```