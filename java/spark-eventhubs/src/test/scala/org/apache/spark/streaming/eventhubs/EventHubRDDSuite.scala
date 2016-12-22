/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import com.microsoft.azure.eventhubs.EventData

import scala.concurrent.duration._
import org.apache.spark.{SparkConf, SparkContext}
import org.scalatest.concurrent.Eventually
import org.scalatest.{BeforeAndAfter, BeforeAndAfterAll, FunSuite}
import org.scalatest.mock.MockitoSugar

/**
  * Test suite for EventHubRDD
  */
class EventHubRDDSuite extends FunSuite with BeforeAndAfter with BeforeAndAfterAll
  with MockitoSugar with Eventually {
  private var sc: SparkContext = _
  private var ehClientMock: EventHubInstance = _
  private var offsetStoreMock: OffsetStore = _
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
    .setAppName("EventHubRDD")
    .set("spark.driver.allowMultipleContexts", "true")

  override def beforeAll() : Unit = {}

  override def afterAll() : Unit = {}

  before {
    sparkConf.set("spark.serializer", "org.apache.spark.serializer.KryoSerializer")
    sparkConf.registerKryoClasses(Array(classOf[EventData]))
    sc = new SparkContext(sparkConf)
    offsetStoreMock = new MyMockedOffsetStore
  }

  after {
    if (sc != null) {
      sc.stop()
      sc = null
    }
  }

  test("EventHubsUtils API works") {
    val offsetRange = OffsetRange("0", "-1", 100)
    val offsetRanges = OffsetRange.createArray(4, "-1", 100)
    EventHubUtils.createRDD(sc, ehParams, offsetRanges)
    EventHubUtils.createPartitionRDD(sc, ehParams, offsetRange)
    sc.stop()
  }

  test("EventHub RDD Test: Generate 100 event Partition RDD") {
    // after 100 messages ehClientMock will return null objects
    ehClientMock = new MyMockedEventHubInstance(100, -1)

    val offsetRange = OffsetRange("0", "-1", 100)
    val ehRDD = new EventHubRDD(sc, ehParams, None, Some(offsetRange), ehClientMock)

    val count = ehRDD.count
    sc.stop()

    eventually(timeout(4000.milliseconds), interval(200.milliseconds)) {
      // Make sure we have received 100 messages
      assert(count === 100)
    }
  }

  test("EventHub RDD Test: Generate 400 event RDD - 100 events for 4 partitions") {
    // after 100 messages ehClientMock will return null objects
    ehClientMock = new MyMockedEventHubInstance(400, -1)

    val offsetRanges = OffsetRange.createArray(4, "-1", 100)
    val ehRDD = new EventHubRDD(sc, ehParams, Some(offsetRanges), None, ehClientMock)

    val count = ehRDD.count
    sc.stop()

    eventually(timeout(4000.milliseconds), interval(200.milliseconds)) {
      // Make sure we have received 100 messages
      assert(count === 400)
    }
  }
}
