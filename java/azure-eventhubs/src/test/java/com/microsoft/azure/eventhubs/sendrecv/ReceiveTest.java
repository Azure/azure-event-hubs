package com.microsoft.azure.eventhubs.sendrecv;

import java.time.Instant;
import java.util.Iterator;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.PartitionReceiver;
import com.microsoft.azure.eventhubs.lib.ApiTestBase;
import com.microsoft.azure.eventhubs.lib.TestBase;
import com.microsoft.azure.eventhubs.lib.TestEventHubInfo;
import com.microsoft.azure.servicebus.ConnectionStringBuilder;
import com.microsoft.azure.servicebus.ServiceBusException;

public class ReceiveTest extends ApiTestBase
{
	static final TestEventHubInfo eventHubInfo = TestBase.checkoutTestEventHub();
	static final String cgName = eventHubInfo.getRandomConsumerGroup();
	static final String partitionId = "0";
	
	static EventHubClient ehClient;
	
	PartitionReceiver offsetReceiver = null;
	PartitionReceiver datetimeReceiver = null;
	
	@BeforeClass
	public static void initializeEventHub()  throws Exception
	{
		final ConnectionStringBuilder connectionString = TestBase.getConnectionString(eventHubInfo);
		ehClient = EventHubClient.createFromConnectionStringSync(connectionString.toString());
		TestBase.pushEventsToPartition(ehClient, partitionId, 25).get();
	}
	
	@Test()
	public void testReceiverStartOfStreamFilters() throws ServiceBusException
	{
		offsetReceiver = ehClient.createReceiverSync(cgName, partitionId, PartitionReceiver.START_OF_STREAM, false);
		Iterable<EventData> startingEventsUsingOffsetReceiver = offsetReceiver.receiveSync(100);
		
		Assert.assertTrue(startingEventsUsingOffsetReceiver != null && startingEventsUsingOffsetReceiver.iterator().hasNext());
		
		datetimeReceiver = ehClient.createReceiverSync(cgName, partitionId, Instant.EPOCH);
		Iterable<EventData> startingEventsUsingDateTimeReceiver = datetimeReceiver.receiveSync(100);
		
		Assert.assertTrue(startingEventsUsingOffsetReceiver != null && startingEventsUsingDateTimeReceiver.iterator().hasNext());
		
		Iterator<EventData> dateTimeIterator = startingEventsUsingDateTimeReceiver.iterator();
		for(EventData eventDataUsingOffset: startingEventsUsingOffsetReceiver)
		{
			EventData eventDataUsingDateTime = dateTimeIterator.next();
			Assert.assertTrue(
					String.format("START_OF_STREAM offset: %s, EPOCH offset: %s", eventDataUsingOffset.getSystemProperties().getOffset(), eventDataUsingDateTime.getSystemProperties().getOffset()),
					eventDataUsingOffset.getSystemProperties().getOffset().equalsIgnoreCase(eventDataUsingDateTime.getSystemProperties().getOffset()));
			
			if (!dateTimeIterator.hasNext())
				break;
		}
	}
	
	@Test()
	public void testReceiverOffsetInclusiveFilter() throws ServiceBusException
	{
		datetimeReceiver = ehClient.createReceiverSync(cgName, partitionId, Instant.EPOCH);
		Iterable<EventData> events = datetimeReceiver.receiveSync(100);
		
		Assert.assertTrue(events != null && events.iterator().hasNext());
		EventData event = events.iterator().next();
		
		offsetReceiver = ehClient.createReceiverSync(cgName, partitionId, event.getSystemProperties().getOffset(), true);
		EventData eventReturnedByOffsetReceiver = offsetReceiver.receiveSync(10).iterator().next();
		
		Assert.assertTrue(eventReturnedByOffsetReceiver.getSystemProperties().getOffset().equals(event.getSystemProperties().getOffset()));
		Assert.assertTrue(eventReturnedByOffsetReceiver.getSystemProperties().getSequenceNumber() == event.getSystemProperties().getSequenceNumber());
	}
	
	@Test()
	public void testReceiverOffsetNonInclusiveFilter() throws ServiceBusException
	{
		datetimeReceiver = ehClient.createReceiverSync(cgName, partitionId, Instant.EPOCH);
		Iterable<EventData> events = datetimeReceiver.receiveSync(100);
		
		Assert.assertTrue(events != null && events.iterator().hasNext());
		
		EventData event = events.iterator().next();
		offsetReceiver = ehClient.createReceiverSync(cgName, partitionId, event.getSystemProperties().getOffset(), false);
		EventData eventReturnedByOffsetReceiver= offsetReceiver.receiveSync(10).iterator().next();
		
		Assert.assertTrue(eventReturnedByOffsetReceiver.getSystemProperties().getSequenceNumber() == event.getSystemProperties().getSequenceNumber() + 1);
	}
	
	@After
	public void testCleanup() throws ServiceBusException
	{
		if (offsetReceiver != null)
		{
			offsetReceiver.closeSync();
			offsetReceiver = null;
		}
		
		if (datetimeReceiver != null)
		{
			datetimeReceiver.closeSync();
			datetimeReceiver = null;
		}
	}
	
	@AfterClass()
	public static void cleanup() throws ServiceBusException
	{
		TestBase.checkinTestEventHub(eventHubInfo.getName());

		if (ehClient != null)
		{
			ehClient.closeSync();
		}
	}
}
