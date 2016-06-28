/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import com.microsoft.azure.eventhubs.EventData
import org.apache.spark.rdd.RDD
import org.apache.spark.{SparkConf, SparkContext}
import org.apache.spark.streaming.{Seconds, StreamingContext}

object Driver {
  def main(args : Array[String]): Unit = {
    Samples.partitionRDDSample()
    //Samples.rddSample()
    //Samples.streamSample()
    //Samples.partitionStreamSample()
  }
}

object Samples {
  //Create a partition RDD and output messages
  def partitionRDDSample(): Unit = {
    val conf = new SparkConf().setAppName("EventHubs-Spark Test").setMaster("local")
    val sc = new SparkContext(conf)
    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> "sgrewa-test-ns",
      "eventHubName" -> "sgrewa-test",
      "sasKeyName" -> "RootManageSharedAccessKey",
      "sasKey" -> "F72qroEfDwPkuGrjn6mVVajTvHt5O7SlUn25fIosYpE=")
    val offsetRange1 = OffsetRange(partitionId = "1", startingOffset = -1, batchSize = 50)
    val offsetRange2 = OffsetRange(partitionId = "3", startingOffset = -1, batchSize = 50)

    println("creating RDDs....")
    val ehRDD1: RDD[EventData] = EventHubUtils.createPartitionRDD(sc, eventHubParams, offsetRange1)
    val ehRDD2: RDD[EventData] = EventHubUtils.createPartitionRDD(sc, eventHubParams, offsetRange2)

    val ehRDD3: RDD[String] = ehRDD1.union(ehRDD2).map[String] { case (x) => new String(x.getBody) }

    for (elem <- ehRDD3)
      println(elem)

    sc.stop()
  }

  //Create a RDD and output messages
  def rddSample(): Unit = {
    val conf = new SparkConf().setAppName("EventHubs-Spark Test").setMaster("local")
    val sc = new SparkContext(conf)
    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> "sgrewa-test-ns",
      "eventHubName" -> "sgrewa-test",
      "sasKeyName" -> "RootManageSharedAccessKey",
      "sasKey" -> "F72qroEfDwPkuGrjn6mVVajTvHt5O7SlUn25fIosYpE=")
    val offsetRanges = OffsetRange.createArray(numPartitions = 4, startingOffset = -1, batchSize = 50)

    println("creating RDDs....")
    val ehRDD1: RDD[EventData] = EventHubUtils.createRDD(sc, eventHubParams, offsetRanges)
    val ehRDD2: RDD[EventData] = EventHubUtils.createRDD(sc, eventHubParams, offsetRanges)

    val ehRDD3: RDD[String] = ehRDD1.union(ehRDD2).map[String] { case (x) => new String(x.getBody) }

    for (elem <- ehRDD3)
      println(elem)

    sc.stop()
  }
  //Create a stream and output messages
  def streamSample(): Unit = {
    val conf = new SparkConf().setAppName("Event Hub Partition DStream Test").setMaster("local[6]")
    conf.set("spark.streaming,ui.retainedBatches", "5")
    conf.set("spark.streaming.backpressure.enabled", "true")

    val ssc = new StreamingContext(conf, batchDuration = Seconds(2))
    ssc.checkpoint("_checkpoint")

    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> "sgrewa-test-ns",
      "eventHubName" -> "sgrewa-test",
      "sasKeyName" -> "RootManageSharedAccessKey",
      "sasKey" -> "F72qroEfDwPkuGrjn6mVVajTvHt5O7SlUn25fIosYpE=",
      "checkpointDir" -> "C:\\Users\\t-sgrewa\\Documents")

    println("Setting up stream...")
    val stream = EventHubUtils.createStream(ssc, eventHubParams, partitionCount = 4)
      .map[String] { eventData => new String(eventData.getBody)}

    stream.foreachRDD(rdd =>
      for (elem <- rdd)
        println(elem)
    )

    println("starting streaming context...")
    ssc.start()
    ssc.awaitTermination()
  }

  //Create a PartitionStream and output messages
  def partitionStreamSample(): Unit = {
    val conf = new SparkConf().setAppName("Event Hub Partition DStream Test").setMaster("local[4]")
    conf.set("spark.streaming,ui.retainedBatches", "5")
    conf.set("spark.streaming.backpressure.enabled", "true")

    val ssc = new StreamingContext(conf, batchDuration = Seconds(2))
    ssc.checkpoint("_checkpoint")

    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> "sgrewa-test-ns",
      "eventHubName" -> "sgrewa-test",
      "sasKeyName" -> "RootManageSharedAccessKey",
      "sasKey" -> "F72qroEfDwPkuGrjn6mVVajTvHt5O7SlUn25fIosYpE=",
      "checkpointDir" -> "C:\\Users\\t-sgrewa\\Documents")

    println("Setting up Partition DStream...")
    val partitionDStream = EventHubUtils.createPartitionStream(ssc, eventHubParams, partitionId = "1")
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