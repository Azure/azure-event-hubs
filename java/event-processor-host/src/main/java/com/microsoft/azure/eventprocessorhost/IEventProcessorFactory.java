package com.microsoft.azure.eventprocessorhost;


public interface IEventProcessorFactory<T extends IEventProcessor>
{
    public void setEventProcessorClass(Class<T> eventProcessorClass);

    public T createEventProcessor(PartitionContext context) throws Exception;
}
