/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import org.apache.spark.Partition

/** An identifier for a event hubs partition in an RDD */
private[eventhubs]
class EventHubRDDPartition(
    val index: Int,
    val partitionId: String,
    val fromOffset: Long,
    val untilOffset: Long
) extends Partition {
  def count: Long = untilOffset - fromOffset
}
