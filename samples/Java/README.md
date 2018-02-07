# Azure Event Hubs Java samples

Azure Event Hubs is a highly scalable data streaming platform and event ingestion service, capable of receiving and processing millions of events per second. The samples present here enables Java developers to easily ingest and process events from your event hub.

## Prerequisites

1.	The samples depend on the Java JDK 1.8 and are built using Maven. You can [download Maven](https://maven.apache.org/download.cgi). [Install and configure Maven](https://maven.apache.org/install.html). The sample require Maven version > 3.3.9. 
2.	You need an Azure Subscription, if you do not have one - create a [free account](https://azure.microsoft.com/free/?ref=microsoft.com&utm_source=microsoft.com&utm_medium=docs&utm_campaign=visualstudio) before you begin
3.	[An Event Hubs namespace and an event hub where you ingest the data](https://docs.microsoft.com/azure/event-hubs/event-hubs-create)
4.	[A SAS key to access the event hub](https://docs.microsoft.com/azure/event-hubs/event-hubs-create#SAS)

### Sending events

* **Send** - The [Send](./Basic/Send) sample illustrates how to ingest events into your event hub.
* **SendBatch** - The [SendBatch](./Basic/SendBatch) sample illustrates how to ingest batches of events into your event hub.
 
### Processing events

* **ReceiveByDateTime** - The [ReceiveByDateTime](./Basic/ReceiveByDateTime) sample illustrates how to receive events from an event hub partition using a specific date-time offset.
* **ReceiveUsingOffset** - The [ReceiveUsingOffset](./Basic/ReceiveUsingOffset) sample illustrates how to receive events from an event hub partition using a specific data offset.
* **EventProcessorSample** - The [EventProcessorSample](./Basic/EventProcessorSample) sample illustrates how to receive events from an event hub using the event processor host, which provides automatic partition selection and fail-over across multiple concurrent receivers. 

### Benchmarks

* **AutoScaleOnIngress** - The [AutoScaleOnIngress](./Benchmarks/AutoScaleOnIngress) sample illustrates how an event hub can automatically scale up on high loads. The sample will send events at a rate that just exceed the configured rate of an event hub, causing the event hub to scale up.
* **IngressBenchmark** - The [IngressBenchmark](./Benchmarks/IngressBenchmark) sample allows measuring the ingress rate. 

## Build and run

All samples can be built at once with

```bash
mvn clean package
```

The samples are dropped into the respective sample's ./target subfolder. The build packages all dependencies into a single assembly so that you can execute them with:

```bash
java -jar ./target/{project}-1.0.0-jar-with-dependencies.jar
```