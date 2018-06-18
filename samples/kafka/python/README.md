# Using Confluent's Python Kafka client and librdkafka with Event Hubs for Apache Kafka Ecosystems

One of the key benefits of using Kafka is the number of ecosystems it can connect to. Kafka-enabled Event Hubs allow users to combine the flexibility of the Kafka ecosystems with the scalability, consistency, and support of the Azure ecosystem without having to manage on prem clusters or resources - it's the best of both worlds!

This tutorial will show how to connect Confluent's Apache Kafka Python client to Kafka-enabled Event Hubs without changing your protocol clients or running your own clusters. Azure Event Hubs for Apache Kafka Ecosystems supports [Apache Kafka version 1.0.](https://kafka.apache.org/10/documentation.html)

***This sample is still a work in progress. Your mileage may vary, and your feedback is appreciated!***

## Prerequisites

If you don't have an Azure subscription, create a [free account](https://azure.microsoft.com/en-us/free/?ref=microsoft.com&utm_source=microsoft.com&utm_medium=docs&utm_campaign=visualstudio) before you begin.

## Create an Event Hubs namespace

An Event Hubs namespace is required to send or receive from any Event Hubs service. See [Create Kafka-enabled Event Hubs](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-create-kafka-enabled) for instructions on getting an Event Hubs Kafka endpoint. Make sure to copy the Event Hubs connection string for later use.

## Set up

Clone this repo and copy the setup script, producer, and consumer to your working directory:

```bash
git clone https://github.com/Azure/azure-event-hubs.git
cp -n azure-event-hubs/samples/kafka/* .
```

Now run the set up script:

```shell
source setup.sh
```

## Producer

### Update the configuration

Update `bootstrap.servers` and `sasl.password` in `producer.py` to direct the producer to the Event Hubs Kafka endpoint with the correct authentication.

### Run the producer from the command line
 
```shell 
python producer.py <topic>
```

Note that the topic must already exist or else you will see an "Unknown topic or partition" error.

## Consumer

### Update the configuration

Update `bootstrap.servers` and `sasl.password` in `consumer.py` to direct the consumer to the Event Hubs Kafka endpoint with the correct authentication.

### Run the consumer from the command line

```shell
python consumer.py <your-consumer-group> <topic.1> <topic.2> ... <topic.n> 
```

## Key resources and attributions

* [Confluent's Apache Kafka Python client](https://github.com/confluentinc/confluent-kafka-python)
	* This repo's producer is based on [Confluent's samples](https://github.com/confluentinc/confluent-kafka-python/tree/master/examples)
* [librdkafka](https://github.com/edenhill/librdkafka)
