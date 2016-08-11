# Event Hubs and Apache Spark Integration

This repository contains open-source adapters to connect Event Hubs with Apache Spark and Spark Streaming. This guide is written presuming users have experience with both Apache Spark and Event Hubs. Usage samples can be found in the examples directory.

## eventHubParams<a name="ehparams"></a>
Every adapter takes in information about your Event Hubs instance in the form of a map called **eventHubParams**. Here are the following parameters:

| Parameter | Required? | Description
--- | --- | ---
namespaceName | Always | The namespace of your EventHub instance.
eventHubName | Always | The name of your EventHub instance.
sasKeyName | Always | The name of your SAS key.
sasKey | Always | Your SAS key.
checkpointDir | DStreams Only | The directory checkpoint data specific to EventHubs will be saved. See [Checkpointing](#checkpoint) for more information. 
checkpointInterval | No | Only relevant for Spark Streaming. Checkpointing for EventHubs will happen on this interval. Provide the value in miliseconds. Default value is 10 seconds. See [Checkpointing](#checkpoint) for more information. 

Below is an example of eventHubParams initialization: 
```scala 
val eventHubParams: Map[String, String] = Map(
	"namespaceName" -> "XXXXX-ns",	// Required
    "eventHubName" -> "XXXXX",		// Required
    "sasKeyName" -> "SAS_KEY_NAME",	// Required
    "sasKey" -> "SAS_KEY",			// Required
    "checkpointDir" -> "directory",	// For Spark Streaming only!
    "checkpointInterval" "2000")    // Optional - only relevant in streaming scenarios
```

## Fault Tolerance
#### Checkpointing<a name="checkpoint"></a>
When using the EventHubs+Spark adapter, data from Spark and the EventHubs+Spark adapter is checkpointed to ensure the system is fault tolerant. The directory for each checkpointing mechanism is set separately. For Spark Streaming, you set the checkpoint directory like so:

```scala 
ssc.checkpoint("checkpoint_dir")
```

Where *ssc* is your StreamingContext. For checkpointing data from EventHubs+Spark adapter, you set the directory in the  [eventHubParams](#ehparams).

#### At Most Once Semantics
By default, the EventHubs+Spark adapter will deliver *at most once* semantics. Spark's write ahead log (aka journaling) mechanism isn't used, so if a failure occurs, then the event data that has been received by the spark adapter but not yet processed will be lost. More specifically, when the node recovers, the EventHubs+Spark adapter will start consuming events based on the last offset that was checkpointed. In the *at most once case*, offsets are checkpointed before events are processed in Spark. 

This is the more performant option, so if your system works properly when most of your data is processed, then this is the recommended option. 

#### At Least Once Semantics
Write ahead logs can be enabled in Apache Spark which will deliver *at least once* semantics. The key difference is that event data is resiliently stored in a write ahead log. Offsets will not be checkpointed until the event data is properly stored in the write ahead log. That way, if a failure occurs, the system will first consume events from the write ahead log before consuming directly from the EventHubs instance. 

This is more expensive. We only recommend this option if it's critical all of your data is processed.

To enable Write Ahead Logs, simple add the following code:
```scala 
sparkConf.set("spark.streaming.receiver.writeAheadLog.enable", "true")
```

## Spark Streaming Adapter 
There are two options when creating DStreams with Event Hubs - you can make a DStream for your entire Event Hubs instance (createStream) or for a single Event Hubs partition (createPartitionStream). The stream will start at the last offset that has been checkpointed in your checkpoint directory. If an offset doesn't exist in the checkpoint directory, then the stream starts at the beginning of the EventHubs instance. 

*namespaceName*, *eventHubName*, *sasKeyName*, *sasKey*, and *checkpointDir* must be provided in your eventHubParams for either type of DStream.

#### createStream
Returns a DStream of your entire Event Hubs instance.

```scala 
def createStream (
    ssc: StreamingContext,
    eventHubParams: Map[String, String],
    partitionCount: Int, 
    storageLevel: StorageLevel,		// Optional! Uses StorageLevel.MEMORY_ONLY by default. See Storage Level,
    offsetStore: OffsetStore		// Optional! Uses DfsBasedOffsetStore by default. See Offset Store.
) 
```

#### createPartitionStream
Returns a DStream of a single Event Hubs partition.

```scala
createPartitionStream
def createPartitionStream (
    ssc: StreamingContext,
    eventHubParams: Map[String, String],
    partitionId: String,
    storageLevel: StorageLevel,		// Optional! Uses StorageLevel.MEMORY_ONLY by default. See Storage Level.
    offsetStore: OffsetStore		// Optional! Uses DfsBasedOffsetStore by default. See Offset Store.
)
```  

#### Storage Level<a name="storagelevel"></a> 
The storage level for DStreams generated by the EventHubs+Spark adapter defaults to *MEMORY_ONLY*. If you'd like to change it, simply pass a different storage level when you create a DStream. 

For more info on storage levels in Apache Spark, check out:
http://spark.apache.org/docs/latest/programming-guide.html#which-storage-level-to-choose

#### Offset Store<a name="offsetstore"></a> 
The Offset Store helps facilitate the reading and writing of checkpoint data specific to Event Hubs. The default OffsetStore used can be found in DfsBasedOffsetStore.scala. The default will work if your checkpoint directory is compatible with the HDFS API. For example, HDFS and Azure Blob will work. Check out CheckpointWithAzureBlob.md for info on how Azure Blob can be used for checkpointing in Apache Spark/Spark Streaming.  

If you're checkpoint directory isn't compatible with the the current implementation, a new OffsetStore needs to be implemented.

## Spark Core Adapter
Just like the Spark Streaming adapters, the Apache Spark adapters have two varieties - you can make a RDD for your entire Event Hubs instance (createRDD) or for a particular partition (createPartitionRDD).

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
val offsetRanges = OffsetRange.createArray(numPartitions = 4, startingOffset = "-1", batchSize = 50)
```
#### createPartitionRDD
Returns a RDD of a single Event Hubs partition.
```scala
def createPartitionRDD (
	sc: SparkContext,
    eventHubParams: Map[String, String],
	offsetRange: OffsetRange
)
```
The offsetRange can be created like so:
```scala
val offsetRange = OffsetRange(partitionId = "4", startingOffset = "-1", batchSize = 50)
```

#### Offset Range
The offset range specifies a starting offset, batch size, and partitionId. In the EventHubs+Spark Adapter, there is a one-to-one mapping between an offset range and a EventHubs partition.  

To start at the beginning of your EventHubs instance pass a starting offset of "-1". You can start at the most recently received event by passing in "latest" as your starting offset. You can pass any other positive number as well (assuming that offset exists in your EventHubs instance).

A single offset range is passed when creating a Partition RDD. 
```scala
val offsetRange = OffsetRange(partitionId = "4", startingOffset = "-1", batchSize = 50)
```

An array of offset ranges is passed when creating a RDD for your entire EventHubs instance. 
```scala
val offsetRanges = OffsetRange.createArray(numPartitions = 4, startingOffset = "-1", batchSize = 50)
```

If you'd like to change the starting offset or batch size of a particular offset range, it can be done like so:

```scala
val offsetRanges = OffsetRange.createArray(numPartitions = 4, startingOffset = "-1", batchSize = 500)
offsetRanges(0).set(startingOffset = -1, batchSize = 1000)
```
If *offsetRanges* from this code sample is used to generate an RDD, then 1000 events would be consumed from partition 0 in EventHubs. For all other partitions, only 500 events would be consumed. 
