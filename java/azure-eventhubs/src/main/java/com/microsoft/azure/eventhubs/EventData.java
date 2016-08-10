/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.eventhubs;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.nio.ByteBuffer;
import java.time.Instant;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

import org.apache.qpid.proton.Proton;
import org.apache.qpid.proton.amqp.Binary;
import org.apache.qpid.proton.amqp.Symbol;
import org.apache.qpid.proton.amqp.messaging.ApplicationProperties;
import org.apache.qpid.proton.amqp.messaging.Data;
import org.apache.qpid.proton.amqp.messaging.MessageAnnotations;
import org.apache.qpid.proton.message.Message;

import com.microsoft.azure.servicebus.amqp.AmqpConstants;

/**
 * The data structure encapsulating the Event being sent-to and received-from EventHubs.
 * Each EventHubs partition can be visualized as a Stream of {@link EventData}.
 */
public class EventData implements Serializable
{
	private static final long serialVersionUID = -5631628195600014255L;

	transient private Binary bodyData;
	
	private String partitionKey;
	private String offset;
	private long sequenceNumber;
	private Instant enqueuedTime;
	private boolean isReceivedEvent;
	private Map<String, String> properties;

	private SystemProperties systemProperties;

	private EventData()
	{
	}

	/**
	 * Internal Constructor - intended to be used only by the {@link PartitionReceiver} to Create #EventData out of #Message
	 */
	@SuppressWarnings("unchecked")
	EventData(Message amqpMessage)
	{
		if (amqpMessage == null)
		{
			throw new IllegalArgumentException("amqpMessage cannot be null");
		}

		Map<Symbol, Object> messageAnnotations = amqpMessage.getMessageAnnotations().getValue();

		Object partitionKeyObj = messageAnnotations.get(AmqpConstants.PARTITION_KEY);
		if (partitionKeyObj != null)
		{
			this.partitionKey = partitionKeyObj.toString();
			messageAnnotations.remove(AmqpConstants.PARTITION_KEY);
		}

		Object sequenceNumberObj = messageAnnotations.get(AmqpConstants.SEQUENCE_NUMBER);
		this.sequenceNumber = (Long) sequenceNumberObj;
		messageAnnotations.remove(AmqpConstants.SEQUENCE_NUMBER);

		Object enqueuedTimeUtcObj = messageAnnotations.get(AmqpConstants.ENQUEUED_TIME_UTC);
		this.enqueuedTime = ((Date) enqueuedTimeUtcObj).toInstant();
		messageAnnotations.remove(AmqpConstants.ENQUEUED_TIME_UTC);

		this.offset = messageAnnotations.get(AmqpConstants.OFFSET).toString();
		messageAnnotations.remove(AmqpConstants.OFFSET);

		this.properties = amqpMessage.getApplicationProperties() == null ? null 
				: ((Map<String, String>)(amqpMessage.getApplicationProperties().getValue()));

		if (!messageAnnotations.isEmpty())
		{
			if (this.properties == null)
			{
				this.properties = new HashMap<String, String>();
			}

			for(Map.Entry<Symbol, Object> annotation: messageAnnotations.entrySet())
			{
				this.properties.put(annotation.getKey().toString(), annotation.getValue() != null ? annotation.getValue().toString() : null);
			}
		}

		this.bodyData = amqpMessage.getBody() == null ? null : ((Data) amqpMessage.getBody()).getValue();

		this.isReceivedEvent = true;

		amqpMessage.clear();
	}

	/**
	 * Construct EventData to Send to EventHubs.
	 * Typical pattern to create a Sending EventData is:
	 * <pre>
	 * i.	Serialize the sending ApplicationEvent to be sent to EventHubs into bytes.
	 * ii.	If complex serialization logic is involved (for example: multiple types of data) - add a Hint using the {@link #getProperties()} for the Consumer.
	 * </pre> 
	 * <p> Sample Code:
	 * <pre>
	 * EventData eventData = new EventData(telemetryEventBytes);
	 * HashMap{@literal <}String, String{@literal >} applicationProperties = new HashMap{@literal <}String, String{@literal >}();
	 * applicationProperties.put("eventType", "com.microsoft.azure.monitoring.EtlEvent");
	 * eventData.setProperties(applicationProperties);
	 * partitionSender.Send(eventData);
	 * </pre>
	 * @param data the actual payload of data in bytes to be Sent to EventHubs.
	 * @see EventHubClient#createFromConnectionString(String)
	 */
	public EventData(byte[] data)
	{
		this();

		if (data == null)
		{
			throw new IllegalArgumentException("data cannot be null");
		}

		this.bodyData = new Binary(data);
	}

	/**
	 * Construct EventData to Send to EventHubs.
	 * Typical pattern to create a Sending EventData is:
	 * <pre>
	 * i.	Serialize the sending ApplicationEvent to be sent to EventHubs into bytes.
	 * ii.	If complex serialization logic is involved (for example: multiple types of data) - add a Hint using the {@link #getProperties()} for the Consumer.
	 *  </pre> 
	 *  <p> Illustration:
	 *  <pre> {@code
	 *  EventData eventData = new EventData(telemetryEventBytes, offset, length);
	 *  HashMap{@literal <}String, String{@literal >} applicationProperties = new HashMap{@literal <}String, String{@literal >}();
	 *  applicationProperties.put("eventType", "com.microsoft.azure.monitoring.EtlEvent");
	 *	eventData.setProperties(applicationProperties);
	 *  partitionSender.Send(eventData);
	 *  }</pre>
	 * @param data the byte[] where the payload of the Event to be sent to EventHubs is present
	 * @param offset Offset in the byte[] to read from ; inclusive index
	 * @param length length of the byte[] to be read, starting from offset
	 * @see EventHubClient#createFromConnectionString(String)
	 */
	public EventData(byte[] data, final int offset, final int length)
	{
		this();

		if (data == null)
		{
			throw new IllegalArgumentException("data cannot be null");
		}

		this.bodyData = new Binary(data, offset, length);
	}

	/**
	 * Construct EventData to Send to EventHubs.
	 * Typical pattern to create a Sending EventData is:
	 * <pre>
	 * i.	Serialize the sending ApplicationEvent to be sent to EventHubs into bytes.
	 * ii.	If complex serialization logic is involved (for example: multiple types of data) - add a Hint using the {@link #getProperties()} for the Consumer.
	 *  </pre> 
	 *  <p> Illustration:
	 *  <code>
	 *  	EventData eventData = new EventData(telemetryEventByteBuffer);
	 *  	HashMap{@literal <}String, String{@literal >} applicationProperties = new HashMap{@literal <}String, String{@literal >}();
	 *  	applicationProperties.put("eventType", "com.microsoft.azure.monitoring.EtlEvent");
	 *		eventData.setProperties(applicationProperties);
	 *  	partitionSender.Send(eventData);
	 *  </code>
	 * @param buffer ByteBuffer which references the payload of the Event to be sent to EventHubs
	 * @see EventHubClient#createFromConnectionString(String)
	 */
	public EventData(ByteBuffer buffer)
	{
		this();

		if (buffer == null)
		{
			throw new IllegalArgumentException("data cannot be null");
		}

		this.bodyData = Binary.create(buffer);
	}

	/**
	 * Get Actual Payload/Data wrapped by EventData.
	 * This is intended to be used after receiving EventData using @@PartitionReceiver.
	 * @return returns the byte[] of the actual data 
	 */
	public byte[] getBody()
	{
		// TODO: enforce on-send constructor type 2
		return this.bodyData == null ? null : this.bodyData.getArray();
	}

	/**
	 * Application property bag
	 * @return returns Application properties
	 */
	public Map<String, String> getProperties()
	{
		return this.properties;
	}

	public void setProperties(Map<String, String> applicationProperties)
	{
		this.properties = applicationProperties;
	}

	/**
	 * SystemProperties that are populated by EventHubService.
	 * <p>As these are populated by Service, they are only present on a Received EventData.
	 * @return an encapsulation of all SystemProperties appended by EventHubs service into EventData
	 */
	public SystemProperties getSystemProperties()
	{
		if (this.isReceivedEvent && this.systemProperties == null)
		{
			this.systemProperties = new SystemProperties(this);
		}

		return this.systemProperties;
	}

	Message toAmqpMessage()
	{
		Message amqpMessage = Proton.message();

		if (this.properties != null && !this.properties.isEmpty())
		{
			ApplicationProperties applicationProperties = new ApplicationProperties(this.properties);
			amqpMessage.setApplicationProperties(applicationProperties);
		}

		if (this.bodyData != null)
		{
			amqpMessage.setBody(new Data(this.bodyData));
		}

		return amqpMessage;
	}

	Message toAmqpMessage(String partitionKey)
	{
		Message amqpMessage = this.toAmqpMessage();

		MessageAnnotations messageAnnotations = (amqpMessage.getMessageAnnotations() == null) 
				? new MessageAnnotations(new HashMap<Symbol, Object>()) 
						: amqpMessage.getMessageAnnotations();		
		messageAnnotations.getValue().put(AmqpConstants.PARTITION_KEY, partitionKey);
		amqpMessage.setMessageAnnotations(messageAnnotations);

		return amqpMessage;
	}
	
	private void writeObject(ObjectOutputStream out) throws IOException
	{
		out.defaultWriteObject();
		
		out.writeInt(this.bodyData.getLength());
		out.write(this.bodyData.getArray(), this.bodyData.getArrayOffset(), this.bodyData.getLength());
	}
	
	private void readObject(ObjectInputStream in) throws IOException, ClassNotFoundException
	{
		in.defaultReadObject();
		
		final int length = in.readInt();
		final byte[] data = new byte[length];
		in.read(data, 0, length);
		this.bodyData = new Binary(data, 0, length);
	}

	public static class SystemProperties implements Serializable
	{
		private static final long serialVersionUID = -2827050124966993723L;
		
		private final EventData eventData;
		
		protected SystemProperties()
		{
			this.eventData = null;
		}

		private SystemProperties(final EventData eventData)
		{
			this.eventData = eventData;
		}

		public long getSequenceNumber()
		{
			return this.eventData.sequenceNumber;
		}
		
		public Instant getEnqueuedTime()
		{
			return this.eventData.enqueuedTime;
		}

		public String getOffset()
		{
			return this.eventData.offset;
		}

		public String getPartitionKey()
		{
			return this.eventData.partitionKey;
		}
	}
}
