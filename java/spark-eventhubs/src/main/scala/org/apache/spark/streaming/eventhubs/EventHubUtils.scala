/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import com.microsoft.azure.eventhubs.EventData
import org.apache.spark.SparkContext
import org.apache.spark.api.java.{JavaRDD, JavaSparkContext}
import org.apache.spark.rdd.RDD
import org.apache.spark.storage.StorageLevel
import org.apache.spark.streaming.StreamingContext
import org.apache.spark.streaming.api.java.{JavaDStream, JavaReceiverInputDStream, JavaStreamingContext}
import org.apache.spark.streaming.dstream.{DStream, ReceiverInputDStream}
import org.apache.spark.streaming.receiver.Receiver

import scala.reflect.ClassTag

object EventHubUtils {

  /**
    * Create a RDD for a single Event Hubs partition
    * @param sc SparkContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param offsetRange contains the offsets and partitonId where data will be consumed
    * @return RDD of the single specified partition
    */
  def createPartitionRDD (
      sc: SparkContext,
      eventHubParams: Map[String, String],
      offsetRange: OffsetRange
  ): RDD[EventData] = sc.withScope {
    val client = new EventHubInstance
    new EventHubRDD(sc, eventHubParams, None, Some(offsetRange), client)
  }

  /**
    * Create a RDD for a single Event Hubs partition
    * @param jsc JavaSparkContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param offsetRange contains the offsets and partitonId where data will be consumed
    * @return RDD of the single specified partition
    */
  def createPartitionRDD (
      jsc: JavaSparkContext,
      eventHubParams: Map[String, String],
      offsetRange: OffsetRange
  ): JavaRDD[EventData] = jsc.sc.withScope {
    implicitly[ClassTag[AnyRef]].asInstanceOf[ClassTag[String]]
    val client = new EventHubInstance
    new EventHubRDD(jsc.sc, eventHubParams, None, Some(offsetRange), client)
  }

  /**
    * Create a RDD for an Event Hubs instance
    * @param sc SparkContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param offsetRanges contains the offsets and partitonId where data will be consumed
    * @return RDD of the Event Hubs instance
    */
  def createRDD (
      sc: SparkContext,
      eventHubParams: Map[String, String],
      offsetRanges: Array[OffsetRange]
  ): RDD[EventData] = sc.withScope {
    val client = new EventHubInstance
    new EventHubRDD(sc, eventHubParams, Some(offsetRanges), None, client)
  }

  /**
    * Create a RDD for an Event Hubs instance
    * @param jsc JavaSparkContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param offsetRanges contains the offsets and partitonId where data will be consumed
    * @return RDD of the Event Hubs instance
    */
  def createRDD (
      jsc: JavaSparkContext,
      eventHubParams: Map[String, String],
      offsetRanges: Array[OffsetRange]
  ): JavaRDD[EventData] = jsc.sc.withScope {
    implicitly[ClassTag[AnyRef]].asInstanceOf[ClassTag[String]]
    val client = new EventHubInstance
    new EventHubRDD(jsc.sc, eventHubParams, Some(offsetRanges), None, client)
  }

  /**
    * Create a input DStream for
    * @param ssc StreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionCount number of partitions in Event Hub instance
    * @param storageLevel Stream storage level
    * @param offsetStore Stream offset store
    * @return Input DStream of the Event Hubs instance
    */
  def createStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionCount: Int,
      storageLevel: StorageLevel,
      offsetStore: OffsetStore
  ): DStream[EventData] = {
    val streams = (0 until partitionCount).map { partitionId =>
      createPartitionStream(ssc, eventHubParams, partitionId.toString, storageLevel, offsetStore)
    }
    ssc.union[EventData](streams)
  }

  /**
    * Create a input DStream for
    * @param ssc StreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionCount number of partitions in Event Hub instance
    * @return Input DStream of the Event Hubs instance
    */
  def createStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionCount: Int
  ): DStream[EventData] = {
    createStream(ssc, eventHubParams, partitionCount, StorageLevel.MEMORY_ONLY, offsetStore = null)
  }

  /**
    * Create a input DStream for
    * @param ssc StreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionCount number of partitions in Event Hub instance
    * @param offsetStore Stream offset store
    * @return Input DStream of the Event Hubs instance
    */
  def createStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionCount: Int,
      offsetStore: OffsetStore
  ): DStream[EventData] = {
    createStream(ssc, eventHubParams, partitionCount, StorageLevel.MEMORY_ONLY, offsetStore)
  }

 /**
    * Create a input DStream for
    * @param ssc StreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionCount number of partitions in Event Hub instance
    * @param storageLevel Stream storage level
    * @return Input DStream of the Event Hubs instance
    */
  def createStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionCount: Int,
      storageLevel: StorageLevel
  ): DStream[EventData] = {
    createStream(ssc, eventHubParams, partitionCount, storageLevel, offsetStore = null)
  }

  /**
    * Create a input DStream for
    * @param jssc JavaStreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionCount number of partitions in Event Hub instance
    * @param storageLevel Stream storage level
    * @param offsetStore Stream offset store
    * @return Input DStream of the Event Hubs instance
    */
  def createStream (
      jssc: JavaStreamingContext,
      eventHubParams: Map[String, String],
      partitionCount: Int,
      storageLevel: StorageLevel,
      offsetStore: OffsetStore
  ): JavaDStream[EventData] = {
    implicitly[ClassTag[AnyRef]].asInstanceOf[ClassTag[String]]
    val streams = (0 until partitionCount).map { partitionId =>
      createPartitionStream(jssc.ssc, eventHubParams, partitionId.toString, storageLevel, offsetStore)
    }
    jssc.ssc.union[EventData](streams)
  }

  /**
    * Create a input DStream for
    * @param jssc JavaStreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionCount number of partitions in Event Hub instance
    * @return Input DStream of the Event Hubs instance
    */
  def createStream (
      jssc: JavaStreamingContext,
      eventHubParams: Map[String, String],
      partitionCount: Int
  ): JavaDStream[EventData] = {
    createStream(jssc, eventHubParams, partitionCount, StorageLevel.MEMORY_ONLY, offsetStore = null)
  }

 /**
    * Create a input DStream for
    * @param jssc JavaStreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionCount number of partitions in Event Hub instance
    * @param offsetStore Stream offset store
    * @return Input DStream of the Event Hubs instance
    */
  def createStream (
      jssc: JavaStreamingContext,
      eventHubParams: Map[String, String],
      partitionCount: Int,
      offsetStore: OffsetStore
  ): JavaDStream[EventData] = {
    createStream(jssc, eventHubParams, partitionCount, StorageLevel.MEMORY_ONLY, offsetStore)
  }

 /**
    * Create a input DStream for
    * @param jssc JavaStreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionCount number of partitions in Event Hub instance
    * @param storageLevel Stream storage level
    * @return Input DStream of the Event Hubs instance
    */
  def createStream (
      jssc: JavaStreamingContext,
      eventHubParams: Map[String, String],
      partitionCount: Int,
      storageLevel: StorageLevel
  ): JavaDStream[EventData] = {
    createStream(jssc, eventHubParams, partitionCount, storageLevel, offsetStore = null)
  }

  /**
    * Create a input DStream for a single partition in Event Hubs
    * @param ssc StreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionId partition ID that will be streamed
    * @param storageLevel Stream storage level
    * @param offsetStore Stream offset store
    * @return Input DStream a partition in an Event Hubs instance
    */
  def createPartitionStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String,
      storageLevel: StorageLevel,
      offsetStore: OffsetStore
  ): ReceiverInputDStream[EventData] = {
    ssc.receiverStream(getReceiver(ssc, eventHubParams, partitionId, storageLevel, offsetStore, new EventHubInstance))
  }

 /**
    * Create a input DStream for a single partition in Event Hubs
    * @param ssc StreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionId partition ID that will be streamed
    * @return Input DStream a partition in an Event Hubs instance
    */
  def createPartitionStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String
  ): ReceiverInputDStream[EventData] = {
    createPartitionStream(ssc, eventHubParams, partitionId, StorageLevel.MEMORY_ONLY, offsetStore = null)
  }

  /**
    * Create a input DStream for a single partition in Event Hubs
    * @param ssc StreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionId partition ID that will be streamed
    * @param offsetStore Stream offset store
    * @return Input DStream a partition in an Event Hubs instance
    */
  def createPartitionStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String,
      offsetStore: OffsetStore
  ): ReceiverInputDStream[EventData] = {
    createPartitionStream(ssc, eventHubParams, partitionId, StorageLevel.MEMORY_ONLY, offsetStore)
  }

 /**
    * Create a input DStream for a single partition in Event Hubs
    * @param ssc StreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionId partition ID that will be streamed
    * @param storageLevel Stream storage level
    * @return Input DStream a partition in an Event Hubs instance
    */
  def createPartitionStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String,
      storageLevel: StorageLevel
  ): ReceiverInputDStream[EventData] = {
    createPartitionStream(ssc, eventHubParams, partitionId, storageLevel, offsetStore = null)
  }

  /**
    * Create a input DStream for a single partition in Event Hubs
    * @param jssc JavaStreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionId partition ID that will be streamed
    * @param storageLevel Stream storage level
    * @param offsetStore Stream offset store
    * @return Input DStream a partition in an Event Hubs instance
    */
  def createPartitionStream (
      jssc: JavaStreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String,
      storageLevel: StorageLevel,
      offsetStore: OffsetStore
  ): JavaReceiverInputDStream[EventData] = {
    implicitly[ClassTag[AnyRef]].asInstanceOf[ClassTag[String]]
    jssc.ssc.receiverStream(getReceiver(jssc.ssc, eventHubParams, partitionId, storageLevel, offsetStore, new EventHubInstance))
  }

  /**
    * Create a input DStream for a single partition in Event Hubs
    * @param jssc JavaStreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionId partition ID that will be streamed
    * @return Input DStream a partition in an Event Hubs instance
    */
  def createPartitionStream (
      jssc: JavaStreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String
  ): JavaReceiverInputDStream[EventData] = {
    createPartitionStream(jssc, eventHubParams, partitionId, StorageLevel.MEMORY_ONLY, offsetStore = null)
  }

 /**
    * Create a input DStream for a single partition in Event Hubs
    * @param jssc JavaStreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionId partition ID that will be streamed
    * @param offsetStore Stream offset store
    * @return Input DStream a partition in an Event Hubs instance
    */
  def createPartitionStream (
      jssc: JavaStreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String,
      offsetStore: OffsetStore
  ): JavaReceiverInputDStream[EventData] = {
    createPartitionStream(jssc, eventHubParams, partitionId, StorageLevel.MEMORY_ONLY, offsetStore)
  }

 /**
    * Create a input DStream for a single partition in Event Hubs
    * @param jssc JavaStreamingContext object
    * @param eventHubParams Map of event hubs parameters - see the readme here:
    *                       https://github.com/sabeegrewal/azure-event-hubs/tree/master/java/spark-eventhubs
    * @param partitionId partition ID that will be streamed
    * @param storageLevel Stream storage level
    * @return Input DStream a partition in an Event Hubs instance
    */
  def createPartitionStream (
      jssc: JavaStreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String,
      storageLevel: StorageLevel
  ): JavaReceiverInputDStream[EventData] = {
    createPartitionStream(jssc, eventHubParams, partitionId, storageLevel, offsetStore = null)
  }

  /**
    * Helper function to create the correct type of receiver
    */
  private def getReceiver(
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String,
      storageLevel: StorageLevel,
      offsetStore: OffsetStore,
      receiverClient: EventHubInstance): Receiver[EventData] = {
    val enabled = ssc.conf.getBoolean("spark.streaming.receiver.writeAheadLog.enable", defaultValue = false)
    if (enabled) {
      new ReliableEventHubReceiver(eventHubParams, partitionId, storageLevel, offsetStore, receiverClient)
    } else {
      new EventHubReceiver(eventHubParams, partitionId, storageLevel, offsetStore, receiverClient)
    }
  }
}
