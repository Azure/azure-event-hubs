/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import java.util.concurrent.ConcurrentHashMap

import com.microsoft.azure.eventhubs.EventData
import org.apache.spark.SparkEnv
import org.apache.spark.storage.{StorageLevel, StreamBlockId}
import org.apache.spark.streaming.receiver.{BlockGenerator, BlockGeneratorListener}

import scala.collection.mutable.ArrayBuffer

class ReliableEventHubReceiver(
  eventHubParams: Map[String, String],
  partitionId: String,
  storageLevel: StorageLevel,
  offsetStore: OffsetStore,
  client: EventHubInstance
) extends EventHubReceiver(eventHubParams, partitionId, storageLevel, offsetStore, client) {

  private var blockGenerator: BlockGenerator = null
  private var latestOffsetCurBlock: String = null
  private var blockOffsetMap: ConcurrentHashMap[StreamBlockId, String] = null

  override def onStop() : Unit = {
    super.onStop()

    if (blockGenerator != null) {
      blockGenerator.stop()
      blockGenerator = null
    }
    if (blockOffsetMap != null) {
      blockOffsetMap.clear()
      blockOffsetMap = null
    }
  }

  override def onStart(): Unit = {
    blockOffsetMap = new ConcurrentHashMap[StreamBlockId, String]

    blockGenerator = new BlockGenerator(new GeneratedBlockHandler, streamId, SparkEnv.get.conf)
    blockGenerator.start()

    super.onStart()
  }

  override def processReceivedMessage(events: Iterable[EventData]): Unit = {
    for (event <- events)
      blockGenerator.addDataWithCallback(event, event.getSystemProperties.getOffset)
  }

  private def storeBlockAndCommitOffset(
      blockId: StreamBlockId,
      arrayBuffer: ArrayBuffer[_]): Unit = {
    var count = 0
    var pushed = false
    var exception: Exception = null
    while (!pushed && count <= 3) {
      try {
        store(arrayBuffer.asInstanceOf[ArrayBuffer[EventData]])
        pushed = true
      } catch {
        case e: Exception =>
          count += 1
          exception = e
      }
    }
    if (pushed) {
      offsetToSave = blockOffsetMap.get(blockId)
      blockOffsetMap.remove(blockId)
    } else {
      stop("Error while storing block into Spark", exception)
    }
  }

  private final class GeneratedBlockHandler extends BlockGeneratorListener {

    override def onAddData(data: Any, metadata: Any): Unit = {
      if (metadata != null)
        latestOffsetCurBlock = metadata.asInstanceOf[String]
    }

    override def onGenerateBlock(blockId: StreamBlockId): Unit = {
      blockOffsetMap.put(blockId, latestOffsetCurBlock)
    }

    override def onPushBlock(blockId: StreamBlockId, arrayBuffer: ArrayBuffer[_]): Unit = {
      storeBlockAndCommitOffset(blockId, arrayBuffer)
    }

    override def onError(message: String, throwable: Throwable): Unit = {
      reportError(message, throwable)
    }
  }
}
