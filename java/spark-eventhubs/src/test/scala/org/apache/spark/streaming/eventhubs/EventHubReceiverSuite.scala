/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import com.microsoft.azure.eventhubs.EventData
import com.microsoft.azure.eventhubs.EventData.SystemProperties

import org.apache.spark.storage.StorageLevel
import org.apache.spark.streaming.{StreamingContext, TestSuiteBase}
import org.apache.spark.streaming.receiver.ReceiverSupervisor

import org.mockito.Mockito._
import org.scalatest.Matchers
import org.scalatest.mock.MockitoSugar

/**
  * Test suite for EventHub Receiver
  */
class EventHubReceiverSuite extends TestSuiteBase with Matchers with MockitoSugar {
  var ehClientMock: EventHubInstance = _
  var offsetStoreMock: OffsetStore = _
  var executorMock: ReceiverSupervisor = _
  val ehParams = Map[String, String] (
    "namespaceName" -> "namespace",
    "eventHubName" -> "name",
    "sasKeyName" -> "keyname",
    "sasKey" -> "key",
    "checkpointDir" -> "checkpointdir",
    "checkpointInterval" -> "0"
  )

  override def beforeFunction() = {
    ehClientMock = mock[EventHubInstance]
    offsetStoreMock = mock[OffsetStore]
    executorMock = mock[ReceiverSupervisor]
  }

  override def afterFunction(): Unit = {
    super.afterFunction()
    // Since this suite was originally written using EasyMock, add this to preserve the old
    // mocking semantics (see SPARK-5735 for more details)
    //verifyNoMoreInteractions(ehClientMock, offsetStoreMock)
  }

  test("EventHubsUtils API works") {
    val ssc = new StreamingContext(master, framework, batchDuration)
    EventHubUtils.createStream(ssc, ehParams, 4, StorageLevel.MEMORY_ONLY_2)
    EventHubUtils.createPartitionStream(ssc, ehParams, partitionId = "0", StorageLevel.MEMORY_ONLY_2)
    ssc.stop()
  }

  test("EventHubsReceiver can receive message with proper checkpointing") {
    // Mock object setup
    val data = new MyMockedEventData("mock".getBytes, "123", 456)
    val batch = new java.util.LinkedList[EventData]
    batch.add(data)

    when(offsetStoreMock.read()).thenReturn("-1")
    when(ehClientMock.receive(1))
      .thenReturn(batch)
      .thenReturn(null)

    val receiver = new EventHubReceiver(ehParams, "0", StorageLevel.MEMORY_ONLY,
      offsetStoreMock, ehClientMock)
    receiver.attachSupervisor(executorMock)

    receiver.onStart()
    Thread sleep 100
    receiver.onStop()

    verify(offsetStoreMock, times(1)).open()
    verify(offsetStoreMock, times(1)).write("123")
    verify(offsetStoreMock, times(1)).close()

    verify(ehClientMock, times(1)).createReceiver(ehParams, "0", "-1")
    verify(ehClientMock, atLeastOnce).receive(1)
    verify(ehClientMock, times(1)).close
  }

  test("EventHubsReceiver can restart when exception is thrown") {
    // Mock object setup
    val data = new MyMockedEventData("mock".getBytes, "123", 456)
    val batch = new java.util.LinkedList[EventData]
    batch.add(data)
    val exception = new RuntimeException("error")

    when(offsetStoreMock.read()).thenReturn("-1")
    when(ehClientMock.receive(1))
      .thenReturn(batch)
      .thenThrow(exception) // then throw

    val receiver = new EventHubReceiver(ehParams, "0", StorageLevel.MEMORY_ONLY,
      offsetStoreMock, ehClientMock)
    receiver.attachSupervisor(executorMock)

    receiver.onStart()
    Thread sleep 100
    receiver.onStop()

    // Verify that executor.restartReceiver() has been called
    verify(executorMock, times(1))
      .restartReceiver("Error handling message; restarting receiver", Some(exception))

    verify(offsetStoreMock, times(1)).open()
    verify(offsetStoreMock, times(1)).close()

    verify(ehClientMock, times(1)).createReceiver(ehParams, "0", "-1")
    verify(ehClientMock, times(2)).receive(1)
    verify(ehClientMock, times(1)).close
  }
}

class MyMockedEventData (
      body: Array[Byte],
      offSet: String,
      sequenceNumber: Long) extends EventData(body) {
  override def getSystemProperties : MyMockedSystemProperties = new MyMockedSystemProperties
  override def getBody = { body }


  class MyMockedSystemProperties extends SystemProperties {
    override def getSequenceNumber = { sequenceNumber }
    override def getOffset = { offSet }
  }
}