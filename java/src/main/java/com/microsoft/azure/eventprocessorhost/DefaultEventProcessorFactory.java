package com.microsoft.azure.eventprocessorhost;


class DefaultEventProcessorFactory<T extends IEventProcessor> implements IEventProcessorFactory<T>
{
    public T createEventProcessor(Class<T> eventProcessorClass, PartitionContext context) throws Exception
    {
        return eventProcessorClass.newInstance();
    }
}
