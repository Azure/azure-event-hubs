package com.microsoft.azure.eventhubs.samples;


import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventprocessorhost.*;

import java.io.IOException;
import java.util.concurrent.Callable;
import java.util.concurrent.Future;

public class EventProcessorSample {
    public static void main(String args[])
    {
    	PartitionManager.dummyHostCount = 3;
    	PartitionManager.dummyPartitionCount = 8;
    	
    	EventProcessorHost[] hosts = new EventProcessorHost[PartitionManager.dummyHostCount];
    	
    	for (int i = 0; i < PartitionManager.dummyHostCount; i++)
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
            for (int i = 0; i < PartitionManager.dummyHostCount; i++)
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
            System.out.println("Partition " + context.getLease().getPartitionId() + " is opening");
        }

        public void onClose(PartitionContext context, CloseReason reason) throws Exception
        {
            System.out.println("Partition " + context.getLease().getPartitionId() + " is closing for reason " + reason.toString());
        }

        public void onEvents(PartitionContext context, Iterable<EventData> messages) throws Exception
        {
            System.out.println("Partition " + context.getLease().getPartitionId() + " got messages");
            for (EventData data : messages)
            {
                System.out.println(new String(data.getBody(), "UTF8"));
            }
        }

    }
}

