package com.microsoft.azure.eventhubs.eventdata;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.time.Instant;
import java.util.HashMap;
import java.util.concurrent.ExecutionException;

import org.apache.qpid.proton.Proton;
import org.apache.qpid.proton.amqp.Binary;
import org.apache.qpid.proton.amqp.Symbol;
import org.apache.qpid.proton.amqp.messaging.ApplicationProperties;
import org.apache.qpid.proton.amqp.messaging.Data;
import org.apache.qpid.proton.amqp.messaging.MessageAnnotations;
import org.apache.qpid.proton.message.Message;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.PartitionReceiver;
import com.microsoft.azure.eventhubs.lib.ApiTestBase;
import com.microsoft.azure.eventhubs.lib.TestContext;
import com.microsoft.azure.servicebus.ConnectionStringBuilder;
import com.microsoft.azure.servicebus.MessageSender;
import com.microsoft.azure.servicebus.MessagingFactory;
import com.microsoft.azure.servicebus.ServiceBusException;
import com.microsoft.azure.servicebus.amqp.AmqpConstants;

public class InteropTest extends ApiTestBase
{
	static EventHubClient ehClient;
	static MessagingFactory msgFactory;
	static PartitionReceiver receiver;
	static MessageSender partitionSender;

	static String partitionId = "0";

	@BeforeClass
	public static void initialize() throws ServiceBusException, IOException, InterruptedException, ExecutionException
	{
		final ConnectionStringBuilder connStrBuilder = TestContext.getConnectionString();
		final String connectionString = connStrBuilder.toString();

		ehClient = EventHubClient.createFromConnectionStringSync(connectionString);
		msgFactory = MessagingFactory.createFromConnectionString(connectionString).get();
		receiver = ehClient.createReceiverSync(TestContext.getConsumerGroupName(), partitionId, Instant.now());
		partitionSender = MessageSender.create(msgFactory, "link1", connStrBuilder.getEntityPath() + "/partitions/" + partitionId).get();
	}

	@Test
	public void interopWithDirectProtonAmqpMessage() throws ServiceBusException, InterruptedException, ExecutionException
	{
		final Message protonMessage = Proton.message();
		
		final String applicationProperty = "firstProp";
		final HashMap<String, String> appProperties = new HashMap<String, String>();
		appProperties.put(applicationProperty, "value1");
		final ApplicationProperties applicationProperties = new ApplicationProperties(appProperties);
		protonMessage.setApplicationProperties(applicationProperties);
		
		protonMessage.setMessageId("id1");
		protonMessage.setUserId("user1".getBytes());
		protonMessage.setAddress("eventhub1");
		protonMessage.setSubject("sub");
		protonMessage.setReplyTo("replyingTo");
		protonMessage.setExpiryTime(456L);
		protonMessage.setGroupSequence(5555L);
		protonMessage.setContentType("events");
		protonMessage.setContentEncoding("UTF-8");
		protonMessage.setCorrelationId("corid1");
		protonMessage.setCreationTime(345L);
		protonMessage.setGroupId("gid");
		protonMessage.setReplyToGroupId("replyToGroupId");
		
		final String msgAnnotation = "message-annotation-1";
		protonMessage.setMessageAnnotations(new MessageAnnotations(new HashMap<Symbol, Object>()));
		protonMessage.getMessageAnnotations().getValue().put(Symbol.getSymbol(msgAnnotation), "messageAnnotationValue");
		
		final String payload = "testmsg";
		protonMessage.setBody(new Data(Binary.create(ByteBuffer.wrap(payload.getBytes()))));
		
		partitionSender.send(protonMessage).get();
		
		EventData receivedEvent = receiver.receiveSync(10).iterator().next();
		
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_MESSAGE_ID)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_MESSAGE_ID).equals(protonMessage.getMessageId()));
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_USER_ID)
				&& new String((byte[]) receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_USER_ID)).equals(new String(protonMessage.getUserId())));		
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_TO)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_TO).equals(protonMessage.getAddress()));		
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_CONTENT_TYPE)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_CONTENT_TYPE).equals(protonMessage.getContentType()));
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_CONTENT_ENCODING)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_CONTENT_ENCODING).equals(protonMessage.getContentEncoding()));
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_CORRELATION_ID)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_CORRELATION_ID).equals(protonMessage.getCorrelationId()));
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_CREATION_TIME)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_CREATION_TIME).equals(protonMessage.getCreationTime()));
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_SUBJECT)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_SUBJECT).equals(protonMessage.getSubject()));
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_GROUP_ID)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_GROUP_ID).equals(protonMessage.getGroupId()));
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_REPLY_TO_GROUP_ID)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_REPLY_TO_GROUP_ID).equals(protonMessage.getReplyToGroupId()));
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_REPLY_TO)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_REPLY_TO).equals(protonMessage.getReplyTo()));
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(AmqpConstants.AMQP_PROPERTY_ABSOLUTE_EXPRITY_time)
				&& receivedEvent.getSystemProperties().get(AmqpConstants.AMQP_PROPERTY_ABSOLUTE_EXPRITY_time).equals(protonMessage.getExpiryTime()));
		
		
		Assert.assertTrue(receivedEvent.getSystemProperties().containsKey(msgAnnotation)
				&& receivedEvent.getSystemProperties().get(msgAnnotation).equals(protonMessage.getMessageAnnotations().getValue().get(Symbol.getSymbol(msgAnnotation))));
		
		Assert.assertTrue(receivedEvent.getProperties().containsKey(applicationProperty)
				&& receivedEvent.getProperties().get(applicationProperty).equals(protonMessage.getApplicationProperties().getValue().get(applicationProperty)));
		
		Assert.assertTrue(new String(receivedEvent.getBody()).equals(payload));
	}
	
	@AfterClass
	public static void cleanup() throws ServiceBusException
	{
		if (partitionSender != null)
			partitionSender.closeSync();
		
		if (receiver != null)
			receiver.closeSync();
		
		if (ehClient != null)
			ehClient.closeSync();
		
		if (msgFactory != null)
			msgFactory.closeSync();
	}
}
