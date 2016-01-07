package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.Callable;


public class DefaultEventProcessorFactory implements IEventProcessorFactory
{
    public DefaultEventProcessorFactory()
    {
    }

    public IEventProcessor CreateEventProcessor(Callable<IEventProcessor> maker, PartitionContext context) throws Exception
    {
        IEventProcessor processor = maker.call();
        // TODO get lease etc?
        return processor;
    }
}
