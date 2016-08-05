/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
import org.apache.hadoop.conf.Configuration
import org.apache.spark.SparkConf
import org.apache.spark.streaming.eventhubs.EventHubUtils
import org.apache.spark.streaming.{Seconds, StreamingContext}

/**
  * This code sample shows how to consume tweets from EventHubs and display the most used hashtags
  * within the last 5 minutes. Of course, this assumes you're EventHub is filled with tweets. Please
  * see TwitterProdcuerSample.java for more info on how to do that.
  */
object TwitterSparkSample{
  def main(args : Array[String]): Unit = {
    if (args.length < 7) {
      System.err.println(
        s"""
         |Usage: PartitionRDDSample <namespaceName> <eventHubName> <sasKeyName> <sasKey>
         |  <namespaceName> is the EventHub namespace name
         |  <eventHubName> is the name of your EventHub instance
         |  <sasKeyName> is the name of the sasKey
         |  <sasKey> is the EventHub sasKey
         |  <blobAccountKey> is the account key for your Azure storage account
         |  <checkpointDir> is the directory checkpoint data will be stored in
         |  <partitionCount> is the number of partitions in your EventHub
         """.stripMargin)
    }

    val Array(namespaceName, eventHubName, sasKeyName, sasKey, accountKey, checkpointDir, partitionCount) = args

    // Be sure to set master appropriately - the number of number of threads should be greater
    // than the number of receivers. Same goes for cores if deployed on a cluster.
    val conf = new SparkConf().setAppName("Spark/Twitter/EventHub Demo").setMaster("MASTER")
    conf.set("spark.streaming.ui.retainedBatches", "5")
    conf.set("spark.streaming.backpressure.enabled", "true")

    val ssc = new StreamingContext(conf, batchDuration = Seconds(1))

    // We're using Azure Blob Storage for checkpointing here.
    val config: Configuration = ssc.sparkContext.hadoopConfiguration
    config.set("fs.azure", "org.apache.hadoop.fs.azure.NativeAzureFileSystem")
    config.set("fs.azure.account.key.sgrewasparkcheckpoint.blob.core.windows.net", accountKey)

    // NOTE: For this example, the checkpointDirectory must correspond to your Blob Storage using the "wasb" scheme
    // Example: "wasb://container@accountName.blob.core.windows.net/"
    ssc.checkpoint(checkpointDir)

    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> namespaceName,
      "eventHubName" -> eventHubName,
      "sasKeyName" -> sasKeyName,
      "sasKey" -> sasKey,
      "checkpointDir" -> checkpointDir)

    // Create DStream and convert EventData to Strings.
    val eventHubDStream = EventHubUtils.createStream(ssc, eventHubParams, partitionCount.toInt)
      .map[String] { eventData => new String(eventData.getBody) }

    // Splits each String around a space, quotation, or newline character
    val words = eventHubDStream.flatMap(status => status.split(" |\"|\n"))

    // Find all words that start with #
    val hashTags = words.filter(word => word.startsWith("#"))

    // Add counts of new data to existing data. Old data is removed from window.
    val counts = hashTags.map(tag => (tag, 1))
      .reduceByKeyAndWindow(_ + _, _ - _, Seconds(60 * 5), Seconds(1))

    // Sort in descending order
    val sortedCounts = counts.map { case(tag, count) => (count, tag)}
      .transform(rdd => rdd.sortByKey(false))

    // Output first 10 elements from RDD
    sortedCounts.foreachRDD(rdd =>
      println("\nTop 10 hashtags:\n" + rdd.take(10).mkString("\n")))

    ssc.start()
    ssc.awaitTermination()
  }
}
