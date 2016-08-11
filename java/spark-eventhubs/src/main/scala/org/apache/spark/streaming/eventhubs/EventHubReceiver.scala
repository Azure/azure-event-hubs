/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import com.microsoft.azure.eventhubs.EventData
import com.microsoft.azure.servicebus.ServiceBusException
import org.apache.spark.Logging
import org.apache.spark.storage.StorageLevel
import org.apache.spark.streaming.receiver.Receiver
import org.apache.spark.util.ThreadUtils

import scala.collection.JavaConverters._
import scala.util.control.ControlThrowable

/**
  * The receiver used by the the ReceiverInputDStream in createStream and createPartitionStream
  * within EventHubUtils
  */
private[eventhubs]
class EventHubReceiver(
  eventHubParams: Map[String, String],
  partitionId: String,
  storageLevel: StorageLevel,
  offsetStore: OffsetStore,
  client: EventHubInstance
) extends Receiver[EventData](storageLevel) with Logging {

  /** Create offset store if one does not exist*/
  var myOffsetStore: OffsetStore = offsetStore
  if(myOffsetStore == null) {
    myOffsetStore = new DfsBasedOffsetStore(
      eventHubParams("checkpointDir"),
      eventHubParams("namespaceName"),
      eventHubParams("eventHubName"),
      partitionId
    )
  }

  /** Flag to communicate from main thread to the MessageHandler thread
    * that the message handler should be stopped. */
  private var stopMessageHandler = false

  /** Latest sequence number received from Event Hubs. Used to throw away duplicate
    * messages when receiver gets restarted. */
  private var latestSequenceNo: Long = Long.MinValue

  /** Offset that will be saved at the end of current checkpoint interval. */
  protected var offsetToSave: String = null

  /** The last offset that was successfully stored. */
  protected var savedOffset: String = null

  def onStop(): Unit = {
    logInfo(s"Stopping EventHubReceiver for partitionId: $partitionId")
    stopMessageHandler = true
    // All other clean up is done in the message handler thread
  }

  def onStart(): Unit = {
    logInfo(s"Starting EventHubReceiver for partitionId: $partitionId")

    stopMessageHandler = false
    val executorPool =
      ThreadUtils.newDaemonFixedThreadPool(1, "EventHubMessageHandler")
    try {
      executorPool.submit(new EventHubMessageHandler)
    } finally {
      executorPool.shutdown() // exits thread when everything is done
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

    def retry[T](n: Int)(fn: => T): Unit = {
      try {
        fn
      } catch {
        case c: ControlThrowable => throw c
        case s: ServiceBusException =>
          if (s.getIsTransient) {
            if (n > 1) {
              logInfo(s"ServiceBusException caught. Retrying receiver...")
              retry(n - 1)(fn)
            }
          }
          restart("Error handling message; restarting receiver", s)
        case e: Throwable =>
          restart("Error handling message; restarting receiver", e)
      }
    }

    def run(): Unit = {
      logInfo(s"Begin EventHubMessageHandler for partition $partitionId")

      retry(3) {
        // if offset isn't available in offset store, we use -1 as starting offset
        myOffsetStore.open()
        val offset = myOffsetStore.read()
        logInfo(s"Retrieved offset $offset from offset store. NOTE: default value is -1 if no offset store is provided")
        client.createReceiver(eventHubParams, partitionId, offset)

        while (!stopMessageHandler) {
          val batch = client.receive(1)
          if (batch != null)
            processReceivedMessage(batch.asScala)

          val now = System.currentTimeMillis()
          if (now > nextTime) {
            if (offsetToSave != savedOffset) {
              logInfo(s"Saving offset: $offsetToSave for partitionId: $partitionId")
              myOffsetStore.write(offsetToSave)
              savedOffset = offsetToSave
              nextTime = now + checkpointInterval
            }
            logInfo(s"Offset wasn't stored because offset hasn't change since last checkpoint")
          }
        }
      }
      myOffsetStore.close()
      client.close()
      logInfo(s"Closing EventHubMessageHandler for partitionId: $partitionId")
    }
  }
}
