/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package org.apache.spark.streaming.eventhubs

@SerialVersionUID(1L)
trait OffsetStore extends Serializable {
  def open(): Unit
  def write(offset: String): Unit
  def read(): String
  def close(): Unit
}
