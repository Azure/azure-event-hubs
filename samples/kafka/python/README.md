# Using Confluent's Python Kafka client and librdkafka with Event Hubs for Apache Kafka Ecosystems

Sample for sending 100 messages to a Kafka-enabled Event Hub using Confluent's Python Kafka client

***This sample is still a work in progress. Your mileage may vary.***

### Set up

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
