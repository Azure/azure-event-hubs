# Send and Receive Messages in Go using Azure Event Hubs for Kafka Ecosystem

Azure Event Hubs is a highly scalable data streaming platform and event ingestion service, capable of receiving and processing millions of events per second. Event Hubs can process and store events, data, or telemetry produced by distributed software and devices. Data sent to an event hub can be transformed and stored using any real-time analytics provider or batching/storage adapters.

An Azure Event Hubs Kafka endpoint enables users to connect to Azure Event Hubs using the Kafka protocol (i.e. Kafka clients). By making minimal changes to a Kafka application, users will be able to connect to Azure Event Hubs and reap the benefits of the Azure ecosystem. Kafka enabled Event Hubs currently supports Kafka versions 1.0 and later.

This quickstart will show how to create and connect to an Event Hubs Kafka endpoint using an example producer and consumer written in Go.

## Prerequisites

If you don't have an Azure subscription, create a [free account](https://azure.microsoft.com/en-us/free/?ref=microsoft.com&utm_source=microsoft.com&utm_medium=docs&utm_campaign=visualstudio) before you begin.

In addition:

-   [Go Tools Distribution](https://golang.org/doc/install)
-   [Git](https://www.git-scm.com/downloads)
    -   On Ubuntu, you can run `sudo apt-get install git` to install Git.

## Create an Event Hubs namespace

An Event Hubs namespace is required to send or receive from any Event Hubs service. See [Create Kafka Enabled Event Hubs](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-create-kafka-enabled) for instructions on getting an Event Hubs Kafka endpoint.

Make sure to copy the Event Hubs connection string for later use.

Additionally, topics in Kafka map to Event Hub instances, so create an Event Hub instance called "test" that our samples can send and receive messages from.

## Clone the example project

Now that you have a Kafka enabled Event Hubs connection string, clone the Azure Event Hubs repository and navigate to the `quickstart` subfolder:

```bash
git clone https://github.com/Azure/azure-event-hubs.git
cd azure-event-hubs/samples/kafka/quickstart/Go
```

## Configuration

Define two environmental variables that specify the fully qualified domain name and port of the Kafka head of your Event Hub and its connection string.

```bash
$ export KAFKA_EVENTHUB_ENDPOINT="my-event-hub-namespace.servicebus.windows.net:9093"
$ export KAFKA_EVENTHUB_CONNECTION_STRING="Endpoint=sb://my-event-hub-namespace.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=Q7fh80DP+70+HrcduL0Xpub2VTN+retgFi4ivxMY7Yk="
```

## Producer

The producer sample demonstrates how to send messages to the Event Hubs service using the Kafka head.

You can run the sample via:

```bash
$ cd producer
$ go run producer.go
```

The producer will now begin sending events to the Kafka enabled Event Hub on topic `test` and printing the events to stdout. If you would like to change the topic, change the topic variable in `producer.go`.

## Consumer

The consumer sample demonstrates how to receive messages from the Event Hubs service using the Kafka head.

You can run the sample via:

```bash
$ cd consumer
$ go run consumer.go
```

The consumer will now begin receiving events from the Kafka enabled Event Hub on topic `test` and printing the events to stdout. If you would like to change the topic, change the topic variable in `consumer.go`.
