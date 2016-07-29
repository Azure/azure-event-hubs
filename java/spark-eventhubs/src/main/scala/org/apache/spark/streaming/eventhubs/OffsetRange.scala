/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import scala.collection.mutable.ArrayBuffer

/**
  * Contains starting offset and batch size for EventHubRDD
  */
final class OffsetRange private(
    val partitionId: String,
    val startingOffset: Long,
    val batchSize: Long) extends Serializable {
  import OffsetRange.OffsetRangeTuple

  var fromOffset: Long = startingOffset
  var untilOffset: Long = fromOffset + batchSize

  def count = batchSize

  override def equals(obj: Any): Boolean = obj match {
    case that: OffsetRange =>
      this.partitionId == that.partitionId &&
      this.batchSize == that.batchSize
    case _ => false
  }

  override def hashCode(): Int = {
    toTuple.hashCode()
  }

  override def toString: String =
    s"OffsetRange(partitionId: $partitionId, batchSize: $batchSize)"

  def set(startingOffset: Long, batchSize: Long): Unit = {
    fromOffset = startingOffset
    untilOffset = fromOffset + batchSize
  }

  private[streaming]
  def toTuple: OffsetRangeTuple = (partitionId, startingOffset, batchSize)
}

object OffsetRange {
  def createArray(numPartitions: Int, startingOffset: Long, batchSize: Long): Array[OffsetRange] = {
    val res = ArrayBuffer[OffsetRange]()
    for (i <- 0 until numPartitions)
      res += OffsetRange(i.toString, startingOffset, batchSize)
    res.toArray
  }

  def create(partitionId: String, startingOffset: Long, batchSize: Long): OffsetRange =
    new OffsetRange(partitionId, startingOffset, batchSize)

  def apply(partitionId: String, startingOffset: Long, batchSize: Long): OffsetRange =
    new OffsetRange(partitionId, startingOffset, batchSize)

  private[eventhubs]
  type OffsetRangeTuple = (String, Long, Long)

  private[eventhubs]
  def apply(t: OffsetRangeTuple) =
    new OffsetRange(t._1, t._2, t._3)
}
