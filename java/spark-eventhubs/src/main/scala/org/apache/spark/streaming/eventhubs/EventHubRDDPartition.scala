/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

import org.apache.spark.Partition

private[eventhub]
class EventHubRDDPartition(
    val index: Int,
    val partitionId: String,
    val fromOffset: Long,
    val untilOffset: Long
) extends Partition {
  def count: Long = untilOffset - fromOffset
}
