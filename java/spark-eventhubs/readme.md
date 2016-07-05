# Event Hubs and Apache Spark Integration

This repository contains open-source adapters to connect Event Hubs with Apache Spark and Spark Streaming. This guide is written presuming users have experience with both Apache Spark and Event Hubs.  

Usage examples can be found in Driver.scala. 

## eventHubParams
Every adapter takes in information about your Event Hubs instance in the form of a map called **eventHubParams**. The following parameters must be specified:
* namespaceName
* eventHubName
* sasKeyName
* sasKey
* checkpointDir (for DStreams only)

Below is an example of eventHubParams initialization is: 
```scala 
val eventHubParams: Map[String, String] = Map(
	"namespaceName" -> "XXXXX-ns",
    "eventHubName" -> "XXXXX",
    "sasKeyName" -> "SAS_KEY_NAME",
    "sasKey" -> "SAS_KEY")    
```

## Spark Streaming
There are two options when creating DStreams with Event Hubs - you can make a DStream for your entire Event Hubs instance or for a single Event Hubs partition. 

*namespaceName*, *eventHubName*, *sasKeyName*, *sasKey*, and *checkpointDir* must be provided in your eventHubParams for either type of DStream.

#### createStream
Returns a DStream of your entire Event Hubs instance.

```scala 
def createStream (
    ssc: StreamingContext,
    eventHubParams: Map[String, String],
    partitionCount: Int
) 
```
#### createPartitionStream
Returns a DStream of a single Event Hubs partition.

```scala
createPartitionStream
def createPartitionStream (
    ssc: StreamingContext,
    eventHubParams: Map[String, String],
    partitionId: String
)
```  
## Apache Spark
Just like the Spark Streaming adapters, the Apache Spark adapters have two varieties - you can make a RDD for your entire Event Hubs instance or for a particular partition.

*namespaceName*, *eventHubName*, *sasKeyName*, and *sasKey* must be provided in your eventHubParams for either type of DStream.
#### createRDD
Returns a RDD of your entire Event Hubs instance.
```scala
def createRDD (
	sc: SparkContext,
    eventHubParams: Map[String, String],
    offsetRanges: Array[OffsetRange]
)
```
The offsetRanges array can be created like so:
```scala
val offsetRanges = OffsetRange.createArray(numPartitions = 4, startingOffset = -1, batchSize = 50)
```
#### createPartitionRDD
Returns a RDD of a single Event Hubs partition.
```scala
def createPartitionRDD (
	jsc: JavaSparkContext,
    eventHubParams: Map[String, String],
	offsetRange: OffsetRange
)
```
The offsetRange can be created like so:
```scala
val offsetRange = OffsetRange(numPartitions = 4, startingOffset = -1, batchSize = 50)
```
