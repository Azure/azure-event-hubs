# Azure Event Hubs Java samples

Azure Event Hubs is a highly scalable data streaming platform and event ingestion service, capable of receiving and processing millions of events per second. The samples present here enables Java developers to easily ingest and process events from your event hub.

## Prerequisites
1.	The samples here are built using Maven. If you plan to use Maven, you can [download Maven](https://maven.apache.org/download.cgi). [Install and configure Maven](https://maven.apache.org/install.html). The samples present here requires version > 3.3.9. 
2.	You need an Azure Subscription, if you do not have one - create a [free account](https://azure.microsoft.com/free/?ref=microsoft.com&utm_source=microsoft.com&utm_medium=docs&utm_campaign=visualstudio) before you begin
3.	[An Event Hubs namespace and an event hub where you ingest the data](https://docs.microsoft.com/azure/event-hubs/event-hubs-create)
4.	[A SAS key to access the event hub](https://docs.microsoft.com/azure/event-hubs/event-hubs-create#SAS)

## Ingest events into event hub
The tutorial [here](https://docs.microsoft.com/azure/event-hubs/event-hubs-java-get-started-send) walks you for ingesting into your event hub using Java. Download the code from GitHub. Provide the following details in your sample [Send client](https://github.com/Azure/azure-event-hubs/blob/master/samples/Java/src/main/java/com/microsoft/azure/eventhubs/samples/Basic/Send.java)

```java
    final String namespaceName = "----ServiceBusNamespaceName-----";
    final String eventHubName = "----EventHubName-----";
    final String sasKeyName = "-----SharedAccessSignatureKeyName-----";
    final String sasKey = "---SharedAccessSignatureKey----";
    ConnectionStringBuilder connStr = new ConnectionStringBuilder(namespaceName, eventHubName, sasKeyName, sasKey);
```

## Process events from event hub
The tutorial here walks you through the consuming/receiving events from event hub. You can download the sample from GitHub. The sample uses Event Processor Host for consuming the events. Events in an event hub are distributed in various partitions to achieve parallelism while consuming. EPH simplifies processing events from event hub by managing persistent checkpoints and parallel receives. Provide the following details in your sample [EventProcessorHost client](https://github.com/Azure/azure-event-hubs/blob/master/samples/Java/src/main/java/com/microsoft/azure/eventhubs/samples/Basic/EventProcessorSample.java) 


```java
final String namespaceName = "----ServiceBusNamespaceName-----";
final String eventHubName = "----EventHubName-----";

final String sasKeyName = "-----SharedAccessSignatureKeyName-----";
final String sasKey = "---SharedAccessSignatureKey----";

final String storageAccountName = "---StorageAccountName----"
final String storageAccountKey = "---StorageAccountKey----";
```

## Running the sample
Do a maven clean compile from the location you have your samples in
``` java
cd azure-event-hubs/samples/Java/
mvn clean compile exec:java
```


