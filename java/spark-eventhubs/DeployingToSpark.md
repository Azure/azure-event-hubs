# Deploying to Spark
This document has some tips that should help in the process of deploying a Spark app to your cluster. It would be great if were using the EventHubs+Spark adapter, but should be helpful regardless. If you see any issues or have any other tips to add, please contribute! 

### 1. Setup Your Cluster
Checkout SparkClusterSetup.md for a walkthrough on how to do this.

### 2. Write Your App
In order to use EventHubs with Spark and Spark Streaming, you need to use *EventHubUtils.scala* to create DStreams and RDDs. You can use createStream, createPartitionStream, createRDD, or createPartitionRDD - check out the README for more information. For more information on how to process your data using Apache Spark and Spark Streaming, checkout their programming guides. [Here](http://spark.apache.org/docs/latest/streaming-programming-guide.html) is the one for Spark Streaming and [here](http://spark.apache.org/docs/latest/programming-guide.html) is the one for Spark Core. 

### 3. Run Locally
When you run your app locally, you want to set master to "local[X]" where X is greater than the number of partitions in your EventHubs instance. The number of receivers by the adapter will be equal to the number of partitions you have. Here's a great explanation from Apache Spark's website:
```
When running a Spark Streaming program locally, do not use “local” or “local[1]” as the master URL. Either of these means that only one thread will be used for running tasks locally. If you are using an input DStream based on a receiver (e.g. sockets, Kafka, Flume, etc.), then the single thread will be used to run the receiver, leaving no thread for processing the received data. Hence, when running locally, always use “local[n]” as the master URL, where n > number of receivers to run.
```
Check out [Spark Properties](http://spark.apache.org/docs/latest/configuration.html#spark-properties) for more information.

### 4. Deploy
For detail on how to submit your application, checkout Apache Spark's [Submitting Applications](http://spark.apache.org/docs/latest/submitting-applications.html) page. A key thing to note is that the .jar you submit must be publically available - all of the machines in your cluster must be able to access the jar. You can do this in two ways:
1. Copy the jar to every machine in your node, and make sure the share the same path on each machine.
2. Put the jar in HDFS, Azure Blob, or something similar. 

### 5. Tune Your Cluster
Hopefully everything up to this point has been relatively painless thanks to the documentation on here. Tuning your cluster to make sure your Spark applications run quickly is crucial. I've found some resources that helped a lot. First from Apache Spark's website:
* [Cluster Overview](http://spark.apache.org/docs/latest/cluster-overview.html)
* [Tuning Spark](http://spark.apache.org/docs/latest/tuning.html)

And, Cloudera wrote two blog posts about cluster tuning:
* [How To Tune Your Apache Spark Jobs Part 1](http://blog.cloudera.com/blog/2015/03/how-to-tune-your-apache-spark-jobs-part-1/)
* [How To Tune Your Apache Spark Jobs Part 2](http://blog.cloudera.com/blog/2015/03/how-to-tune-your-apache-spark-jobs-part-2/)

Good luck!
