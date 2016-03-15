package com.microsoft.azure.eventhubs.samples;


import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventprocessorhost.*;
import com.microsoft.azure.storage.StorageException;

import java.io.IOException;
import java.net.URISyntaxException;
import java.security.InvalidKeyException;
import java.util.ArrayList;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

public class EventProcessorSample {
    public static void main(String args[])
    {
    	final int hostCount = 1;
    	final int partitionCount = 8;
    	EventProcessorHost.setDummyPartitionCount(partitionCount);
    	
    	if (true)
    	{
	    	//InMemoryCheckpointLeaseManager mgr = new InMemoryCheckpointLeaseManager();
    		AzureStorageCheckpointLeaseManager mgr = new AzureStorageCheckpointLeaseManager("storage connection string");
	    	EventProcessorHost blah = new EventProcessorHost("namespace", "eventhub", "keyname", "key", "$Default", mgr, mgr);
	    	try
	    	{
				mgr.initialize(blah);
			}
	    	catch (Exception e)
	    	{
	    		System.out.println("Initialize failed " + e.toString());
	    		e.printStackTrace();
			}
	    	
	    	basicLeaseManagerTest(mgr, partitionCount);
    	}
    	else if (false)
    	{
    		EventProcessorHost[] hosts = new EventProcessorHost[hostCount];
    		for (int i = 0; i < hostCount; i++)
    		{
    			hosts[i] = new EventProcessorHost("namespace", "eventhub", "keyname", "key", "$Default", "storage connection string");
    		}
    		processMessages(hosts);
    	}
    	
        System.out.println("Exiting");
    }
    
    private static void processMessages(EventProcessorHost[] hosts)
    {
    	int hostCount = hosts.length;
    	
    	for (int i = 0; i < hostCount; i++)
    	{
    		System.out.println("Registering host " + i + " named " + hosts[i].getHostName());
    		hosts[i].registerEventProcessor(EventProcessor.class);
    		try
    		{
    			Thread.sleep(3000);
    		}
    		catch (InterruptedException e1)
    		{
    			// Watch me not care
    		}
    	}

        System.out.println("Press enter to stop");
        try
        {
            System.in.read();
            for (int i = 0; i < hostCount; i++)
            {
	            System.out.println("Calling unregister " + i);
	            Future<?> blah = hosts[i].unregisterEventProcessor();
	            System.out.println("Waiting for Future to complete");
	            blah.get();
	            System.out.println("Completed");
            }
        }
        catch(Exception e)
        {
            System.out.println(e.toString());
            e.printStackTrace();
        }
    }
    
    private static void basicLeaseManagerTest(ILeaseManager mgr, int partitionCount)
    {
    	try
    	{
        	System.out.println("Store should not exist");
			Boolean boolret = mgr.leaseStoreExists().get();
			System.out.println("getStoreExists() returned " + boolret);
			
			System.out.println("Creating store");
			boolret = mgr.createLeaseStoreIfNotExists().get();
			System.out.println("createStoreIfNotExists() returned " + boolret);

        	System.out.println("Store should exist now");
			boolret = mgr.leaseStoreExists().get();
			System.out.println("getStoreExists() returned " + boolret);
			
			Lease[] leases = new Lease[partitionCount];
			for (Integer i = 0; i < partitionCount; i++)
			{
				System.out.print("Creating lease for partition " + i + "... ");
				Lease createdLease = mgr.createLeaseIfNotExists(i.toString()).get();
				leases[i] = createdLease;
				System.out.println("OK");
			}
			
			for (int i = 0; i < partitionCount; i++)
			{
				System.out.println("Partition " + i + " state before: " + ((AzureBlobLease)leases[i]).getState());
				System.out.print("Acquiring lease for partition " + i + "... ");
				boolret = mgr.acquireLease(leases[i]).get();
				System.out.println(boolret.toString());
				System.out.println("Partition " + i + " state after: " + ((AzureBlobLease)leases[i]).getState());
			}
			
			System.out.print("Sleeping... ");
			Thread.sleep(5000);
			System.out.println("done");
			
			for (int i = 0; i < partitionCount; i++)
			{
				System.out.println("Partition " + i + " state before: " + ((AzureBlobLease)leases[i]).getState());
				System.out.print("Renewing lease for partition " + i + "... ");
				boolret = mgr.renewLease(leases[i]).get();
				System.out.println(boolret.toString());
				System.out.println("Partition " + i + " state after: " + ((AzureBlobLease)leases[i]).getState());
			}
			
			System.out.println("Waiting for lease on 0 to expire.");
			int x = 1;
			while (!leases[0].isExpired())
			{
				Thread.sleep(5000);
				System.out.println("Still waiting for lease on 0 to expire: " + (5 * x++));
				for (int i = 1; i < partitionCount; i++)
				{
					System.out.print("   Renewing lease for partition " + i + "... ");
					boolret = mgr.renewLease(leases[i]).get();
					System.out.println(boolret.toString());
				}
			}
			System.out.println("Expired!");
			
			for (int i = 0; i < partitionCount; i++)
			{
				System.out.println("Partition " + i + " state before: " + ((AzureBlobLease)leases[i]).getState());
				System.out.print("Releasing lease for partition " + i + "... ");
				try
				{
					boolret = mgr.releaseLease(leases[i]).get();
					System.out.println(boolret.toString());
				}
				catch (ExecutionException lle)
				{
					if (lle.getCause() instanceof LeaseLostException)
					{
						System.out.println("caught LeaseLostException");
					}
					else
					{
						throw lle;
					}
				}
				System.out.println("Partition " + i + " state after: " + ((AzureBlobLease)leases[i]).getState());
			}
    	}
    	catch (Exception e)
    	{
        	System.out.println("Caught " + e.toString());
        	StackTraceElement[] stack = e.getStackTrace();
        	for (int i = 0; i < stack.length; i++)
        	{
        		System.out.println(stack[i].toString());
        	}
		}
    }
    
    public class SyntheticPump extends PartitionPump
    {
    	Future<Void> producer = null;
    	boolean keepGoing = true;
    	
		@Override
		public void specializedStartPump()
		{
			this.producer = EventProcessorHost.getExecutorService().submit(() -> produceMessages());
		}

		@Override
		public void specializedShutdown(CloseReason reason)
		{
			this.keepGoing = false;
			try
			{
				this.producer.get();
			}
			catch (InterruptedException | ExecutionException e)
			{
				System.out.println("SyntheticPump shutdown failure" + e.toString());
				e.printStackTrace();
			}
		}
		
		private Void produceMessages()
		{
			ArrayList<EventData> events = new ArrayList<EventData>();
			int eventNumber = 0;
			
			while (this.keepGoing)
			{
				events.clear();
				String eventBody = "Event " + eventNumber + " on partition " + this.lease.getPartitionId();
				EventData event = new EventData(eventBody.getBytes());
				events.add(event);
				onEvents(events);
				
				try
				{
					Thread.sleep(3000);
				}
				catch (InterruptedException e)
				{
					// Watch me not care
				}
			}
			
			return null;
		}
    }

    public static class EventProcessor implements IEventProcessor
    {
        public void onOpen(PartitionContext context) throws Exception
        {
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " is opening");
        }

        public void onClose(PartitionContext context, CloseReason reason) throws Exception
        {
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " is closing for reason " + reason.toString());
        }

        public void onEvents(PartitionContext context, Iterable<EventData> messages) throws Exception
        {
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " got message batch");
            int messageCount = 0;
            for (EventData data : messages)
            {
                System.out.println("SAMPLE: " + new String(data.getBody(), "UTF8"));
                messageCount++;
            }
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " batch size was " + messageCount);
        }
    }
}

