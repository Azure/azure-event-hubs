# Migrating to Azure Event Hubs for Apache Kafka Ecosystems

When we built Apache Kafka-enabled Event Hubs, we wanted to give Kafka users the stability, scalability, and support of Event Hubs without sacrificing their ability to connect to the network of Kafka supporting frameworks. With that in mind, we've started rolling out a set of tutorials to show how simple it is to connect Kafka-enabled Event Hubs with various platforms and frameworks. The tutorials in this directory all work right out of the box, but for those of you looking to connect with a framework we haven't covered, this guide will outline the generic steps needed to connect your preexisting Kafka application to an Event Hubs Kafka endpoint.

## Prerequisites

If you don't have an Azure subscription, create a [free account](https://azure.microsoft.com/en-us/free/?ref=microsoft.com&utm_source=microsoft.com&utm_medium=docs&utm_campaign=visualstudio) before you begin.

## Create an Event Hubs namespace

An Event Hubs namespace is required to send or receive from any Event Hubs service. See [Create Kafka-enabled Event Hubs](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-create-kafka-enabled) for instructions on getting an Event Hubs Kafka endpoint. Make sure to copy the Event Hubs connection string for later use.

## Update your Kafka client configuration

To connect to a Kafka-enabled Event Hub, you'll need to update the Kafka client configs. If you're having trouble finding yours, try searching for where `bootstrap.servers` is set in your application.

Insert the following configs wherever makes sense in your application. Make sure to update the `bootstrap.servers` and `sasl.jaas.config` values to direct the client to your Event Hubs Kafka endpoint with the correct authentication. 

```
bootstrap.servers={YOUR.EVENTHUBS.FQDN}:9093
request.timeout.ms=60000
security.protocol=SASL_SSL
sasl.mechanism=PLAIN
sasl.jaas.config=org.apache.kafka.common.security.plain.PlainLoginModule required username="$ConnectionString" password="{YOUR.EVENTHUBS.CONNECTION.STRING}";
``` 

If `sasl.jaas.config` is not a supported configuration in your framework, find the configurations that are used to set the SASL username and password and use those instead. Set the username to `$ConnectionString` and the password to your Event Hubs connection string.

## Run your application 

Run your application and see how it goes - in most cases this should be enough to make the switch. If it wasn't and you need help (or if you know the secret to making it work with your framework), let us know by opening up a GitHub issue at our [tutorials page on Azure Docs](https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-quickstart-kafka-enabled-event-hubs)!