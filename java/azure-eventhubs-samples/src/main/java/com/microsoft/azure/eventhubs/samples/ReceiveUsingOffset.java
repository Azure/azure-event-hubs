/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.eventhubs.samples;

import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Collection;
import java.util.concurrent.ExecutionException;
import java.util.logging.*;

import com.microsoft.azure.eventhubs.*;
import com.microsoft.azure.servicebus.*;

public class ReceiveUsingOffset
{
	public static void main(String[] args) 
			throws ServiceBusException, ExecutionException, InterruptedException, IOException
	{
		final String namespaceName = "----ServiceBusNamespaceName-----";
		final String eventHubName = "----EventHubName-----";
		final String sasKeyName = "-----SharedAccessSignatureKeyName-----";
		final String sasKey = "---SharedAccessSignatureKey----";
		ConnectionStringBuilder connStr = new ConnectionStringBuilder(namespaceName, eventHubName, sasKeyName, sasKey);
		
		EventHubClient ehClient = EventHubClient.createFromConnectionString(connStr.toString()).get();
		
		// receiver
		String partitionId = "0"; // API to get PartitionIds will be released as part of V0.2
		PartitionReceiver receiver = ehClient.createEpochReceiver(
				EventHubClient.DEFAULT_CONSUMER_GROUP_NAME, 
				partitionId, 
				PartitionReceiver.START_OF_STREAM,
				false,
				1).get();
		
		try
		{
			Iterable<EventData> receivedEvents = receiver.receive(100).get();
			
			while (true)
			{
				int batchSize = 0;
				if (receivedEvents != null)
				{
					for(EventData receivedEvent: receivedEvents)
					{
						System.out.println(String.format("Message Payload: %s", new String(receivedEvent.getBody(), Charset.defaultCharset())));
						System.out.println(String.format("Offset: %s, SeqNo: %s, EnqueueTime: %s", 
								receivedEvent.getSystemProperties().getOffset(), 
								receivedEvent.getSystemProperties().getSequenceNumber(), 
								receivedEvent.getSystemProperties().getEnqueuedTime()));
						batchSize++;
					}
				}
				
				System.out.println(String.format("ReceivedBatch Size: %s", batchSize));
				receivedEvents = receiver.receive(100).get();
			}
		}
		finally
		{
			// this is paramount; max number of concurrent receiver per consumergroup per partition is 5
			receiver.close().get();
		}
	}

	/**
	 * actual application-payload, ex: a telemetry event
	 */
	static final class PayloadEvent
	{
		PayloadEvent()	{}
		
		public String strProperty;
		public long longProperty;
		public int intProperty;
	}

}
