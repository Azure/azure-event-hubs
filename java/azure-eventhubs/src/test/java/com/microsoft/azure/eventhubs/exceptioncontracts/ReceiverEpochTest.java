package com.microsoft.azure.eventhubs.exceptioncontracts;

import java.util.concurrent.ExecutionException;

import org.junit.Assert;
import org.junit.Assume;
import org.junit.Test;

import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.PartitionReceiveHandler;
import com.microsoft.azure.eventhubs.PartitionReceiver;
import com.microsoft.azure.eventhubs.lib.TestBase;
import com.microsoft.azure.eventhubs.lib.TestEventHubInfo;
import com.microsoft.azure.servicebus.ConnectionStringBuilder;
import com.microsoft.azure.servicebus.ReceiverDisconnectedException;

public class ReceiverEpochTest extends TestBase
{

	@Test (expected = ReceiverDisconnectedException.class)
	public void testEpochReceiver() throws Throwable
	{
		Assume.assumeTrue(TestBase.isTestConfigurationSet());

		TestEventHubInfo eventHubInfo = TestBase.checkoutTestEventHub();
		try 
		{
			ConnectionStringBuilder connectionString = TestBase.getConnectionString(eventHubInfo);
			EventHubClient ehClient = EventHubClient.createFromConnectionString(connectionString.toString()).get();		

			try
			{
				String cgName = eventHubInfo.getRandomConsumerGroup();
				String partitionId = "0";
				long epoch = 345632;
				PartitionReceiver receiver = ehClient.createEpochReceiver(cgName, partitionId, PartitionReceiver.START_OF_STREAM, false, epoch).get();
				EventCounter counter = new EventCounter();
				receiver.setReceiveHandler(counter);

				try
				{
					ehClient.createEpochReceiver(cgName, partitionId, PartitionReceiver.START_OF_STREAM, false, epoch - 10).get();
				}
				catch(ExecutionException exp)
				{
					throw exp.getCause();
				}

				Assert.assertTrue(counter.count > 0);
			}
			finally
			{
				ehClient.close();
			}
		}
		finally
		{
			TestBase.checkinTestEventHub(eventHubInfo.getName());
		}
	}

	public static final class EventCounter extends PartitionReceiveHandler
	{
		public long count;

		public EventCounter()
		{
			super(50);
			count = 0;
		}

		@Override
		public void onError(Throwable error)
		{			
		}

		@Override
		public void onReceive(Iterable<EventData> events)
		{
			count++;
		}

	}
}
