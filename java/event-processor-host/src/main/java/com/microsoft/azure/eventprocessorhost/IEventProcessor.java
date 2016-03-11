/*
 * LICENSE GOES HERE
 */

package com.microsoft.azure.eventprocessorhost;

import com.microsoft.azure.eventhubs.EventData;


/**
 * Interface that must be implemented by event processor classes.
 *
 * <p>
 * Any given instance of an event processor class will only process events from one partition
 * of one Event Hub. A PartitionContext is provided with each call to the event processor because
 * some parameters could change, but it will always be the same partition.
 * <p>
 * Although EventProcessorHost is multithreaded, calls to a given instance of an event processor
 * class are serialized. onOpen() is called first, then onEvents() will be called zero or more
 * times. When the event processor needs to be shut down, whether because there was a failure
 * somewhere, or the lease for the partition has been lost, or because the entire processor host
 * is being shut down, onClose() is called after the last onEvents() call returns.
 *
 */
public interface IEventProcessor
{
	/**
	 * Called by processor host to initialize the event processor.
	 *  
	 * @param context	Information about the partition that this event processor will process events from.
	 * @throws Exception
	 */
    public void onOpen(PartitionContext context) throws Exception;

    /**
     * Called by processor host to indicate that the event processor is being stopped.
     * 
     * @param context	Information about the partition.
     * @param reason	Reason why the event processor is being stopped.
     * @throws Exception
     */
    public void onClose(PartitionContext context, CloseReason reason) throws Exception;

    /**
     * Called by the processor host when a batch of events has arrived.
     * 
     * This is where the real work of the event processor is done.
     * 
     * @param context	Information about the partition.
     * @param messages	The events to be processed.
     * @throws Exception
     */
    public void onEvents(PartitionContext context, Iterable<EventData> messages) throws Exception;
}