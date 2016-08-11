/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import java.time.Instant

import com.microsoft.azure.eventhubs.{EventHubClient, PartitionReceiver}
import com.microsoft.azure.servicebus.ConnectionStringBuilder
import org.apache.spark.SparkException

/**
  * Class to wrap event hub client and receiver. This class ensures that the client and receiver
  * are created on the correct machine in the Spark Cluster.
  */
class EventHubInstance extends Serializable {
  private var ehClient: EventHubClient = null
  private var receiver: PartitionReceiver = null

  protected def createClient(eventHubParams: Map[String, String]): Unit = {
    val namespaceName: String = eventHubParams.getOrElse("namespaceName",
      throw new SparkException("Must specify namespaceName"))
    val eventHubName: String = eventHubParams.getOrElse("eventHubName",
      throw new SparkException("Must specify eventHubName"))
    val sasKeyName: String = eventHubParams.getOrElse("sasKeyName",
      throw new SparkException("Must specify sasKeyName"))
    val sasKey: String = eventHubParams.getOrElse("sasKey",
      throw new SparkException("Must specify sasKey"))

    val connStr: ConnectionStringBuilder = new ConnectionStringBuilder(namespaceName, eventHubName, sasKeyName, sasKey)
    ehClient = EventHubClient.createFromConnectionString(connStr.toString).get
  }

  def createReceiver(
      eventHubParams: Map[String, String],
      partitionId: String,
      startingOffset: String): Unit = {

    createClient(eventHubParams)
    if (ehClient == null)
      throw new SparkException("Error making EventHubs client")

    val consumerGroup: String = eventHubParams.getOrElse("consumerGroupName",
      EventHubClient.DEFAULT_CONSUMER_GROUP_NAME)

    receiver = if(startingOffset == "latest") {
      ehClient.createReceiver(
        consumerGroup,
        partitionId,
        Instant.now()
      ).get
    } else {
      ehClient.createReceiver(
        consumerGroup,
        partitionId,
        startingOffset,
        false).get
    }

    receiver.setReceiveTimeout(java.time.Duration.ofSeconds(5))
  }

  def receive(batchSize: Int) = { receiver.receive(batchSize).get }

  def close(): Unit = {
    receiver.close()
    ehClient.close()
  }
}
