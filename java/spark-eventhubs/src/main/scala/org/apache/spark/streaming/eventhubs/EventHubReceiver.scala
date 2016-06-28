/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import com.microsoft.azure.eventhubs.EventData
import org.apache.spark.Logging
import org.apache.spark.storage.StorageLevel
import org.apache.spark.streaming.receiver.Receiver
import org.apache.spark.util.ThreadUtils

import scala.collection.JavaConverters._
import scala.util.control.ControlThrowable

private[eventhubs]
class EventHubReceiver(
  eventHubParams: Map[String, String],
  partitionId: String,
  storageLevel: StorageLevel,
  offsetStore: OffsetStore,
  client: EventHubInstance
) extends Receiver[EventData](storageLevel) with Logging {

  var myOffsetStore: OffsetStore = offsetStore
  if(myOffsetStore == null) {
    myOffsetStore = new DfsBasedOffsetStore(
      eventHubParams("checkpointDir"),
      eventHubParams("namespaceName"),
      eventHubParams("eventHubName"),
      partitionId
    )
  }

  private var stopMessageHandler = false
  private var latestSequenceNo: Long = Long.MinValue
  protected var offsetToSave: String = null
  protected var savedOffset: String = null

  def onStop(): Unit = {
    logInfo(s"Stopping EventHubReceiver for partitionId: $partitionId")
    stopMessageHandler = true
  }

  def onStart(): Unit = {
    logInfo(s"Starting EventHubReceiver for partitionId: $partitionId")

    stopMessageHandler = false
    val executorPool =
      ThreadUtils.newDaemonFixedThreadPool(1, "EventHubMessageHandler")
    try {
      executorPool.submit(new EventHubMessageHandler)
    } finally {
      executorPool.shutdown()
    }
  }

  def processReceivedMessage(events: Iterable[EventData]): Unit = {
    for (event <- events) {
      if (event.getSystemProperties.getSequenceNumber > latestSequenceNo) {
        latestSequenceNo = event.getSystemProperties.getSequenceNumber
        store(event)
        offsetToSave = event.getSystemProperties.getOffset
      }
    }
  }

  private[eventhubs]
  class EventHubMessageHandler extends Runnable {
    val checkpointInterval = eventHubParams.getOrElse("checkpointInterval", "10000").toInt
    var nextTime = System.currentTimeMillis() + checkpointInterval

    def run(): Unit = {
      logInfo(s"Begin EventHubMessageHandler for partition $partitionId")

      try {
        myOffsetStore.open()
        client.createReceiver(eventHubParams, partitionId, myOffsetStore.read())

        //TODO need to change hardcoding
        while (!stopMessageHandler) {
          val batch = client.receiver.receive(1).get
          if(batch != null)
            processReceivedMessage(batch.asScala)
        }

        val now = System.currentTimeMillis()
        if(now > nextTime) {
          if(offsetToSave != savedOffset) {
            logInfo(s"Saving offset: $offsetToSave for partitionId: $partitionId")
            myOffsetStore.write(offsetToSave)
            savedOffset = offsetToSave
            nextTime = now + checkpointInterval
          }
        }
      } catch {
        case c: ControlThrowable => throw c
        case e: Throwable =>
          restart("Error handling message; restarting receiver", e)
      } finally {
        myOffsetStore.close()
        client.close
        logInfo(s"Closing EventHubMessageHandler for partitionId: $partitionId")
      }
    }
  }
}
