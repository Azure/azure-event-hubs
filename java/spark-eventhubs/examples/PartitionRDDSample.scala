/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
import com.microsoft.azure.eventhubs.EventData
import org.apache.spark.rdd.RDD
import org.apache.spark.streaming.eventhubs.{EventHubUtils, OffsetRange}
import org.apache.spark.{SparkConf, SparkContext}

/**
  * This example shows how to create a partitionRDD using the EventHub/Spark adapter.
  * A partitionRDD consumes events from a single EventHub partition. For more information see
  * the documentation here: https://github.com/azure/azure-event-hubs
  */
object PartitionRDDSample {
  def main(args: Array[String]): Unit = {
    if (args.length < 4) {
      System.err.println(
        s"""
           |Usage: PartitionRDDSample <namespaceName> <eventHubName> <sasKeyName> <sasKey>
           |  <namespaceName> is the EventHub namespace name
           |  <eventHubName> is the name of your EventHub instance
           |  <sasKeyName> is the name of the sasKey
           |  <sasKey> is the EventHub sasKey
         """.stripMargin)
    }

    val Array(namespaceName, eventHubName, sasKeyName, sasKey) = args

    val conf = new SparkConf().setAppName("PartitionRDDSample")
    val sc = new SparkContext(conf)

    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> namespaceName,
      "eventHubName" -> eventHubName,
      "sasKeyName" -> sasKeyName,
      "sasKey" -> sasKey)

    // A starting offset of -1 starts consuming from the beginning of the EventHub Instance
    val offsetRange1 = OffsetRange(partitionId = "1", startingOffset = -1, batchSize = 50)
    val offsetRange2 = OffsetRange(partitionId = "3", startingOffset = -1, batchSize = 50)

    println("creating RDDs....")
    val ehRDD1: RDD[EventData] = EventHubUtils.createPartitionRDD(sc, eventHubParams, offsetRange1)
    val ehRDD2: RDD[EventData] = EventHubUtils.createPartitionRDD(sc, eventHubParams, offsetRange2)

    //Combine the two RDDs and convert events from EventData to String
    val ehRDD3: RDD[String] = ehRDD1.union(ehRDD2).map[String] { case (x) => new String(x.getBody) }

    for (elem <- ehRDD3)
      println(elem)

    sc.stop()
  }
}
