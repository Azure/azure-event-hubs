/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.servicebus;

import java.io.IOException;
import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.concurrent.CompletableFuture;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.apache.qpid.proton.amqp.transport.ErrorCondition;
import org.apache.qpid.proton.engine.BaseHandler;
import org.apache.qpid.proton.engine.Connection;
import org.apache.qpid.proton.engine.EndpointState;
import org.apache.qpid.proton.engine.Event;
import org.apache.qpid.proton.engine.Handler;
import org.apache.qpid.proton.engine.HandlerException;
import org.apache.qpid.proton.engine.Link;
import org.apache.qpid.proton.reactor.Reactor;

import com.microsoft.azure.servicebus.amqp.BaseLinkHandler;
import com.microsoft.azure.servicebus.amqp.ConnectionHandler;
import com.microsoft.azure.servicebus.amqp.IAmqpConnection;
import com.microsoft.azure.servicebus.amqp.ProtonUtil;
import com.microsoft.azure.servicebus.amqp.ReactorHandler;

/**
 * Abstracts all amqp related details and exposes AmqpConnection object
 * Manages connection life-cycle
 */
public class MessagingFactory extends ClientEntity implements IAmqpConnection, IConnectionFactory, ITimeoutErrorHandler
{
	
	public static final Duration DefaultOperationTimeout = Duration.ofSeconds(60); 
	
	private static final Logger TRACE_LOGGER = Logger.getLogger(ClientConstants.SERVICEBUS_CLIENT_TRACE);
	private static final int TIMEOUT_ERROR_THRESHOLD_IN_SECS = 180;
	private final Object connectionLock = new Object();
	private final String hostName;
	
	private Reactor reactor;
	private Thread reactorThread;
	private ConnectionHandler connectionHandler;
	private Connection connection;
	private boolean waitingConnectionOpen;
	
	private Duration operationTimeout;
	private RetryPolicy retryPolicy;
	private CompletableFuture<MessagingFactory> open;
	private CompletableFuture<Connection> openConnection;
	private LinkedList<Link> registeredLinks;
	private TimeoutTracker connectionCreateTracker;
	private Instant timeoutErrorStart;
	
	/**
	 * @param reactor parameter reactor is purely for testing purposes and the SDK code should always set it to null
	 */
	MessagingFactory(final ConnectionStringBuilder builder) throws IOException
	{
		super("MessagingFactory".concat(StringUtil.getRandomString()));
		this.hostName = builder.getEndpoint().getHost();
		this.timeoutErrorStart = null;
		
		this.startReactor(new ReactorHandler()
		{
			@Override
			public void onReactorFinal(Event e)
		    {
				super.onReactorFinal(e);
				MessagingFactory.this.onReactorError(new ServiceBusException(true, "Reactor finalized."));
		    }
		});
		
		this.operationTimeout = builder.getOperationTimeout();
		this.retryPolicy = builder.getRetryPolicy();
		this.registeredLinks = new LinkedList<Link>();
	}
	
	String getHostName()
	{
		return this.hostName;
	}
	
	private void createConnection(ConnectionStringBuilder builder)
	{
		assert this.reactor != null;
		
		this.connectionHandler = new ConnectionHandler(this, 
				builder.getEndpoint().getHost(), builder.getSasKeyName(), builder.getSasKey());
		this.waitingConnectionOpen = true;
		this.connection = reactor.connection(this.connectionHandler);
		this.open = new CompletableFuture<MessagingFactory>();
	}

	private void startReactor(ReactorHandler reactorHandler) throws IOException
	{
		this.reactor = ProtonUtil.reactor(reactorHandler);
		this.reactorThread = new Thread(new RunReactor(this.reactor));
		this.reactorThread.start();
	}
	
	@Override
	public CompletableFuture<Connection> getConnection()
	{
		if (this.connection.getLocalState() == EndpointState.CLOSED
				|| (this.connectionCreateTracker != null && this.connectionCreateTracker.remaining().minus(ClientConstants.TIMER_TOLERANCE).isNegative()))
		{
			synchronized (this.connectionLock)
			{
				if ((this.connection.getLocalState() == EndpointState.CLOSED && !this.waitingConnectionOpen)
						|| (this.connectionCreateTracker != null && this.connectionCreateTracker.remaining().minus(ClientConstants.TIMER_TOLERANCE).isNegative()))
				{
					try
					{
						this.startReactor(new ReactorHandler()
						{
							@Override
							public void onReactorInit(Event e)
							{
								super.onReactorInit(e);
								
								Reactor reactor = e.getReactor();
								MessagingFactory.this.connection = reactor.connection(MessagingFactory.this.connectionHandler);
							}
							
							@Override
							public void onReactorFinal(Event e)
						    {
								super.onReactorFinal(e);
								MessagingFactory.this.onReactorError(new ServiceBusException(true, "Reactor finalized."));
						    }
						});
					}
					catch (IOException e)
					{
						MessagingFactory.this.onReactorError(new ServiceBusException(true, e));
					}
					
					if(this.openConnection != null && !this.openConnection.isDone())
					{
						this.openConnection.completeExceptionally(new ServiceBusException(false, "Connection creation timedout."));
					}

					this.openConnection = new CompletableFuture<Connection>();
					
					this.connectionCreateTracker = TimeoutTracker.create(this.operationTimeout);
					this.waitingConnectionOpen = true;
				}
			}
		}
		
		return this.openConnection == null ? CompletableFuture.completedFuture(this.connection): this.openConnection;
	}
	
	public Duration getOperationTimeout()
	{
		return this.operationTimeout;
	}
	
	public RetryPolicy getRetryPolicy()
	{
		return this.retryPolicy;
	}
	
	public static CompletableFuture<MessagingFactory> createFromConnectionString(final String connectionString) throws IOException
	{
		ConnectionStringBuilder builder = new ConnectionStringBuilder(connectionString);
		MessagingFactory messagingFactory = new MessagingFactory(builder);
		
		messagingFactory.createConnection(builder);
		return messagingFactory.open;
	}
	
	@Override
	public void onOpenComplete(Exception exception)
	{
		synchronized (this.connectionLock)
		{
			this.waitingConnectionOpen = false;
		}
		
		if (exception == null)
		{
			this.open.complete(this);
			if(this.openConnection != null)
			{
				this.openConnection.complete(this.connection);
			}
		}
		else
		{
			this.open.completeExceptionally(exception);
			if (this.openConnection != null)
			{
				this.openConnection.completeExceptionally(exception);
			}
		}
	}
	
	@Override
	public void onConnectionError(ErrorCondition error)
	{
		Connection currentConnection = this.connection;
		
		Iterator<Link> literator = this.registeredLinks.iterator();
		while (literator.hasNext())
		{
			Link link = literator.next();
			if (link.getLocalState() != EndpointState.CLOSED)
			{
				link.close();
			}
		}
		
		try
		{
			if (currentConnection.getLocalState() != EndpointState.CLOSED)
			{
				currentConnection.close();
			}
			
			literator = this.registeredLinks.iterator();
			while (literator.hasNext())
			{
				Link link = literator.next();
				Handler handler = BaseHandler.getHandler(link);
				if (handler != null && handler instanceof BaseLinkHandler)
				{
					BaseLinkHandler linkHandler = (BaseLinkHandler) handler;
					linkHandler.processOnClose(link, error);
				}
			}
		}
		finally
		{
			currentConnection.free();
		}
	}
	
	private void onReactorError(Exception cause)
	{
		if (!this.open.isDone())
		{
			this.onOpenComplete(cause);
			return;
		}

		Connection currentConnection = this.connection;
		
		Iterator<Link> literator = this.registeredLinks.iterator();
		while (literator.hasNext())
		{
			Link link = literator.next();
			if (link.getLocalState() != EndpointState.CLOSED)
			{
				link.close();
			}
		}
		
		try
		{
			if (currentConnection != null && currentConnection.getLocalState() != EndpointState.CLOSED)
			{
				currentConnection.close();
			}
			
			literator = this.registeredLinks.iterator();
			while (literator.hasNext())
			{
				Link link = literator.next();
				Handler handler = BaseHandler.getHandler(link);
				if (handler != null && handler instanceof BaseLinkHandler)
				{
					BaseLinkHandler linkHandler = (BaseLinkHandler) handler;
					linkHandler.processOnClose(link, cause);
				}
			}
		}
		finally
		{
			currentConnection.free();
		}
	}
	
	void resetConnection()
	{		
		this.reactor.free();
		this.onReactorError(new ServiceBusException(true, "Client invoked connection reset."));
	}
	
	@Override
	public void closeSync()
	{
		if (this.connection != null)
		{
			if (this.connection.getLocalState() != EndpointState.CLOSED)
			{
				this.connection.close();
			}
			
			this.connection.free();
		}
	}

	@Override
	public CompletableFuture<Void> close()
	{
		this.closeSync();
		
		// hook up onRemoteClose & timeout 
		return CompletableFuture.completedFuture(null);
	}

	private class RunReactor implements Runnable
	{
		private Reactor r;
		
		public RunReactor(Reactor r)
		{
			this.r = r;
		}
		
		public void run()
		{
			if(TRACE_LOGGER.isLoggable(Level.FINE))
		    {
				TRACE_LOGGER.log(Level.FINE, "starting reactor instance.");
		    }
			
			try
			{
				this.r.run();
			}
			catch (HandlerException handlerException)
			{
				Exception cause = handlerException;
				
				if(TRACE_LOGGER.isLoggable(Level.WARNING))
			    {
					TRACE_LOGGER.log(Level.WARNING, "UnHandled exception while processing events in reactor:");
					TRACE_LOGGER.log(Level.WARNING, handlerException.getMessage());
					if (handlerException.getStackTrace() != null)
						for (StackTraceElement ste: handlerException.getStackTrace())
						{
							TRACE_LOGGER.log(Level.WARNING, ste.toString());
						}
			    }
				
				MessagingFactory.this.onReactorError(new ServiceBusException(true, cause));
			}
		}
	}

	@Override
	public void registerForConnectionError(Link link)
	{
		this.registeredLinks.add(link);	
	}

	@Override
	public void deregisterForConnectionError(Link link)
	{
		this.registeredLinks.remove(link);	
	}

	@Override
	public void reportTimeoutError()
	{
		if (this.timeoutErrorStart == null)
			this.timeoutErrorStart = Instant.now();
		else if (this.timeoutErrorStart.isBefore(Instant.now().minus(TIMEOUT_ERROR_THRESHOLD_IN_SECS, ChronoUnit.SECONDS)))
		{
			this.resetConnection();
			this.resetTimeoutErrorTracking();
		}
	}

	@Override
	public void resetTimeoutErrorTracking()
	{
		this.timeoutErrorStart = null;
	}	
}
