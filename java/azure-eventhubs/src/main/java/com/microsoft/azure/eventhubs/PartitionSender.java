package com.microsoft.azure.eventhubs;

import java.util.*;
import java.util.concurrent.*;
import java.util.function.*;

import com.microsoft.azure.servicebus.*;

// TODO: Implement Timeout on Send operation
public final class PartitionSender
{
	private final String partitionId;
	private final String eventHubName;
	private final MessagingFactory factory;
	
	private MessageSender internalSender;
		
	private PartitionSender(MessagingFactory factory, String eventHubName, String partitionId)
	{
		this.partitionId = partitionId;
		this.eventHubName = eventHubName;
		this.factory = factory;
	}
	
	/**
	 * Internal-Only: factory pattern to Create PartitionSender
	 */
	static CompletableFuture<PartitionSender> Create(MessagingFactory factory, String eventHubName, String partitionId) throws ServiceBusException
	{
		final PartitionSender sender = new PartitionSender(factory, eventHubName, partitionId);
		return sender.createInternalSender()
				.thenApplyAsync(new Function<Void, PartitionSender>()
				{
					public PartitionSender apply(Void a)
					{
						return sender;
					}
				});
	}
	
	private CompletableFuture<Void> createInternalSender() throws ServiceBusException
	{
		return MessageSender.Create(this.factory, UUID.randomUUID().toString(), 
				String.format("%s/Partitions/%s", this.eventHubName, this.partitionId))
				.thenAcceptAsync(new Consumer<MessageSender>()
				{
					public void accept(MessageSender a) { PartitionSender.this.internalSender = a;}
				});
	}

	public final CompletableFuture<Void> send(EventData data) 
			throws ServiceBusException
	{
		return this.internalSender.send(data.toAmqpMessage());
	}
	
	public final CompletableFuture<Void> send(Iterable<EventData> eventDatas) 
			throws ServiceBusException
	{
		if (eventDatas == null || IteratorUtil.sizeEquals(eventDatas.iterator(), 0))
		{
			throw new IllegalArgumentException("EventData batch cannot be empty.");
		}
		
		return this.internalSender.send(EventDataUtil.toAmqpMessages(eventDatas), null);
	}
}
