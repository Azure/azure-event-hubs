/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import org.apache.spark.SparkException
import org.scalatest.FunSuite

class EventHubInstanceSuite extends FunSuite {
  var clientMock: EventHubInstance = new EventHubInstance
  var partitionIdMock: String = "0"
  var startingOffsetMock: String = "0"

  test("Exception is thrown if namespaceName is not specified") {
    val eventHubParams: Map[String, String] = Map(
      "eventHubName" -> "XXXXX",
      "sasKeyName" -> "SAS_KEY_NAME",
      "sasKey" -> "SAS_KEY")
    val thrown = intercept[SparkException] {
      clientMock.createReceiver(eventHubParams, partitionIdMock, startingOffsetMock)
    }
    assert(thrown.getMessage === "Must specify namespaceName")
  }

  test("Exception is thrown if eventHubName is not specified") {
    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> "namespace",
      "sasKeyName" -> "SAS_KEY_NAME",
      "sasKey" -> "SAS_KEY")
    val thrown = intercept[SparkException] {
      clientMock.createReceiver(eventHubParams, partitionIdMock, startingOffsetMock)
    }
    assert(thrown.getMessage === "Must specify eventHubName")
  }

  test("Exception is thrown if sasKeyName is not specified") {
    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> "namespace",
      "eventHubName" -> "XXXXX",
      "sasKey" -> "SAS_KEY")
    val thrown = intercept[SparkException] {
      clientMock.createReceiver(eventHubParams, partitionIdMock, startingOffsetMock)
    }
    assert(thrown.getMessage === "Must specify sasKeyName")
  }

  test("Exception is thrown if sasKey is not specified") {
    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> "namepsace",
      "eventHubName" -> "XXXXX",
      "sasKeyName" -> "SAS_KEY_NAME")
    val thrown = intercept[SparkException] {
      clientMock.createReceiver(eventHubParams, partitionIdMock, startingOffsetMock)
    }
    assert(thrown.getMessage === "Must specify sasKey")
  }
}
