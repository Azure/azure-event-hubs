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

  def createPartitionRDD (
      sc: SparkContext,
      eventHubParams: Map[String, String],
      offsetRange: OffsetRange,
      client: EventHubInstance = new EventHubInstance
  ): RDD[EventData] = sc.withScope {
    new EventHubRDD(sc, eventHubParams, None, Some(offsetRange), client)
  }

  def createPartitionRDD (
      jsc: JavaSparkContext,
      eventHubParams: Map[String, String],
      offsetRange: OffsetRange,
      client: EventHubInstance = new EventHubInstance
  ): JavaRDD[EventData] = jsc.sc.withScope {
    implicitly[ClassTag[AnyRef]].asInstanceOf[ClassTag[String]]
    new EventHubRDD(jsc.sc, eventHubParams, None, Some(offsetRange), client)
  }

  def createRDD (
      sc: SparkContext,
      eventHubParams: Map[String, String],
      offsetRanges: Array[OffsetRange],
      client: EventHubInstance = new EventHubInstance
  ): RDD[EventData] = sc.withScope {
    new EventHubRDD(sc, eventHubParams, Some(offsetRanges), None, client)
  }

  def createRDD (
      jsc: JavaSparkContext,
      eventHubParams: Map[String, String],
      offsetRanges: Array[OffsetRange],
      client: EventHubInstance = new EventHubInstance
  ): JavaRDD[EventData] = jsc.sc.withScope {
    implicitly[ClassTag[AnyRef]].asInstanceOf[ClassTag[String]]
    new EventHubRDD(jsc.sc, eventHubParams, Some(offsetRanges), None, client)
  }

  def createStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionCount: Int,
      storageLevel: StorageLevel = StorageLevel.MEMORY_ONLY
  ): DStream[EventData] = {
    val streams = (0 until partitionCount).map { partitionId =>
      createPartitionStream(ssc, eventHubParams, partitionId.toString, storageLevel)
    }
    ssc.union[EventData](streams)
  }

  def createStream (
      jssc: JavaStreamingContext,
      eventHubParams: Map[String, String],
      partitionCount: Int,
      storageLevel: StorageLevel = StorageLevel.MEMORY_ONLY
  ): JavaDStream[EventData] = {
    implicitly[ClassTag[AnyRef]].asInstanceOf[ClassTag[String]]
    val streams = (0 until partitionCount).map { partitionId =>
      createPartitionStream(jssc.ssc, eventHubParams, partitionId.toString, storageLevel)
    }
    jssc.ssc.union[EventData](streams)
  }

  def createPartitionStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String,
      storageLevel: StorageLevel = StorageLevel.MEMORY_ONLY,
      offsetStore: OffsetStore = null,
      receiverClient: EventHubInstance = new EventHubInstance()
  ): ReceiverInputDStream[EventData] = {
    ssc.receiverStream(getReceiver(ssc, eventHubParams, partitionId, storageLevel, offsetStore, receiverClient))
  }

  def createPartitionStream (
      jssc: JavaStreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String,
      storageLevel: StorageLevel = StorageLevel.MEMORY_ONLY,
      offsetStore: OffsetStore = null,
      receiverClient: EventHubInstance = new EventHubInstance()
  ): JavaReceiverInputDStream[EventData] = {
    implicitly[ClassTag[AnyRef]].asInstanceOf[ClassTag[String]]
    jssc.ssc.receiverStream(getReceiver(jssc.ssc, eventHubParams, partitionId, storageLevel, offsetStore, receiverClient))
  }

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
