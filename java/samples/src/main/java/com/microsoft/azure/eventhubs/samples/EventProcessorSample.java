package com.microsoft.azure.eventhubs.samples;


import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventprocessorhost.*;

import java.util.concurrent.Callable;

public class EventProcessorSample {
    public static void main(String args[])
    {
        EventProcessorHost host = new EventProcessorHost("namespace", "eventhub", "keyname",
                "key", "$Default", "storage connection string");
        host.registerEventProcessorFactory(new EventProcessorFactory());
    }


    private static class EventProcessorFactory implements IEventProcessorFactory
    {
        public IEventProcessor createEventProcessor(PartitionContext context) throws Exception
        {
            return new EventProcessor();
        }
    }

    private static class EventProcessor implements IEventProcessor
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
                System.out.println(data.toString());
            }
        }

    }
}
