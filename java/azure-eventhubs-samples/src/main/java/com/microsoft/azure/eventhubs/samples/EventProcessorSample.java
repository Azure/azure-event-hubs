package com.microsoft.azure.eventhubs.samples;


import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventprocessorhost.*;

import java.io.IOException;
import java.util.concurrent.Callable;
import java.util.concurrent.Future;

public class EventProcessorSample {
    public static void main(String args[])
    {
    	int hostCount = 2;
    	PartitionManager.dummyPartitionCount = 4;
    	
    	EventProcessorHost[] hosts = new EventProcessorHost[hostCount];
    	
    	for (int i = 0; i < hostCount; i++)
    	{
    		hosts[i] = new EventProcessorHost("namespace", "eventhub", "keyname", "key", "$Default", "storage connection string");
    		System.out.println("Registering host " + i + " named " + hosts[i].getHostName());
    		hosts[i].registerEventProcessor(EventProcessor.class);
    		try
    		{
    			Thread.sleep(3000);
    		}
    		catch (InterruptedException e1)
    		{
    			// TODO Auto-generated catch block
    			e1.printStackTrace();
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

        System.out.println("Exiting");
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

