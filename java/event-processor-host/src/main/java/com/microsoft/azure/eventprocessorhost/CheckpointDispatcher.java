/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.LinkedList;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.logging.Level;

public class CheckpointDispatcher
{
	private EventProcessorHost host;
	private ICheckpointManager checkpointManager;
	
	private LinkedList<Checkpoint> workQueue;
	private boolean keepGoing = true;
	private Future<Void> workThread = null;
	
	public CheckpointDispatcher(EventProcessorHost host)
	{
		this.host = host;
		this.checkpointManager = host.getCheckpointManager();
		
		this.workQueue = new LinkedList<Checkpoint>();
	}
	
	public synchronized void startCheckpointDispatcher()
	{
		if ((this.workThread == null) || this.workThread.isDone())
		{
			this.keepGoing = true;
			this.workThread = EventProcessorHost.getExecutorService().submit(() -> runWorkThread());
		}
	}
	
	public void stopCheckpointDispatcher()
	{
		this.keepGoing = false;
		try
		{
			if (this.workThread != null)
			{
				this.workThread.get();
			}
		}
		catch (InterruptedException | ExecutionException e)
		{
			// Worker thread failed on shutdown. Nothing to do about it except log.
			this.host.logWithHost(Level.WARNING, "Checkpoint dispatcher thread failed on shutdown", e);
		}
	}
	
	public void enqueueCheckpoint(Checkpoint checkpoint)
	{
		synchronized (this.workQueue)
		{
			this.workQueue.addFirst(checkpoint);
		}
		// The work thread shuts down if it has nothing to do, so when new work comes in make sure that it
		// is running. startCheckpointDispatcher() is idempotent and will not start a second thread if one
		// is already running, so we can just call it blindly here.
		startCheckpointDispatcher();
	}
	
	private Void runWorkThread()
	{
		String shutdownReason = "requested shutdown";
		
		this.host.logWithHost(Level.INFO, "Checkpoint dispatcher starting");
		
		while (this.keepGoing)
		{
			// Check for queue size separately so that a shutdown request cannot
			// stop the thread until all queued checkpoints have been saved.
			while (this.workQueue.size() > 0)
			{
				Checkpoint workToDo = null;
				synchronized (this.workQueue)
				{
					workToDo = this.workQueue.removeLast();
				}
				
		    	this.host.logWithHostAndPartition(Level.FINE, workToDo.getPartitionId(), "CheckPoint dispatcher processing: " +
		    			workToDo.getOffset() + "//" + workToDo.getSequenceNumber());
				
				try
				{
					// Because there is only one checkpoint updater thread, and it processes checkpoints in order,
					// the offset should never go backwards. If it tries to, we skip the update, but can't do anything
					// except log a trace.
			    	Checkpoint inStoreCheckpoint = this.checkpointManager.getCheckpoint(workToDo.getPartitionId()).get();
			    	if (workToDo.getSequenceNumber() >= inStoreCheckpoint.getSequenceNumber())
			    	{
				    	inStoreCheckpoint.setOffset(workToDo.getOffset());
				    	inStoreCheckpoint.setSequenceNumber(workToDo.getSequenceNumber());
				        this.checkpointManager.updateCheckpoint(inStoreCheckpoint).get();
			    	}
			    	else
			    	{
			    		this.host.logWithHostAndPartition(Level.SEVERE, workToDo.getPartitionId(),
			    			"Abandoning out of date checkpoint " + workToDo.getOffset() + "//" + workToDo.getSequenceNumber() +
			    			" because store is at " + inStoreCheckpoint.getOffset() + "//" + inStoreCheckpoint.getSequenceNumber());
			    	}
				}
				catch (ExecutionException | InterruptedException e)
				{
					this.host.logWithHost(Level.SEVERE, "Exception in checkpoint dispatcher", e);
					shutdownReason = "error";
					this.keepGoing = false;
					break;
				}
			}
			
			if (this.keepGoing)
			{
				// We can only get here if queue size is 0. Sleep a few seconds to see if more work
				// comes in, so we don't flap the thread.
				
				try
				{
					Thread.sleep(2000);
				}
				catch (InterruptedException e)
				{
					shutdownReason = "interrupted";
					break;
				}
				
				if (this.workQueue.size() <= 0)
				{
					shutdownReason = "out of work";
					break;
				}
			}
		}
		
		this.host.logWithHost(Level.INFO, "Checkpoint dispatcher shutting down: " + shutdownReason);
		return null;
	}
}
