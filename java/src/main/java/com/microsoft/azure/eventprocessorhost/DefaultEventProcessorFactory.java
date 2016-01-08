package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.Callable;


public class DefaultEventProcessorFactory implements IEventProcessorFactory
{
    Callable<IEventProcessor> createProcessor;

    public DefaultEventProcessorFactory(Callable<IEventProcessor> createProcessor)
    {
        this.createProcessor = createProcessor;
    }

    public IEventProcessor createEventProcessor(PartitionContext context) throws Exception
    {
        IEventProcessor processor = this.createProcessor.call();
        // TODO get lease etc?
        return processor;
    }
}
