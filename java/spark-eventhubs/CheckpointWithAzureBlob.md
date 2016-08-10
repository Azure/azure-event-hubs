# Using Azure Blob for Checkpointing

Azure Blob is compatible with the Hadoop API used to checkpoint data in both Apache Spark and the EventHubs+Spark adapter. The first step is to add the hadoop-azure.jar to your project. If you're using Maven, it can be done like so:
```
    <dependency>
      <groupId>org.apache.hadoop</groupId>
      <artifactId>hadoop-azure</artifactId>
      <version>2.7.2</version>
    </dependency>
```

The second step is to configure hadoop properly in your application code.
```scala
    val config: Configuration = ssc.sparkContext.hadoopConfiguration
    config.set("fs.azure", "org.apache.hadoop.fs.azure.NativeAzureFileSystem")
    config.set("fs.azure.account.key.yourStorageAccountName.blob.core.windows.net", accountKey)
```

After that, simply use the wasb-schemed directory corresponding to your Azure Blob, and you're all set! The format of the directory is:
```
wasb[s]://<containername>@<accountname>.blob.core.windows.net/<path>
```
