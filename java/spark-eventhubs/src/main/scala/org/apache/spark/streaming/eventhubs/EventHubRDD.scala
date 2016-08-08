/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import com.microsoft.azure.eventhubs.EventData
import org.apache.spark.partial.{BoundedDouble, PartialResult}
import org.apache.spark.rdd.RDD
import org.apache.spark.util.NextIterator
import org.apache.spark._

import scala.collection.JavaConverters._
import scala.collection.mutable.ArrayBuffer

/**
  * Enables batch processing with Event Hubs
  *
  * @param eventHubParams Requires "namespaceName", "eventHubName", "sasKeyName", "sasKey".
  * @param offsetRange defines what data is to be consumed from EventHubs
  * @param offsetRanges defines what data is to be consumed from EventHubs
  */
private[eventhubs]
class EventHubRDD private[spark] (
    sc: SparkContext,
    eventHubParams: Map[String,String],
    offsetRanges: Option[Array[OffsetRange]] = None,
    offsetRange: Option[OffsetRange] = None,
    client: EventHubInstance
) extends RDD[EventData](sc, Nil) with Logging {

  override def getPartitions: Array[Partition] = {
    if (offsetRanges.isDefined) {
      offsetRanges.get.zipWithIndex.map { case (o, i) =>
        new EventHubRDDPartition(i, o.partitionId, o.fromOffset, o.untilOffset)
      }.toArray
    } else if (offsetRange.isDefined) {
      val o = offsetRange.get
      Array[Partition](new EventHubRDDPartition(0, o.partitionId, o.fromOffset, o.untilOffset))
    } else {
      throw new SparkException("No offset range(s) found in getPartitions")
    }
  }

  override def count: Long = {
    if (offsetRanges.isDefined)
      offsetRanges.get.map(_.count).sum
    else
      offsetRange.get.count
  }

  override def countApprox(
    timeout: Long,
    confidence: Double = 0.95
  ): PartialResult[BoundedDouble] = {
    val c = count
    new PartialResult(new BoundedDouble(c, 1.0, c, c), true)
  }

  override def isEmpty(): Boolean = count == 0L

  override def take(num: Int): Array[EventData] = {
    val nonEmptyPartitions = this.partitions
      .map(_.asInstanceOf[EventHubRDDPartition])
      .filter(_.count > 0)

    if (num < 1 || nonEmptyPartitions.isEmpty) {
      return new Array[EventData](0)
    }

    val parts = nonEmptyPartitions.foldLeft(Map[Int,Int]()) { (result, part) =>
      val remain = num - result.values.sum
      if (remain > 0) {
        val taken = Math.min(remain,part.count)
        result + (part.index -> taken.toInt)
      } else {
        result
      }
    }

    val buf = new ArrayBuffer[EventData]
    val res = context.runJob(
      this,
      (tc: TaskContext, it: Iterator[EventData]) => it.take(parts(tc.partitionId)).toArray,
      parts.keys.toArray)
    res.foreach(buf ++= _)
    buf.toArray
  }

  //TODO
  override def getPreferredLocations(thePart: Partition): Seq[String] = {
    //val part = thePart.asInstanceOf[EventHubRDDPartition]
    Seq[String]()
  }

  private def errBeginAfterEnd(part: EventHubRDDPartition): String =
    s"Beginning offset ${part.fromOffset} is after the ending offset ${part.untilOffset} " +
      s"for partitionId ${part.partitionId}. " +
      "You either provided an invalid fromOffset, or their is an issue with EventHubs"

  private def errRanOutBeforeEnd(part: EventHubRDDPartition): String =
    s"Ran out of messages before reaching ending offsdet ${part.untilOffset} " +
      s"for partitionId ${part.partitionId} start ${part.fromOffset}." +
      " This should not happen, and indicates a message may have been skipped"

  private def errOvershotEnd(itemOffset: Long, part: EventHubRDDPartition): String =
    s"Got $itemOffset > ending offset ${part.untilOffset} " +
      s"for partitionId ${part.partitionId} start ${part.fromOffset}." +
      " This should not happen, and indicates a message may have been skipped"

  override def compute(thePart: Partition, context: TaskContext): Iterator[EventData] = {
    val part = thePart.asInstanceOf[EventHubRDDPartition]
    assert(part.fromOffset <= part.untilOffset, errBeginAfterEnd(part))
    if (part.fromOffset == part.untilOffset) {
      log.info(s"Beginning offset ${part.fromOffset} is the same as ending offset " +
        s"skipping partitionId ${part.partitionId}")
      Iterator.empty
    } else {
      new EventHubsRDDIterator(part, context)
    }
  }

  /**
    * The EventHubsRDDIterator fetches data directly from the given Event Hubs partition
    */
  private class EventHubsRDDIterator(
      part: EventHubRDDPartition,
      context: TaskContext
  ) extends NextIterator[EventData] {

    context.addTaskCompletionListener{ context => closeIfNeeded() }

    log.info(s"Computing partitionId ${part.partitionId} " +
      s"offsets ${part.fromOffset} -> ${part.untilOffset}")

    client.createReceiver(eventHubParams, part.partitionId, part.fromOffset.toString)
    var requestOffset = 0
    var iterator: Iterator[EventData] = null

    override def close(): Unit = {
      client.close
    }

    private def fetchBatch: Iterator[EventData] = {
      val batchSize = part.count.toInt
      var batch: Iterable[EventData] = client.receive(batchSize).asScala
      // fetch data until the batch size is reached
      while (batch.size < batchSize) {
        val additionalBatch = client.receive(batchSize - batch.size)
        if (additionalBatch != null)
          batch = Iterable.concat(batch, additionalBatch.asScala)
      }
      batch.iterator
    }

    override def getNext(): EventData = {
      if (iterator == null) {
        iterator = fetchBatch
      }
      if (!iterator.hasNext) {
        assert(requestOffset == part.count, errRanOutBeforeEnd(part))
        finished = true
        null.asInstanceOf[EventData]
      } else {
        val item = iterator.next
        if (item.getSystemProperties.getSequenceNumber >= part.untilOffset) {
          assert(item.getSystemProperties.getSequenceNumber == part.untilOffset,
            errOvershotEnd(item.getSystemProperties.getSequenceNumber, part))
          finished = true
          null.asInstanceOf[EventData]
        } else {
          requestOffset += 1
          item.asInstanceOf[EventData]
        }
      }
    }
  }
}
