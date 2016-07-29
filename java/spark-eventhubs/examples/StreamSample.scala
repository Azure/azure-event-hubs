/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
import org.apache.spark.streaming.{Seconds, StreamingContext}
import org.apache.spark.streaming.eventhubs.EventHubUtils
import org.apache.spark.SparkConf

/**
  * This example shows how to create a Stream using the EventHub/Spark adapter.
  * A Stream will consume events from all EventHub partitions. A Stream will produce
  * a DStream containing EventData. For more info please see the documentation here:
  * https://github.com/azure/azure-event-hubs
  */
object StreamSample{
  def main(args: Array[String]): Unit = {
    if (args.length < 5) {
      System.err.println(
        s"""
           |Usage: PartitionRDDSample <namespaceName> <eventHubName> <sasKeyName> <sasKey>
           |  <namespaceName> is the EventHub namespace name
           |  <eventHubName> is the name of your EventHub instance
           |  <sasKeyName> is the name of the sasKey
           |  <sasKey> is the EventHub sasKey
           |  <partitionCount> is the number of partitions in the EventHub
           |  <checkpointDir> is where checkpoint data will be stored
         """.stripMargin)
    }
    val Array(namespaceName, eventHubName, sasKeyName, sasKey, partitionCount, checkpointDir) = args

    // Set master appropriately. Number of cores/threads must be greater than the number of receivers.
    // Core/threads depends on if run locally in or in a cluster.
    val conf = new SparkConf().setAppName("StreamSample").setMaster("MASTER")
    conf.set("spark.streaming,ui.retainedBatches", "5")
    conf.set("spark.streaming.backpressure.enabled", "true")

    val ssc = new StreamingContext(conf, batchDuration = Seconds(2))
    ssc.checkpoint(checkpointDir)

    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> namespaceName,
      "eventHubName" -> eventHubName,
      "sasKeyName" -> sasKeyName,
      "sasKey" -> sasKey,
      "checkpointDir" -> checkpointDir)

    val stream = EventHubUtils.createStream(ssc, eventHubParams, partitionCount.toInt)
      .map[String] { eventData => new String(eventData.getBody)}

    stream.foreachRDD(rdd =>
      for (elem <- rdd)
        println(elem)
    )

    println("starting streaming context...")
    ssc.start()
    ssc.awaitTermination()
  }
}
