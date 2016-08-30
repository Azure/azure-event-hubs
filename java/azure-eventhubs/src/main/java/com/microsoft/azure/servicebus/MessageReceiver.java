/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.servicebus;

import java.io.IOException;
import java.time.Duration;
import java.time.Instant;
import java.time.ZonedDateTime;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.apache.qpid.proton.Proton;
import org.apache.qpid.proton.amqp.Symbol;
import org.apache.qpid.proton.amqp.UnknownDescribedType;
import org.apache.qpid.proton.amqp.messaging.Source;
import org.apache.qpid.proton.amqp.messaging.Target;
import org.apache.qpid.proton.amqp.transport.ErrorCondition;
import org.apache.qpid.proton.amqp.transport.ReceiverSettleMode;
import org.apache.qpid.proton.amqp.transport.SenderSettleMode;
import org.apache.qpid.proton.engine.BaseHandler;
import org.apache.qpid.proton.engine.Connection;
import org.apache.qpid.proton.engine.Delivery;
import org.apache.qpid.proton.engine.EndpointState;
import org.apache.qpid.proton.engine.Receiver;
import org.apache.qpid.proton.engine.Session;
import org.apache.qpid.proton.message.Message;

import com.microsoft.azure.servicebus.amqp.AmqpConstants;
import com.microsoft.azure.servicebus.amqp.DispatchHandler;
import com.microsoft.azure.servicebus.amqp.IAmqpReceiver;
import com.microsoft.azure.servicebus.amqp.ReceiveLinkHandler;
import com.microsoft.azure.servicebus.amqp.SessionHandler;

/**
 * Common Receiver that abstracts all amqp related details
 * translates event-driven reactor model into async receive Api
 */
public class MessageReceiver extends ClientEntity implements IAmqpReceiver, IErrorContextProvider
{
	private static final Logger TRACE_LOGGER = Logger.getLogger(ClientConstants.SERVICEBUS_CLIENT_TRACE);
	private static final int MIN_TIMEOUT_DURATION_MILLIS = 20;

	private final ConcurrentLinkedQueue<ReceiveWorkItem> pendingReceives;
	private final MessagingFactory underlyingFactory;
	private final String receivePath;
	private final Runnable onOperationTimedout;
	private final Duration operationTimeout;
	private final CompletableFuture<Void> linkClose;
	private final Object prefetchCountSync;
	private int prefetchCount;

	private ConcurrentLinkedQueue<Message> prefetchedMessages;
	private Receiver receiveLink;
	private WorkItem<MessageReceiver> linkOpen;
	private Duration receiveTimeout;

	private long epoch;
	private boolean isEpochReceiver;
	private Instant dateTime;
	private boolean offsetInclusive;

	private String lastReceivedOffset;
	private Exception lastKnownLinkError;
	private int nextCreditToFlow;

	private MessageReceiver(final MessagingFactory factory,
			final String name, 
			final String recvPath, 
			final String offset,
			final boolean offsetInclusive,
			final Instant dateTime,
			final int prefetchCount,
			final Long epoch,
			final boolean isEpochReceiver)
	{
		super(name, factory);

		this.underlyingFactory = factory;
		this.operationTimeout = factory.getOperationTimeout();
		this.receivePath = recvPath;
		this.prefetchCount = prefetchCount;
		this.epoch = epoch;
		this.isEpochReceiver = isEpochReceiver;
		this.prefetchedMessages = new ConcurrentLinkedQueue<Message>();
		this.linkClose = new CompletableFuture<Void>();
		this.lastKnownLinkError = null;
		this.receiveTimeout = factory.getOperationTimeout();
		this.prefetchCountSync = new Object();

		if (offset != null)
		{
			this.lastReceivedOffset = offset;
			this.offsetInclusive = offsetInclusive;
		}
		else
		{
			this.dateTime = dateTime;
		}

		this.pendingReceives = new ConcurrentLinkedQueue<ReceiveWorkItem>();

		// onOperationTimeout delegate - per receive call
		this.onOperationTimedout = new Runnable()
		{
			public void run()
			{
				WorkItem<Collection<Message>> topWorkItem = null;
				boolean workItemTimedout = false;
				while((topWorkItem = MessageReceiver.this.pendingReceives.peek()) != null)
				{
					if (topWorkItem.getTimeoutTracker().remaining().toMillis() <= MessageReceiver.MIN_TIMEOUT_DURATION_MILLIS)
					{
						WorkItem<Collection<Message>> dequedWorkItem = MessageReceiver.this.pendingReceives.poll();
						if (dequedWorkItem != null)
						{
							workItemTimedout = true;
							dequedWorkItem.getWork().complete(null);
						}
						else
							break;
					}
					else
					{
						MessageReceiver.this.scheduleOperationTimer(topWorkItem.getTimeoutTracker());
						break;
					}
				}

				if (workItemTimedout)
				{
					// workaround to push the sendflow-performative to reactor
					// this sets the receiveLink endpoint to modified state
					// (and increment the unsentCredits in proton by 0)
					try
					{
						MessageReceiver.this.underlyingFactory.scheduleOnReactorThread(new DispatchHandler()
						{
							@Override
							public void onEvent()
							{
								MessageReceiver.this.receiveLink.flow(0);
							}
						});
					}
					catch (IOException ignore)
					{
					}
				}
			}
		};
	}

	// @param connection Connection on which the MessageReceiver's receive Amqp link need to be created on.
	// Connection has to be associated with Reactor before Creating a receiver on it.
	public static CompletableFuture<MessageReceiver> create(
			final MessagingFactory factory, 
			final String name, 
			final String recvPath, 
			final String offset,
			final boolean offsetInclusive,
			final Instant dateTime,
			final int prefetchCount,
			final long epoch,
			final boolean isEpochReceiver)
	{
		MessageReceiver msgReceiver = new MessageReceiver(
				factory,
				name, 
				recvPath, 
				offset, 
				offsetInclusive, 
				dateTime, 
				prefetchCount, 
				epoch, 
				isEpochReceiver);
		return msgReceiver.createLink();
	}

	private CompletableFuture<MessageReceiver> createLink()
	{
		this.linkOpen = new WorkItem<MessageReceiver>(new CompletableFuture<MessageReceiver>(), this.operationTimeout);
		this.scheduleLinkOpenTimeout(this.linkOpen.getTimeoutTracker());
		try
		{
			this.underlyingFactory.scheduleOnReactorThread(new DispatchHandler()
			{
				@Override
				public void onEvent()
				{
					MessageReceiver.this.createReceiveLink();
				}
			});
		}
		catch (IOException ioException)
		{
			this.linkOpen.getWork().completeExceptionally(new ServiceBusException(false, "Failed to create Receiver, see cause for more details.", ioException));
		}

		return this.linkOpen.getWork();
	}

	private List<Message> receiveCore(final int messageCount)
	{
		List<Message> returnMessages = null;
		Message currentMessage = this.pollPrefetchQueue();
	
		while (currentMessage != null) 
		{
			if (returnMessages == null)
			{
				returnMessages = new LinkedList<Message>();
			}

			returnMessages.add(currentMessage);
			if (returnMessages.size() >= messageCount)
			{
				break;
			}

			currentMessage = this.pollPrefetchQueue();
		}
		
		return returnMessages;
	}

	public int getPrefetchCount()
	{
		synchronized (this.prefetchCountSync)
		{
			return this.prefetchCount;
		}
	}

	public void setPrefetchCount(final int value) throws ServiceBusException
	{
		final int deltaPrefetchCount;
		synchronized (this.prefetchCountSync)
		{
			deltaPrefetchCount = this.prefetchCount - value;
			this.prefetchCount = value;
		}
		
		try
		{
			this.underlyingFactory.scheduleOnReactorThread(new DispatchHandler()
			{
				@Override
				public void onEvent()
				{
					sendFlow(deltaPrefetchCount);
				}
			});
		}
		catch (IOException ioException)
		{
			throw new ServiceBusException(false, "Setting prefetch count failed, see cause for more details", ioException);
		}
	}

	public Duration getReceiveTimeout()
	{
		return this.receiveTimeout;
	}

	public void setReceiveTimeout(final Duration value)
	{
		this.receiveTimeout = value;
	}

	public CompletableFuture<Collection<Message>> receive(final int maxMessageCount)
	{
		this.throwIfClosed(this.lastKnownLinkError);

		if (maxMessageCount <= 0 || maxMessageCount > this.prefetchCount)
		{
			throw new IllegalArgumentException(String.format(Locale.US, "parameter 'maxMessageCount' should be a positive number and should be less than prefetchCount(%s)", this.prefetchCount));
		}

		if (this.pendingReceives.isEmpty())
		{
			this.scheduleOperationTimer(TimeoutTracker.create(this.receiveTimeout));
		}

		CompletableFuture<Collection<Message>> onReceive = new CompletableFuture<Collection<Message>>();
		
		try
		{
			this.underlyingFactory.scheduleOnReactorThread(new DispatchHandler()
			{
				@Override
				public void onEvent()
				{
					final List<Message> messages = receiveCore(maxMessageCount);
					if (messages != null)
						onReceive.complete(messages);
					else
						pendingReceives.offer(new ReceiveWorkItem(onReceive, receiveTimeout, maxMessageCount));

					// calls to reactor should precede enqueue of the workItem into PendingReceives.
					// This will allow error handling to enact on the enqueued workItem.
					if (receiveLink.getLocalState() == EndpointState.CLOSED || receiveLink.getRemoteState() == EndpointState.CLOSED)
					{
						createReceiveLink();
					}
				}
			});
		}
		catch (IOException ioException)
		{
			onReceive.completeExceptionally(
					new ServiceBusException(false, "Receive failed while dispatching to Reactor, see cause for more details.", ioException));
		}

		return onReceive;
	}

	public void onOpenComplete(Exception exception)
	{		
		if (exception == null)
		{
			if (this.linkOpen != null && !this.linkOpen.getWork().isDone())
			{
				this.linkOpen.getWork().complete(this);
			}

			this.lastKnownLinkError = null;

			// re-open link always starts from the latest received offset
			this.offsetInclusive = false;
			this.underlyingFactory.getRetryPolicy().resetRetryCount(this.underlyingFactory.getClientId());

			this.nextCreditToFlow = 0;
			this.sendFlow(this.prefetchCount - this.prefetchedMessages.size());

			if(TRACE_LOGGER.isLoggable(Level.FINE))
			{
				TRACE_LOGGER.log(Level.FINE, String.format("receiverPath[%s], linkname[%s], updated-link-credit[%s], sentCredits[%s]",
						this.receivePath, this.receiveLink.getName(), this.receiveLink.getCredit(), this.prefetchCount));
			}
		}
		else
		{
			if (this.linkOpen != null && !this.linkOpen.getWork().isDone())
			{
				this.setClosed();
				ExceptionUtil.completeExceptionally(this.linkOpen.getWork(), exception, this);
			}

			this.lastKnownLinkError = exception;
		}
	}

	@Override
	public void onReceiveComplete(Delivery delivery)
	{
		Message message = null;
		
		int msgSize = delivery.pending();
		byte[] buffer = new byte[msgSize];
		
		int read = receiveLink.recv(buffer, 0, msgSize);
		
		message = Proton.message();
		message.decode(buffer, 0, read);
		
		delivery.settle();

		this.prefetchedMessages.add(message);
		this.underlyingFactory.getRetryPolicy().resetRetryCount(this.getClientId());
		
		final ReceiveWorkItem currentReceive = this.pendingReceives.poll();
		if (currentReceive != null && !currentReceive.getWork().isDone())
		{
			List<Message> messages = this.receiveCore(currentReceive.maxMessageCount);

			CompletableFuture<Collection<Message>> future = currentReceive.getWork();
			future.complete(messages);
		}
	}

	public void onError(ErrorCondition error)
	{		
		Exception completionException = ExceptionUtil.toException(error);
		this.onError(completionException);
	}

	@Override
	public void onError(Exception exception)
	{
		this.prefetchedMessages.clear();

		if (this.getIsClosingOrClosed())
		{
			this.linkClose.complete(null);
			
			WorkItem<Collection<Message>> workItem = null;
			final boolean isTransientException = exception == null ||
					(exception instanceof ServiceBusException && ((ServiceBusException) exception).getIsTransient());
			while ((workItem = this.pendingReceives.poll()) != null)
			{
				CompletableFuture<Collection<Message>> future = workItem.getWork();
				if (isTransientException)
				{
					future.complete(null);
				}
				else
				{
					ExceptionUtil.completeExceptionally(future, exception, this);
				}
			}
		}
		else
		{
			this.lastKnownLinkError = exception;
			this.onOpenComplete(exception);
			
			final WorkItem<Collection<Message>> workItem = this.pendingReceives.peek();
			final Duration nextRetryInterval = workItem != null && workItem.getTimeoutTracker() != null
					? this.underlyingFactory.getRetryPolicy().getNextRetryInterval(this.getClientId(), exception, workItem.getTimeoutTracker().remaining())
					: null;
			if (nextRetryInterval != null)
			{
				try
				{
					this.underlyingFactory.scheduleOnReactorThread((int) nextRetryInterval.toMillis(), new DispatchHandler()
					{
						@Override
						public void onEvent()
						{
							if (receiveLink.getLocalState() == EndpointState.CLOSED || receiveLink.getRemoteState() == EndpointState.CLOSED)
							{
								createReceiveLink();
								underlyingFactory.getRetryPolicy().incrementRetryCount(getClientId());
							}
						}
					});
				}
				catch (IOException ignore)
				{
				}
			}
			else
			{
				WorkItem<Collection<Message>> pendingReceive = null;
				while ((pendingReceive = this.pendingReceives.poll()) != null)
				{
					ExceptionUtil.completeExceptionally(pendingReceive.getWork(), exception, this);
				}
			}
		}
	}

	private void scheduleOperationTimer(TimeoutTracker tracker)
	{
		if (tracker != null)
		{
			Timer.schedule(this.onOperationTimedout, tracker.remaining(), TimerType.OneTimeRun);
		}
	}

	private void createReceiveLink()
	{	
		Connection connection = this.underlyingFactory.getConnection();

		Source source = new Source();
		source.setAddress(receivePath);

		UnknownDescribedType filter = null;
		if (this.lastReceivedOffset == null)
		{
			long totalMilliSeconds;
			try
			{
				totalMilliSeconds = this.dateTime.toEpochMilli();
			}
			catch(ArithmeticException ex)
			{
				totalMilliSeconds = Long.MAX_VALUE;
				if(TRACE_LOGGER.isLoggable(Level.WARNING))
				{
					TRACE_LOGGER.log(Level.WARNING,
							String.format("receiverPath[%s], linkname[%s], warning[starting receiver from epoch+Long.Max]", this.receivePath, this.receiveLink.getName()));
				}
			}

			filter = new UnknownDescribedType(AmqpConstants.STRING_FILTER,
					String.format(AmqpConstants.AMQP_ANNOTATION_FORMAT, AmqpConstants.ENQUEUED_TIME_UTC_ANNOTATION_NAME, StringUtil.EMPTY, totalMilliSeconds));
		}
		else 
		{
			if(TRACE_LOGGER.isLoggable(Level.FINE))
			{
				TRACE_LOGGER.log(Level.FINE, String.format("receiverPath[%s], action[recreateReceiveLink], offset[%s], offsetInclusive[%s]", this.receivePath, this.lastReceivedOffset, this.offsetInclusive));
			}

			filter =  new UnknownDescribedType(AmqpConstants.STRING_FILTER,
					String.format(AmqpConstants.AMQP_ANNOTATION_FORMAT, AmqpConstants.OFFSET_ANNOTATION_NAME, this.offsetInclusive ? "=" : StringUtil.EMPTY, this.lastReceivedOffset));
		}

		final Map<Symbol, UnknownDescribedType> filterMap = Collections.singletonMap(AmqpConstants.STRING_FILTER, filter);
		source.setFilter(filterMap);

		final Session session = connection.session();
		session.setIncomingCapacity(Integer.MAX_VALUE);
		session.open();
		BaseHandler.setHandler(session, new SessionHandler(this.receivePath));

		final String receiveLinkNamePrefix = StringUtil.getRandomString();
		final String receiveLinkName = !StringUtil.isNullOrEmpty(connection.getRemoteContainer()) ? 
				receiveLinkNamePrefix.concat(TrackingUtil.TRACKING_ID_TOKEN_SEPARATOR).concat(connection.getRemoteContainer()) :
				receiveLinkNamePrefix;
		final Receiver receiver = session.receiver(receiveLinkName);
		receiver.setSource(source);
		receiver.setTarget(new Target());

		// use explicit settlement via dispositions (not pre-settled)
		receiver.setSenderSettleMode(SenderSettleMode.UNSETTLED);
		receiver.setReceiverSettleMode(ReceiverSettleMode.SECOND);

		if (this.isEpochReceiver)
		{
			receiver.setProperties(Collections.singletonMap(AmqpConstants.EPOCH, (Object) this.epoch));
		}

		final ReceiveLinkHandler handler = new ReceiveLinkHandler(this);
		BaseHandler.setHandler(receiver, handler);
		this.underlyingFactory.registerForConnectionError(receiver);

		receiver.open();

		if (this.receiveLink != null)
		{
			final Receiver oldReceiver = this.receiveLink;
			this.underlyingFactory.deregisterForConnectionError(oldReceiver);
		}

		this.receiveLink = receiver;
	}

	// CONTRACT: message should be delivered to the caller of MessageReceiver.receive() only via Poll on prefetchqueue
	private Message pollPrefetchQueue()
	{
		final Message message = this.prefetchedMessages.poll();
		if (message != null)
		{
			// message lastReceivedOffset should be up-to-date upon each poll - as recreateLink will depend on this 
			this.lastReceivedOffset = message.getMessageAnnotations().getValue().get(AmqpConstants.OFFSET).toString();
			this.sendFlow(1);
		}

		return message;
	}

	private void sendFlow(final int credits)
	{
		// slow down sending the flow - to make the protocol less-chat'y
		this.nextCreditToFlow += credits;
		if (this.nextCreditToFlow >= this.prefetchCount || this.nextCreditToFlow >= 100)
		{
			final int tempFlow = this.nextCreditToFlow;
			this.receiveLink.flow(tempFlow);
			this.nextCreditToFlow = 0;
			
			if(TRACE_LOGGER.isLoggable(Level.FINE))
			{
				TRACE_LOGGER.log(Level.FINE, String.format("receiverPath[%s], linkname[%s], updated-link-credit[%s], sentCredits[%s]",
						this.receivePath, this.receiveLink.getName(), this.receiveLink.getCredit(), tempFlow));
			}
		}
	}

	private void scheduleLinkOpenTimeout(final TimeoutTracker timeout)
	{
		// timer to signal a timeout if exceeds the operationTimeout on MessagingFactory
		Timer.schedule(
				new Runnable()
				{
					public void run()
					{
						if (!linkOpen.getWork().isDone())
						{
							Exception operationTimedout = new TimeoutException(
									String.format(Locale.US, "%s operation on ReceiveLink(%s) to path(%s) timed out at %s.", "Open", MessageReceiver.this.receiveLink.getName(), MessageReceiver.this.receivePath, ZonedDateTime.now()),
									MessageReceiver.this.lastKnownLinkError);
							if (TRACE_LOGGER.isLoggable(Level.WARNING))
							{
								TRACE_LOGGER.log(Level.WARNING, 
										String.format(Locale.US, "receiverPath[%s], linkName[%s], %s call timedout", MessageReceiver.this.receivePath, MessageReceiver.this.receiveLink.getName(),  "Open"), 
										operationTimedout);
							}

							ExceptionUtil.completeExceptionally(linkOpen.getWork(), operationTimedout, MessageReceiver.this);
						}
					}
				}
				, timeout.remaining()
				, TimerType.OneTimeRun);
	}

	private void scheduleLinkCloseTimeout(final TimeoutTracker timeout)
	{
		// timer to signal a timeout if exceeds the operationTimeout on MessagingFactory
		Timer.schedule(
				new Runnable()
				{
					public void run()
					{
						if (!linkClose.isDone())
						{
							Exception operationTimedout = new TimeoutException(String.format(Locale.US, "%s operation on Receive Link(%s) timed out at %s", "Close", MessageReceiver.this.receiveLink.getName(), ZonedDateTime.now()));
							if (TRACE_LOGGER.isLoggable(Level.WARNING))
							{
								TRACE_LOGGER.log(Level.WARNING, 
										String.format(Locale.US, "receiverPath[%s], linkName[%s], %s call timedout", MessageReceiver.this.receivePath, MessageReceiver.this.receiveLink.getName(), "Close"), 
										operationTimedout);
							}

							ExceptionUtil.completeExceptionally(linkClose, operationTimedout, MessageReceiver.this);
						}
					}
				}
				, timeout.remaining()
				, TimerType.OneTimeRun);
	}

	@Override
	public void onClose(ErrorCondition condition)
	{
		if (condition == null)
		{
			this.onError(new ServiceBusException(true, 
					String.format(Locale.US, "Closing the link. LinkName(%s), EntityPath(%s)", this.receiveLink.getName(), this.receivePath)));
		}
		else
		{
			this.onError(condition);
		}
	}

	@Override
	public ErrorContext getContext()
	{
		final boolean isLinkOpened = this.linkOpen != null && this.linkOpen.getWork().isDone();
		final String referenceId = this.receiveLink != null && this.receiveLink.getRemoteProperties() != null && this.receiveLink.getRemoteProperties().containsKey(ClientConstants.TRACKING_ID_PROPERTY)
				? this.receiveLink.getRemoteProperties().get(ClientConstants.TRACKING_ID_PROPERTY).toString()
						: ((this.receiveLink != null) ? this.receiveLink.getName(): null);

		ReceiverContext errorContext = new ReceiverContext(this.underlyingFactory != null ? this.underlyingFactory.getHostName() : null,
				this.receivePath,
				referenceId,
				(isLinkOpened && this.lastReceivedOffset != null) ? Long.parseLong(this.lastReceivedOffset) : null, 
						isLinkOpened ? this.prefetchCount : null, 
								isLinkOpened && this.receiveLink != null ? this.receiveLink.getCredit(): null, 
										isLinkOpened && this.prefetchedMessages != null ? this.prefetchedMessages.size(): null, 
												this.isEpochReceiver);

		return errorContext;
	}	

	private static class ReceiveWorkItem extends WorkItem<Collection<Message>>
	{
		private final int maxMessageCount;

		public ReceiveWorkItem(CompletableFuture<Collection<Message>> completableFuture, Duration timeout, final int maxMessageCount)
		{
			super(completableFuture, timeout);
			this.maxMessageCount = maxMessageCount;
		}
	}

	@Override
	protected CompletableFuture<Void> onClose()
	{
		if (!this.getIsClosed())
		{
			if (this.receiveLink != null && this.receiveLink.getLocalState() != EndpointState.CLOSED)
			{
				this.receiveLink.close();
				this.scheduleLinkCloseTimeout(TimeoutTracker.create(this.operationTimeout));
			}
			else if (this.receiveLink == null || this.receiveLink.getRemoteState() == EndpointState.CLOSED)
			{
				this.linkClose.complete(null);
			}
		}

		return this.linkClose;
	}
}
