package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.Callable;

/**
 * Created by jbird on 10/27/2015.
 */
public interface IEventProcessorFactory
{
    public IEventProcessor CreateEventProcessor(Callable<IEventProcessor> maker, PartitionContext context) throws Exception;
}
