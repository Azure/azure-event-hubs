package com.microsoft.azure.eventhubs.samples;


import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventprocessorhost.*;
import com.microsoft.azure.servicebus.ClientConstants;

import java.io.IOException;
import java.util.concurrent.Future;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

public class EventProcessorSample {
    public static void main(String args[])
    {
    	int hostCount = 2;
    	PartitionManager.dummyPartitionCount = 4;
    	
        FileHandler fhc = null;
        Boolean tracing = true;
        if (tracing)
        {
			try
			{
				fhc = new FileHandler("f:\\amqpClientframes.log", false);
			}
			catch (SecurityException e2)
			{
				// TODO Auto-generated catch block
				e2.printStackTrace();
			}
			catch (IOException e2)
			{
				// TODO Auto-generated catch block
				e2.printStackTrace();
			} 
	        Logger lc = Logger.getLogger(ClientConstants.ServiceBusClientTrace);
	        fhc.setFormatter(new SimpleFormatter());
	        lc.addHandler(fhc);
	        lc.setLevel(Level.ALL);
        }
    	
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

        if (tracing)
        {
        	fhc.flush();
        }
        System.out.println("Exiting");
    }


    public static class EventProcessor implements IEventProcessor
    {
        public void onOpen(PartitionContext context) throws Exception
        {
            System.out.println("SAMPLE: Partition " + context.getLease().getPartitionId() + " is opening");
        }

        public void onClose(PartitionContext context, CloseReason reason) throws Exception
        {
            System.out.println("SAMPLE: Partition " + context.getLease().getPartitionId() + " is closing for reason " + reason.toString());
        }

        public void onEvents(PartitionContext context, Iterable<EventData> messages) throws Exception
        {
            System.out.println("SAMPLE: Partition " + context.getLease().getPartitionId() + " got batch");
            int count = 0;
            String lastOffset = "OOPS";
            if (messages != null)
            {
	            for (EventData data : messages)
	            {
	                System.out.println(new String(data.getBody(), "UTF8"));
	                count++;
	                lastOffset = data.getSystemProperties().getOffset();
	            }
            }
            System.out.println("SAMPLE: Partition " + context.getLease().getPartitionId() + " batch was " + count + " messages ending at offset " + lastOffset);
        }

    }
}
