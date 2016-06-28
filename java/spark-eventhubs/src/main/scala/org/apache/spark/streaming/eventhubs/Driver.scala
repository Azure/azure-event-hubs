/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import org.apache.spark.SparkConf
import org.apache.spark.streaming.{Seconds, StreamingContext}

object Driver {
  def main(args : Array[String]): Unit = {
    Samples.streamSample()
    //Samples.partitionStreamSample()
  }
}

object Samples {
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
      "checkpointDir" -> "C:\\Users\\t-sgrewa\\Documents",
      "partitionCount" -> "4")

    println("Setting up stream...")
    val stream = EventHubUtils.createStream(ssc, eventHubParams)
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