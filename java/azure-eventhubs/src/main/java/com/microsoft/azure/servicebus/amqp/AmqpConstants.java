/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.servicebus.amqp;

import org.apache.qpid.proton.amqp.*;

public final class AmqpConstants
{
	private AmqpConstants() { }

	public static final String APACHE = "apache.org";
	public static final String VENDOR = "com.microsoft";

	public static final String AMQP_ANNOTATION_FORMAT = "amqp.annotation.%s >%s '%s'";
	public static final String OFFSET_ANNOTATION_NAME = "x-opt-offset";
	public static final String ENQUEUED_TIME_UTC_ANNOTATION_NAME = "x-opt-enqueued-time";
	public static final String PARTITION_KEY_ANNOTATION_NAME = "x-opt-partition-key";
	public static final String SEQUENCE_NUMBER_ANNOTATION_NAME = "x-opt-sequence-number";

	public static final Symbol PARTITION_KEY = Symbol.getSymbol(PARTITION_KEY_ANNOTATION_NAME);
	public static final Symbol OFFSET = Symbol.getSymbol(OFFSET_ANNOTATION_NAME);
	public static final Symbol SEQUENCE_NUMBER = Symbol.getSymbol(SEQUENCE_NUMBER_ANNOTATION_NAME);
	public static final Symbol ENQUEUED_TIME_UTC = Symbol.getSymbol("x-opt-enqueued-time");

	public static final Symbol STRING_FILTER = Symbol.valueOf(APACHE + ":selector-filter:string");
	public static final Symbol EPOCH = Symbol.valueOf(VENDOR + ":epoch");

	public static final int AMQP_BATCH_MESSAGE_FORMAT = 0x80013700; // 2147563264L;

	public static final int MAX_FRAME_SIZE = 65536;
	
	public static final String AMQP_PROPERTY_MESSAGE_ID = "message-id";
	public static final String AMQP_PROPERTY_USER_ID = "user-id";
	public static final String AMQP_PROPERTY_TO = "to";
	public static final String AMQP_PROPERTY_SUBJECT = "subject";
	public static final String AMQP_PROPERTY_REPLY_TO = "reply-to";
	public static final String AMQP_PROPERTY_CORRELATION_ID = "correlation-id";
	public static final String AMQP_PROPERTY_CONTENT_TYPE = "content-type";
	public static final String AMQP_PROPERTY_CONTENT_ENCODING = "content-encoding";
	public static final String AMQP_PROPERTY_ABSOLUTE_EXPRITY_time = "absolute-expiry-time";
	public static final String AMQP_PROPERTY_CREATION_TIME = "creation-time";
	public static final String AMQP_PROPERTY_GROUP_ID = "group-id";
	public static final String AMQP_PROPERTY_GROUP_SEQUENCE = "group-sequence";
	public static final String AMQP_PROPERTY_REPLY_TO_GROUP_ID = "reply-to-group-id";
}
