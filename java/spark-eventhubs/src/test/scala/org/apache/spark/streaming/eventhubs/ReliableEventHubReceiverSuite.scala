/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import java.io.File

import com.microsoft.azure.eventhubs.EventData
import org.apache.spark.util.Utils

import scala.concurrent.duration._
import org.apache.spark.storage.StorageLevel
import org.apache.spark.SparkConf
import org.apache.spark.streaming.{Milliseconds, StreamingContext}
import org.scalatest.concurrent.Eventually
import org.scalatest.{BeforeAndAfter, BeforeAndAfterAll, FunSuite}
import org.scalatest.mock.MockitoSugar

/**
  * Test suite for ReliableEventHubReceiver
  */
class ReliableEventHubsReceiverSuite extends FunSuite with BeforeAndAfter with BeforeAndAfterAll
  with MockitoSugar with Eventually {
  private var ssc: StreamingContext = _
  private var ehClientMock: EventHubInstance = _
  private var offsetStoreMock: OffsetStore = _
  private var tempDirectory: File = null
  val ehParams = Map[String, String] (
    "namespaceName" -> "namespace",
    "eventHubName" -> "name",
    "sasKeyName" -> "keyname",
    "sasKey" -> "key",
    "checkpointDir" -> "checkpointdir",
    "checkpointInterval" -> "0"
  )

  private val sparkConf = new SparkConf()
    .setMaster("local[2]") // At least 2, 1 for receiver and 1 for data transform
    .setAppName("ReliableEventHubReceiverSuite")
    .set("spark.streaming.receiver.writeAheadLog.enable", "true")
    .set("spark.driver.allowMultipleContexts", "true")

  override def beforeAll() : Unit = {}

  override def afterAll() : Unit = {}

  before {
    tempDirectory = Utils.createTempDir()
    //tempDirectory.deleteOnExit()
    sparkConf.set("spark.serializer", "org.apache.spark.serializer.KryoSerializer")
    sparkConf.registerKryoClasses(Array(classOf[EventData]))
    ssc = new StreamingContext(sparkConf, Milliseconds(500))
    ssc.checkpoint(tempDirectory.getAbsolutePath)

    offsetStoreMock = new MyMockedOffsetStore
  }

  after {
    if (ssc != null) {
      ssc.stop()
      ssc = null
    }
    if(tempDirectory != null) {
      // Utils.deleteRecursively(tempDirectory)
      tempDirectory.delete()
      tempDirectory = null
    }
  }

  test("Reliable EventHub DStream Test: Streams 100 events reliably") {
    // after 100 messages ehClientMock will return null objects
    ehClientMock = new MyMockedEventHubInstance(100, -1)

    val ehPartitionStream = ssc.receiverStream(
      new ReliableEventHubReceiver(ehParams, "0", StorageLevel.MEMORY_ONLY, offsetStoreMock, ehClientMock))

    var count = 0
    ehPartitionStream.map {v => v}.foreachRDD { r =>
      val ret = r.collect()
      ret.foreach { v => count += 1}
    }
    ssc.start()

    eventually(timeout(4000.milliseconds), interval(200.milliseconds)) {
      // Make sure we have received 100 messages
      assert(count === 100)
    }
  }

  test("Reliable EventHub DStream recovers when exception is thrown") {
    // After 60 messages ehClientMock will throw an exception
    // After 40 more messages, null will be returned - 100 messages in total
    ehClientMock = new MyMockedEventHubInstance(100, 60)

    val ehPartitionStream = ssc.receiverStream(
      new ReliableEventHubReceiver(ehParams, "0", StorageLevel.MEMORY_ONLY, offsetStoreMock, ehClientMock))

    var count = 0
    ehPartitionStream.map {v => v}.foreachRDD { r =>
      val ret = r.collect()
      ret.foreach { v => count += 1}
    }
    ssc.start()

    eventually(timeout(10000.milliseconds), interval(200.milliseconds)) {
      // Make sure we have received 100 messages
      assert(count === 100)
    }
  }
}

/**
  * The Mock class for EventHubsInstance.
  * Sequence number and offset will always be equal.
  *
  * @param emitCount number of EventData returned. Once emitCount is reached, null will be returned
  * @param exceptionCount the number of messages emitted before it throws exception
  *                       only one exception will be thrown. Set to -1 to avoid an exception being thrown
  */
class MyMockedEventHubInstance(
    emitCount: Int,
    exceptionCount: Int) extends EventHubInstance {
  var offset = -1
  var count = 0
  var myExceptionCount = exceptionCount
  val data = "mock".getBytes

  override def createClient(eventHubParams: Map[String, String]): Unit = {}

  override def createReceiver(
     eventHubParams: Map[String, String],
     partitionId: String,
     startingOffset: String): Unit = {}

  override def receive(batchSize: Int): java.lang.Iterable[EventData] = {
    if(count == myExceptionCount) {
      // make sure we only throw exception once
      myExceptionCount = -1
      throw new RuntimeException("count = " + count)
    }

    val batch = new java.util.LinkedList[EventData]
    offset += 1
    count += 1
    // do not send more than emitCount number of messages
    if(count <= emitCount) {
      batch.add(new MyMockedEventData("mock".getBytes, offset.toString, offset))
      batch
    }
    else {
      Thread sleep 1000
      null
    }
  }

  override def close(): Unit = {}
}

/**
  * The Mock class for OffsetStore
  */
class MyMockedOffsetStore extends OffsetStore {
  var myOffset: String = "-1"
  override def open(): Unit = {}

  override def write(offset: String): Unit = {
    println("writing offset to MyMockedOffsetStore:" + offset)
    myOffset = offset
  }

  override def read(): String = {
    println("reading offset from MyMockedOffsetStore:" + myOffset)
    myOffset
  }

  override def close(): Unit = {}
}
