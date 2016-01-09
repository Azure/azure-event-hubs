package com.microsoft.azure.eventprocessorhost;


public interface IEventProcessorFactory<T extends IEventProcessor>
{
    public T createEventProcessor(Class<T> eventProcessorClass, PartitionContext context) throws Exception;
}
