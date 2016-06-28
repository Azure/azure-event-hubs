/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import com.microsoft.azure.eventhubs.EventData
import org.apache.spark.SparkException
import org.apache.spark.storage.StorageLevel
import org.apache.spark.streaming.StreamingContext
import org.apache.spark.streaming.dstream.{DStream, ReceiverInputDStream}
import org.apache.spark.streaming.receiver.Receiver

object EventHubUtils {

  def createStream (
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      storageLevel: StorageLevel = StorageLevel.MEMORY_ONLY
  ): DStream[EventData] = {
    val partitionCount = eventHubParams.getOrElse("partitionCount",
      throw new SparkException("Please specify the number of partitions in your EventHubs instance")).toInt
    val streams = (0 until partitionCount).map { partitionId =>
      createPartitionStream(ssc, eventHubParams, partitionId.toString, storageLevel)
    }
    ssc.union[EventData](streams)
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

  private def getReceiver(
      ssc: StreamingContext,
      eventHubParams: Map[String, String],
      partitionId: String,
      storageLevel: StorageLevel,
      offsetStore: OffsetStore,
      receiverClient: EventHubInstance): Receiver[EventData] = {
    val enabled = ssc.conf.getBoolean("spark.streaming.receiver.writeAheadLog.enable", false)
    if (enabled) {
      new ReliableEventHubReceiver(eventHubParams, partitionId, storageLevel, offsetStore, receiverClient)
    } else {
      new EventHubReceiver(eventHubParams, partitionId, storageLevel, offsetStore, receiverClient)
    }
  }
}
