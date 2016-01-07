package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.eventhubs.EventData;


public interface IEventProcessor
{
    public void onOpen(PartitionContext context) throws Exception;

    public void onClose(PartitionContext context, CloseReason reason) throws Exception;

    public void onEvents(PartitionContext context, Iterable<EventData> messages) throws Exception;
}
