package com.microsoft.azure.eventprocessorhost;


/**
 * Interface that must be implemented by an event processor factory class.
 *
 * @param <T>	The type of event processor objects produced by this factory, which must implement IEventProcessor
 */
public interface IEventProcessorFactory<T extends IEventProcessor>
{
    public T createEventProcessor(PartitionContext context) throws Exception;
}
