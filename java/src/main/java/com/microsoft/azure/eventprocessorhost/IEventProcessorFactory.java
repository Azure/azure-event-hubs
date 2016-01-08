package com.microsoft.azure.eventprocessorhost;


public interface IEventProcessorFactory
{
    public IEventProcessor createEventProcessor(PartitionContext context) throws Exception;
}
