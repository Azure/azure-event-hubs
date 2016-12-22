/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
import com.microsoft.azure.eventhubs.EventData
import org.apache.spark.rdd.RDD
import org.apache.spark.streaming.eventhubs.{EventHubUtils, OffsetRange}
import org.apache.spark.{SparkConf, SparkContext}

/**
  * This example shows how to create a RDD using the EventHub/Spark adapter.
  * A RDD consumes events from a every EventHub partition. For more information see
  * the documentation here: https://github.com/azure/azure-event-hubs
  */
object RDDSample {
  def main(args: Array[String]): Unit = {
    if (args.length < 5) {
      System.err.println(
        s"""
           |Usage: PartitionRDDSample <namespaceName> <eventHubName> <sasKeyName> <sasKey>
           |  <namespaceName> is the EventHub namespace name
           |  <eventHubName> is the name of your EventHub instance
           |  <sasKeyName> is the name of the sasKey
           |  <sasKey> is the EventHub sasKey
           |  <partitionCount> is the number of partitions in the EventHub instance
         """.stripMargin)
    }

    val Array(namespaceName, eventHubName, sasKeyName, sasKey, numPartitions) = args

    val conf = new SparkConf().setAppName("RDDSample")
    val sc = new SparkContext(conf)

    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> namespaceName,
      "eventHubName" -> eventHubName,
      "sasKeyName" -> sasKeyName,
      "sasKey" -> sasKey)

    // A starting offset of -1 starts consuming from the beginning of the EventHub Instance
    // Unless explicitly done so (see lines 42,43), the same number of events will be consumed from the
    // same starting point for each partition.
    val offsetRanges1 = OffsetRange.createArray(numPartitions.toInt, startingOffset = "-1", batchSize = 50)
    val offsetRanges2 = OffsetRange.createArray(numPartitions.toInt, startingOffset = "-1", batchSize = 50)

    // For partition 0, starting at a different offset and consume a different number of events
    offsetRanges1(0).set(startingOffset = 50, batchSize = 100)

    println("creating RDDs....")
    val ehRDD1: RDD[EventData] = EventHubUtils.createRDD(sc, eventHubParams, offsetRanges1)
    val ehRDD2: RDD[EventData] = EventHubUtils.createRDD(sc, eventHubParams, offsetRanges2)

    //Combine the two RDDs and convert events from EventData to String
    val ehRDD3: RDD[String] = ehRDD1.union(ehRDD2).map[String] { case (x) => new String(x.getBody) }

    for (elem <- ehRDD3)
      println(elem)

    sc.stop()
  }
}
