package Test

import org.apache.hadoop.conf.Configuration
import org.apache.spark.SparkConf
import org.apache.spark.streaming.eventhubs.EventHubUtils
import org.apache.spark.streaming.{Seconds, StreamingContext}

object App {
  def main(args : Array[String]): Unit = {
    //val conf = new SparkConf().setAppName("Spark/Twitter/EventHub Demo").setMaster("local[24]")
    val conf = new SparkConf().setAppName("Spark/Twitter/EventHub Demo").setMaster("spark://13.93.214.223:7077")
    conf.set("spark.streaming.ui.retainedBatches", "5")
    conf.set("spark.streaming.backpressure.enabled", "true")


    val ssc = new StreamingContext(conf, batchDuration = Seconds(1))
    val config: Configuration = ssc.sparkContext.hadoopConfiguration
    config.set("fs.hdfs.impl", classOf[org.apache.hadoop.hdfs.DistributedFileSystem].getName)
    config.set("fs.file.impl", classOf[org.apache.hadoop.fs.LocalFileSystem].getName)

    config.set("fs.azure", "org.apache.hadoop.fs.azure.NativeAzureFileSystem")
    config.set("fs.azure.account.key.sgrewasparkcheckpoint.blob.core.windows.net",
      "5SL1gISNlqHmbhTbocvNy4nz7WeE6hWI5iWUt+zn66Y1Yfpf07lCkRoSzjeEYcFACM1OTqH3tKaD7ZAKM4E/2w==")

    ssc.checkpoint("wasb://checkpoint@sgrewasparkcheckpoint.blob.core.windows.net/")

    val eventHubParams: Map[String, String] = Map(
      "namespaceName" -> "sgrewa-test2-ns",
      "eventHubName" -> "sgrewa-test2",
      "sasKeyName" -> "RootManageSharedAccessKey",
      "sasKey" -> "be3/2AVv98EiR0IhAuUN2sSUvvkLlaepWBn2tF4VU80=",
      "checkpointDir" -> "wasb://checkpoint2@sgrewasparkcheckpoint.blob.core.windows.net/")

    val eventHubDStream = EventHubUtils.createStream(ssc, eventHubParams, partitionCount = 20)
      .map[String] { eventData => new String(eventData.getBody) }

    val words = eventHubDStream.flatMap(status => status.split(" |\"|\n"))
    val hashTags = words.filter(word => word.startsWith("#"))
    val counts = hashTags.map(tag => (tag, 1))
        .reduceByKeyAndWindow(_ + _, _ - _, Seconds(60 * 5), Seconds(1))
    val sortedCounts = counts.map { case(tag, count) => (count, tag)}
        .transform(rdd => rdd.sortByKey(false))
    sortedCounts.foreachRDD(rdd =>
      println("\nTop 10 hashtags:\n" + rdd.take(10).mkString("\n")))

    ssc.start()
    ssc.awaitTermination()
  }
}
