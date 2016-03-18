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
    	final int hostCount = 2;
    	final int partitionCount = 8;
    	EventProcessorHost.setDummyPartitionCount(partitionCount);
    	
    	if (false)
    	{
    		ILeaseManager leaseMgr = null;
    		ICheckpointManager checkpointMgr = null;
	    	//leaseMgr = new InMemoryLeaseManager();
    		//checkpointMgr = new InMemoryCheckpointManager();
    		AzureStorageCheckpointLeaseManager mgr = new AzureStorageCheckpointLeaseManager("storage connection string");
    		leaseMgr = mgr;
    		checkpointMgr = mgr;
	    	EventProcessorHost blah = new EventProcessorHost("namespace", "eventhub", "keyname", "key", "$Default", checkpointMgr, leaseMgr);
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
    	if (false)
    	{
	    	AzureStorageCheckpointLeaseManager mgr1 = new AzureStorageCheckpointLeaseManager("storage connection string");
	    	AzureStorageCheckpointLeaseManager mgr2 = new AzureStorageCheckpointLeaseManager("storage connection string");
	    	EventProcessorHost blah1 = new EventProcessorHost("namespace", "eventhub", "keyname", "key", "$Default", mgr1, mgr1);
	    	EventProcessorHost blah2 = new EventProcessorHost("namespace", "eventhub", "keyname", "key", "$Default", mgr2, mgr2);
	    	try
	    	{
				mgr1.initialize(blah1);
				mgr2.initialize(blah2);
			}
	    	catch (Exception e)
	    	{
	    		System.out.println("Initialize failed " + e.toString());
	    		e.printStackTrace();
			}
	    	
	    	stealLeaseTest(mgr1, mgr2);
    	}
    	if (true)
    	{
    		EventProcessorHost[] hosts = new EventProcessorHost[hostCount];
    		for (int i = 0; i < hostCount; i++)
    		{
    			hosts[i] = new EventProcessorHost("namespace", "eventhub", "keyname", "key", "$Default", "storage connection string");
    			//hosts[i].setPumpClass(SyntheticPump.class);
    		}
    		processMessages(hosts);
    	}
    	
        System.out.println("End of sample");
    }
    
    private static void stealLeaseTest(ILeaseManager mgr1, ILeaseManager mgr2)
    {
    	try
    	{
	    	System.out.println("Store may not exist");
			Boolean boolret = mgr1.leaseStoreExists().get();
			System.out.println("getStoreExists() returned " + boolret);
			
			System.out.println("Create store if not exists");
			boolret = mgr1.createLeaseStoreIfNotExists().get();
			System.out.println("createStoreIfNotExists() returned " + boolret);
	
	    	System.out.println("Store should exist now");
			boolret = mgr1.leaseStoreExists().get();
			System.out.println("getStoreExists() returned " + boolret);
			
			System.out.print("Mgr1 making sure lease for 0 exists... ");
			Lease mgr1Lease = mgr1.createLeaseIfNotExists("0").get();
			System.out.println("OK");
			
			System.out.print("Mgr2 get lease... ");
			Lease mgr2Lease = mgr2.getLease("0").get();
			System.out.println("OK");

			System.out.print("Mgr1 acquiring lease... ");
			boolret = mgr1.acquireLease(mgr1Lease).get();
			System.out.println(boolret);
			System.out.println("Lease token is " + mgr1Lease.getToken());
			
			System.out.println("Waiting for lease on 0 to expire.");
			int x = 1;
			while (!mgr1Lease.isExpired())
			{
				Thread.sleep(5000);
				System.out.println("Still waiting for lease on 0 to expire: " + (5 * x++));
			}
			System.out.println("Expired!");

			System.out.print("Mgr2 acquiring lease... ");
			boolret = mgr2.acquireLease(mgr2Lease).get();
			System.out.println(boolret);
			System.out.println("Lease token is " + mgr2Lease.getToken());
			
			System.out.print("Mgr1 tries to renew lease... ");
			boolret = mgr1.renewLease(mgr1Lease).get();
			System.out.println(boolret);
			
			System.out.print("Mgr1 gets current lease data in order to steal it... ");
			mgr1Lease = mgr1.getLease(mgr1Lease.getPartitionId()).get();
			System.out.println("OK");
			
			System.out.print("Mgr1 tries to steal lease... ");
			boolret = mgr1.acquireLease(mgr1Lease).get();
			System.out.println(boolret);
			System.out.println("Lease token is " + mgr1Lease.getToken());
			
			System.out.println("Checkpoint currently at offset: " + mgr1Lease.getCheckpoint().getOffset() + " seqNo: " + mgr1Lease.getCheckpoint().getSequenceNumber());
			mgr1Lease.setOffset(((Integer)(Integer.parseInt(mgr1Lease.getCheckpoint().getOffset()) + 500)).toString());
			mgr1Lease.setSequenceNumber(mgr1Lease.getCheckpoint().getSequenceNumber() + 5);
			System.out.println("Checkpoint changed to offset: " + mgr1Lease.getCheckpoint().getOffset() + " seqNo: " + mgr1Lease.getCheckpoint().getSequenceNumber());
			System.out.print("Mgr1 checkpointing... ");
			((ICheckpointManager)mgr1).updateCheckpoint(mgr1Lease.getCheckpoint()).get();
			System.out.println("done");
			
			System.out.print("Mgr2 gets current lease data in order to steal it... ");
			mgr2Lease = mgr2.getLease(mgr1Lease.getPartitionId()).get();
			System.out.println("OK");
			
			System.out.print("Mgr2 tries to steal lease... ");
			boolret = mgr2.acquireLease(mgr2Lease).get();
			System.out.println(boolret);
			System.out.println("Lease token is " + mgr1Lease.getToken());
			System.out.println("Got checkpoint of offset: " + mgr1Lease.getCheckpoint().getOffset() + " seqNo: " + mgr1Lease.getCheckpoint().getSequenceNumber());
			
			System.out.print("Mgr2 releasing lease... ");
			boolret = mgr2.releaseLease(mgr2Lease).get();
			System.out.println(boolret);

			System.out.print("Mgr1 releasing lease... ");
			boolret = mgr2.releaseLease(mgr1Lease).get();
			System.out.println(boolret);
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
	            hosts[i].unregisterEventProcessor();
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
        	System.out.println("Store may not exist");
			Boolean boolret = mgr.leaseStoreExists().get();
			System.out.println("getStoreExists() returned " + boolret);
			
			System.out.println("Create store if not exists");
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
				System.out.println("Partition " + i + " state before: " + ((AzureBlobLease)leases[i]).getStateDebug());
				System.out.print("Acquiring lease for partition " + i + "... ");
				boolret = mgr.acquireLease(leases[i]).get();
				System.out.println(boolret.toString());
				System.out.println("Partition " + i + " state after: " + ((AzureBlobLease)leases[i]).getStateDebug());
			}
			
			System.out.print("Sleeping... ");
			Thread.sleep(5000);
			System.out.println("done");
			
			for (int i = 0; i < partitionCount; i++)
			{
				System.out.println("Partition " + i + " state before: " + ((AzureBlobLease)leases[i]).getStateDebug());
				System.out.print("Renewing lease for partition " + i + "... ");
				boolret = mgr.renewLease(leases[i]).get();
				System.out.println(boolret.toString());
				System.out.println("Partition " + i + " state after: " + ((AzureBlobLease)leases[i]).getStateDebug());
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
				System.out.println("Partition " + i + " state before: " + ((AzureBlobLease)leases[i]).getStateDebug());
				System.out.print("Releasing lease for partition " + i + "... ");
				boolret = mgr.releaseLease(leases[i]).get();
				System.out.println(boolret.toString());
				System.out.println("Partition " + i + " state after: " + ((AzureBlobLease)leases[i]).getStateDebug());
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
    
    public static class SyntheticPump extends PartitionPump
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
				eventNumber++;
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
            String hostname = context.getLease().getOwner();
        	System.out.println("SAMPLE: Partition " + context.getPartitionId() + " is opening for host " + hostname.substring(hostname.length() - 4));
        }

        public void onClose(PartitionContext context, CloseReason reason) throws Exception
        {
            String hostname = context.getLease().getOwner();
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " is closing for reason " + reason.toString() + " for host " + hostname.substring(hostname.length() - 4));
        }

        public void onEvents(PartitionContext context, Iterable<EventData> messages) throws Exception
        {
            String hostname = context.getLease().getOwner();
            hostname = hostname.substring(hostname.length() - 4);
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " got message batch for host " + hostname);
            int messageCount = 0;
            for (EventData data : messages)
            {
                System.out.println("SAMPLE (" + hostname + "," + context.getPartitionId() + "): " + new String(data.getBody(), "UTF8"));
                messageCount++;
            }
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " batch size was " + messageCount + " for host " + hostname);
        }
    }
}

