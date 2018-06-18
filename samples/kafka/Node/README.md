# Send and Receive Messages in Node using Event Hubs for Apache Kafka Ecosystems

One of the key benefits of using Apache Kafka is the number of ecosystems it can connect to. Kafka-enabled Event Hubs allow users to combine the flexibility of the Kafka ecosystem with the scalability, consistency, and support of the Azure ecosystem without having to manage on prem clusters or resources - it's the best of both worlds!

An Azure Event Hubs Kafka endpoint enables users to connect to Azure Event Hubs using the Kafka protocol (i.e. Kafka clients). By making minimal changes to a Kafka application, users will be able to connect to Azure Event Hubs and reap the benefits of the Azure ecosystem. Kafka enabled Event Hubs currently supports Kafka versions 1.0 and later.

This quickstart will show how to create and connect to an Event Hubs Kafka endpoint using an example producer and consumer written in Node.

NOTE: This sample uses the [node-rdkafka](https://github.com/Blizzard/node-rdkafka) library which currently doesn't support Windows as well as it supports Linux/Mac. For instructions on how to configure for Windows, please visit the node-rdkafka project and follow their instructions before continuing with the sample (though our sample is untested on Windows). However, this sample has been tested on the [Linux Subsystem on Windows 10](https://docs.microsoft.com/en-us/windows/wsl/install-win10), so that is a good option if you plan on running on Windows.

## Prerequisites

If you don't have an Azure subscription, create a [free account](https://azure.microsoft.com/free/?ref=microsoft.com&utm_source=microsoft.com&utm_medium=docs&utm_campaign=visualstudio) before you begin.

In addition:

-   [Node.js](https://nodejs.org)
-   [OpenSSL](https://github.com/openssl/openssl)
    -   On Mac OS, you can run `brew install openssl`
    -   On Ubuntu, `sudo apt-get install libssl-dev`
-   [Git](https://www.git-scm.com/downloads)
    -   On Ubuntu, you can run `sudo apt-get install git` to install Git.

## Create an Event Hubs namespace

An Event Hub's namespace is required to send or receive from any Event Hubs service. See [Create Kafka Enabled Event Hubs](https://docs.microsoft.com/azure/event-hubs/event-hubs-create-kafka-enabled) for instructions on getting an Event Hubs Kafka endpoint.

Make sure to copy the Event Hubs connection string for later use.

Additionally, topics in Kafka map to Event Hub instances, so create an Event Hub instance called "test" that our samples can send and receive messages from.

## Clone the example project

Now that you have a Kafka enabled Event Hubs connection string, clone the Azure Event Hubs repository and navigate to the `quickstart` subfolder:

```bash
git clone https://github.com/Azure/azure-event-hubs.git
cd azure-event-hubs/samples/kafka/Node
```

## Configuration

Configure the Kafka Producer/Consumer types with the fully qualified domain name and port of the Kafka head of your Event Hub and its connection string. For the consumer, you will also need the consumer group for that 'topic' (event hub); the default in Azure Event Hubs is `$Default`.

```javascript
var producer = new Kafka.Producer({
  //'debug' : 'all',
  'metadata.broker.list': 'EVENTHUB_FQDN',
  'dr_cb': true,  //delivery report callback
  'security.protocol': 'SASL_SSL',
  'sasl.mechanisms': 'PLAIN',
  'sasl.username': '$ConnectionString',
  'sasl.password': 'EVENTHUB_CONNECTION_STRING'
});

```

This sample uses the [`node-rdkafka` module](https://github.com/Blizzard/node-rdkafka); to install the node module to your project, run this command:

Linux:
```bash
npm install node-=rdkafka
```

Mac OS:
``` bash
CPPFLAGS=-I/usr/local/opt/openssl/include LDFLAGS=-L/usr/local/opt/openssl/lib npm install node-rdkafka
```

## Producer

The producer sample demonstrates how to send messages to the Event Hubs service using the Kafka head.

You can run the sample via:

```bash
$ node producer.js
```

The producer will now begin sending events to the Kafka enabled Event Hub on topic `test`. If you would like to change the topic, change the topic variable in `producer.js`.

## Consumer

The consumer sample demonstrates how to receive messages from the Event Hubs service using the Kafka head.

You can run the sample via:

```bash
$ node consumer.js
```

The consumer will now begin receiving events from the Kafka enabled Event Hub on topic `test`. If you would like to change the topic, change the topic variable in `consumer.js`. If the topic has not already been created in the Kafka configuration, no messages will be sent/received.
