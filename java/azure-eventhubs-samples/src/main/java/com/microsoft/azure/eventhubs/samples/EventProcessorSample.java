package com.microsoft.azure.eventhubs.samples;


import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventprocessorhost.*;

import java.util.ArrayList;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

public class EventProcessorSample {
    public static void main(String args[])
    {
    	final int hostCount = 2;
    	
    	String ehConsumerGroup = "$Default";
    	String ehNamespace = "";
    	String ehEventhub = "";
    	String ehKeyname = "";
    	String ehKey = "";
    	String storageConnectionString = "this is not a valid storage connection string";
    	
    		EventProcessorHost[] hosts = new EventProcessorHost[hostCount];
    		for (int i = 0; i < hostCount; i++)
    		{
    			hosts[i] = new EventProcessorHost(ehNamespace, ehEventhub, ehKeyname, ehKey, ehConsumerGroup, storageConnectionString);
    		}
    		processMessages(hosts);
    	
        System.out.println("End of sample");
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
    
    public static class EventProcessor implements IEventProcessor
    {
    	private int checkpointBatchingCount = 0;
    	
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
                System.out.print("SAMPLE (" + hostname + "," + context.getPartitionId() + ",");
                System.out.print(data.getSystemProperties().getOffset() + "," + data.getSystemProperties().getSequenceNumber() + "): ");
                System.out.println(new String(data.getBody(), "UTF8"));
                messageCount++;
                this.checkpointBatchingCount++;
                if ((checkpointBatchingCount % 5) == 0)
                {
                	System.out.println("SAMPLE: Partition " + context.getPartitionId() + " checkpointing at " +
               			data.getSystemProperties().getOffset() + "," + data.getSystemProperties().getSequenceNumber());
                	context.checkpoint(data);
                }
            }
            System.out.println("SAMPLE: Partition " + context.getPartitionId() + " batch size was " + messageCount + " for host " + hostname);
        }
    	
    	@Override
    	public void onError(PartitionContext context, Throwable error)
    	{
    		System.out.println("SAMPLE: Partition " + context.getPartitionId() + " onError: " + error.toString());
    	}

    }
}

