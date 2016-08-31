/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.Iterator;
import java.util.logging.Level;

import com.microsoft.azure.eventhubs.EventData;

abstract class PartitionPump
{
	protected final EventProcessorHost host;
	protected Lease lease = null;
	
	protected PartitionPumpStatus pumpStatus = PartitionPumpStatus.PP_UNINITIALIZED;
	
    protected IEventProcessor processor = null;
    protected PartitionContext partitionContext = null;
    
    protected final Object processingSynchronizer;
    
	PartitionPump(EventProcessorHost host, Lease lease)
	{
		this.host = host;
		this.lease = lease;
		this.processingSynchronizer = new Object();
	}
	
	void setLease(Lease newLease)
	{
		this.partitionContext.setLease(newLease);
	}
	
	// return Void so it can be called from a lambda submitted to ExecutorService
    Void startPump()
    {
    	this.pumpStatus = PartitionPumpStatus.PP_OPENING;
    	
        this.partitionContext = new PartitionContext(this.host, this.lease.getPartitionId(), this.host.getEventHubPath(), this.host.getConsumerGroupName());
        this.partitionContext.setLease(this.lease);
        
        if (this.pumpStatus == PartitionPumpStatus.PP_OPENING)
        {
        	String action = EventProcessorHostActionStrings.CREATING_EVENT_PROCESSOR;
        	try
        	{
				this.processor = this.host.getProcessorFactory().createEventProcessor(this.partitionContext);
				action = EventProcessorHostActionStrings.OPENING_EVENT_PROCESSOR;
	            this.processor.onOpen(this.partitionContext);
        	}
            catch (Exception e)
            {
            	// If the processor won't create or open, only thing we can do here is pass the buck.
            	// Null it out so we don't try to operate on it further.
            	this.processor = null;
            	this.host.logWithHostAndPartition(Level.SEVERE, this.partitionContext, "Failed " + action, e);
            	this.host.getEventProcessorOptions().notifyOfException(this.host.getHostName(), e, action);
            	
            	this.pumpStatus = PartitionPumpStatus.PP_OPENFAILED;
            }
        }

        if (this.pumpStatus == PartitionPumpStatus.PP_OPENING)
        {
        	specializedStartPump();
        }
        
        return null;
    }

    abstract void specializedStartPump();

    PartitionPumpStatus getPumpStatus()
    {
    	return this.pumpStatus;
    }
    
    Boolean isClosing()
    {
    	return ((this.pumpStatus == PartitionPumpStatus.PP_CLOSING) || (this.pumpStatus == PartitionPumpStatus.PP_CLOSED));
    }

    void shutdown(CloseReason reason)
    {
    	this.pumpStatus = PartitionPumpStatus.PP_CLOSING;
        this.host.logWithHostAndPartition(Level.INFO, this.partitionContext, "pump shutdown for reason " + reason.toString());

        specializedShutdown(reason);

        if (this.processor != null)
        {
            try
            {
            	synchronized(this.processingSynchronizer)
            	{
            		// When we take the lock, any existing onEvents call has finished.
                	// Because the client has been closed, there will not be any more
                	// calls to onEvents in the future. Therefore we can safely call onClose.
            		this.processor.onClose(this.partitionContext, reason);
                }
            }
            catch (Exception e)
            {
            	this.host.logWithHostAndPartition(Level.SEVERE, this.partitionContext, "Failure closing processor", e);
            	// If closing the processor has failed, the state of the processor is suspect.
            	// Report the failure to the general error handler instead.
            	this.host.getEventProcessorOptions().notifyOfException(this.host.getHostName(), e, "Closing Event Processor");
            }
        }
        
        this.pumpStatus = PartitionPumpStatus.PP_CLOSED;
    }
    
    abstract void specializedShutdown(CloseReason reason);
    
    protected void onEvents(Iterable<EventData> events)
	{
    	try
        {
        	// Synchronize to serialize calls to the processor.
        	// The handler is not installed until after onOpen returns, so onEvents cannot conflict with onOpen.
        	// There could be a conflict between onEvents and onClose, however. All calls to onClose are
        	// protected by synchronizing too.
        	synchronized(this.processingSynchronizer)
        	{
        		this.processor.onEvents(this.partitionContext, events);
        		
        		if (events != null)
        		{
	        		Iterator<EventData> blah = events.iterator();
	        		EventData last = null;
	        		while (blah.hasNext())
	        		{
	        			last = blah.next();
	        		}
	        		if (last != null)
	        		{
	        			this.host.logWithHostAndPartition(Level.FINE, this.partitionContext, "Updating offset in partition context with end of batch " +
	        					last.getSystemProperties().getOffset() + "//" + last.getSystemProperties().getSequenceNumber());
	        			this.partitionContext.setOffsetAndSequenceNumber(last);
	        		}
        		}
        	}
        }
        catch (Exception e)
        {
            // TODO -- do we pass errors from IEventProcessor.onEvents to IEventProcessor.onError?
        	// Depending on how you look at it, that's either pointless (if the user's code throws, the user's code should already know about it) or
        	// a convenient way of centralizing error handling.
        	// In the meantime, just trace it.
        	this.host.logWithHostAndPartition(Level.SEVERE, this.partitionContext, "Got exception from onEvents", e);
        }
	}
    
    protected void onError(Throwable error)
    {
    	// This handler is called when javaClient calls the error handler we have installed.
    	// JavaClient can only do that when execution is down in javaClient. Therefore no onEvents
    	// call can be in progress right now. JavaClient will not get control back until this handler
    	// returns, so there will be no calls to onEvents until after the user's error handler has returned.
    	this.processor.onError(this.partitionContext, error);
    }
}
