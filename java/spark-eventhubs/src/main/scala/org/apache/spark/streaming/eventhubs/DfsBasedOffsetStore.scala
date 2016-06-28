/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FileSystem, Path}

@SerialVersionUID(1L)
class DfsBasedOffsetStore(
    directory: String,
    namespaceName: String,
    eventHubName: String,
    partitionId: String) extends OffsetStore {

  var path: Path = null
  var fs: FileSystem = null

  override def open(): Unit = {
    if(fs == null) {
      path = new Path(s"$directory/$namespaceName/$eventHubName/$partitionId")
      fs = path.getFileSystem(new Configuration())
    }
  }

  override def write(offset: String): Unit = {
    val stream = fs.create(path, true)
    stream.writeUTF(offset)
    stream.close()
  }

  override def read(): String = {
    var offset: String = "-1"
    if(fs.exists(path)) {
      val stream = fs.open(path)
      offset = stream.readUTF()
      stream.close()
    }
    offset
  }

  override def close(): Unit = {
    if(fs != null) {
      fs.close()
      fs = null
    }
  }
}
