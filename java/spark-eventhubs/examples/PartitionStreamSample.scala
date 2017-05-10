/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
import org.apache.spark.streaming.{Seconds, StreamingContext}
import org.apache.spark.streaming.eventhubs.EventHubUtils
import org.apache.spark.SparkConf

/**
  * This example shows how to create a PartitionStream using the EventHub/Spark adapter.
  * A PartitionStream will consume events from a single EventHub partition.
  * A partitionStream will produce a DStream containing EventData. For more info please see
  * the documentation here: https://github.com/azure/azure-event-hubs
  */
object PartitionStreamSample{
  def main(args: Array[String]): Unit = {
    if (args.length < 5) {
      System.err.println(
        s"""
           |Usage: PartitionRDDSample <namespaceName> <eventHubName> <sasKeyName> <sasKey>
           |  <namespaceName> is the EventHub namespace name
           |  <eventHubName> is the name of your EventHub instance
           |  <sasKeyName> is the name of the sasKey
           |  <sasKey> is the EventHub sasKey
           |  <partitionId> is the EventHub partition that will be consumed from
           |  <checkpointDir> is where checkpoint data will be stored
         """.stripMargin)
    }
    val Array(namespaceName, eventHubName, sasKeyName, sasKey, partitionId, checkpointDir) = args

    // Set master appropriately. It's set to 4 to ensure we have more threads than receivers.
    val conf = new SparkConf().setAppName("PartitionStreamSample").setMaster("local[4]")
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

    println("Setting up Partition DStream...")
    val partitionDStream = EventHubUtils.createPartitionStream(ssc, eventHubParams, partitionId)
      .map[String] { eventData => new String(eventData.getBody)}

    partitionDStream.foreachRDD(rdd =>
      for (elem <- rdd)
        println(elem)
    )

    println("starting streaming context...")
    ssc.start()
    ssc.awaitTermination()
  }
}
