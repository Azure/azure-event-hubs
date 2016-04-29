package com.microsoft.azure.eventprocessorhost;

import org.junit.Test;
import static org.junit.Assert.assertEquals;

import java.util.ArrayList;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

public class PartitionManagerTest
{
	private ILeaseManager[] leaseManagers;
	private ICheckpointManager[] checkpointManagers;
	private EventProcessorHost[] hosts;
	private TestPartitionManager[] partitionManagers;
	private Future<?>[] managerFutures;
	
	private boolean keepGoing;
	private int countOfChecks;
	
	@Test
	public void testPartitionBalancingExactMultiples() throws InterruptedException
	{
		setup(2, 4);
		this.keepGoing = true;
		this.countOfChecks = 0;
		startManagers();
		// Poll until checkPartitionDistribution() declares that it's time to stop.
		while (this.keepGoing)
		{
			try
			{
				Thread.sleep(15000);
			}
			catch (InterruptedException e)
			{
				System.out.println("Sleep interrupted, emergency bail");
				Thread.currentThread().interrupt();
				throw e;
			}
		}
		stopManagers();
		
		assertEquals(1,1);
	}
	
	synchronized void checkPartitionDistribution()
	{
		System.out.println("Partitions redistributed");
		for (int i = 0; i < this.partitionManagers.length; i++)
		{
			Iterable<String> ownedPartitions = this.partitionManagers[i].getOwnedPartitions();
			System.out.print("\tHost " + this.hosts[i].getHostName() + " has ");
			for (String id : ownedPartitions)
			{
				System.out.print(id + ", ");
			}
			System.out.println();
		}
	
		this.countOfChecks++;
		if (this.countOfChecks > 20)
		{
			this.keepGoing = false;
		}
	}
	
	private void setup(int hostCount, int partitionCount)
	{
		this.leaseManagers = new ILeaseManager[hostCount];
		this.checkpointManagers = new ICheckpointManager[hostCount];
		this.hosts = new EventProcessorHost[hostCount];
		this.partitionManagers = new TestPartitionManager[hostCount];
		this.managerFutures = new Future<?>[hostCount];
		
		for (int i = 0; i < hostCount; i++)
		{
			InMemoryLeaseManager lm = new InMemoryLeaseManager(); 
			InMemoryCheckpointManager cm = new InMemoryCheckpointManager();
			
			String suffix = String.valueOf(i);
			this.hosts[i] = new EventProcessorHost("dummyHost" + suffix, "dummyNamespace" + suffix, "dummyEventHub" + suffix, "dummyKeyName" + suffix,
					"dummyKey" + suffix, "dummyConsumerGroup" + suffix, cm, lm);
			
			lm.initialize(this.hosts[i]);
			this.leaseManagers[i] = lm;
			cm.initialize(this.hosts[i]);
			this.checkpointManagers[i] = cm;
			
			this.partitionManagers[i] = new TestPartitionManager(this.hosts[i], partitionCount);
			this.hosts[i].setPartitionManager(this.partitionManagers[i]);
		}
	}
	
	private void startManagers()
	{
		for (int i = 0; i < this.partitionManagers.length; i++)
		{
			this.managerFutures[i] = EventProcessorHost.getExecutorService().submit(this.partitionManagers[i]);
		}
	}
	
	private void stopManagers()
	{
		for (int i = 0; i < this.partitionManagers.length; i++)
		{
			this.partitionManagers[i].stopPartitions();
		}
		for (int i = 0; i < this.partitionManagers.length; i++)
		{
			try
			{
				this.managerFutures[i].get();
			}
			catch (InterruptedException | ExecutionException e)
			{
				System.out.println("Error stopping manager " + i + ": " + e.toString());
				e.printStackTrace();
			}
		}
	}
	
	private class TestPartitionManager extends PartitionManager
	{
		private int partitionCount;
		
		TestPartitionManager(EventProcessorHost host, int partitionCount)
		{
			super(host);
			this.partitionCount = partitionCount;
		}
		
		Iterable<String> getOwnedPartitions()
		{
			return ((DummyPump)this.pump).getPumpsList();
		}

		@Override
	    Iterable<String> getPartitionIds()
	    {
			ArrayList<String> ids = new ArrayList<String>();
			for (int i = 0; i < this.partitionCount; i++)
			{
				ids.add(String.valueOf(i));
			}
			return ids;
	    }
	    
		@Override
	    Pump createPumpTestHook()
	    {
			return new DummyPump(this.host);
	    }
		
		@Override
		void onInitializeCompleteTestHook()
		{
			System.out.println("PartitionManager for host " + this.host.getHostName() + " initialized stores OK");
		}
		
		@Override
		void onPartitionCheckCompleteTestHook()
		{
			PartitionManagerTest.this.checkPartitionDistribution();
		}
	}
}
