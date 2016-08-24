package com.microsoft.azure.eventhubs.exceptioncontracts;

import java.io.IOException;
import java.util.Collection;
import java.util.logging.Level;

import org.apache.qpid.proton.amqp.messaging.Source;
import org.apache.qpid.proton.amqp.transport.ErrorCondition;
import org.apache.qpid.proton.amqp.transport.Target;
import org.apache.qpid.proton.engine.EndpointState;
import org.apache.qpid.proton.engine.Event;
import org.apache.qpid.proton.engine.Link;
import org.apache.qpid.proton.message.Message;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import com.microsoft.azure.eventhubs.lib.TestBase;
import com.microsoft.azure.eventhubs.lib.Mock.MockServer;
import com.microsoft.azure.eventhubs.lib.Mock.Sender1MsgOnLinkFlowHandler;
import com.microsoft.azure.servicebus.ClientConstants;
import com.microsoft.azure.servicebus.ConnectionStringBuilder;
import com.microsoft.azure.servicebus.MessageReceiver;
import com.microsoft.azure.servicebus.MessagingFactory;

public class ReceiverRetryTest extends TestBase
{
	Sender1MsgOnLinkFlowHandler recvFlowHandler;
	TestData data;
	MockServer server;
	MessagingFactory factory;
	
	@Before
	public void initializeMockServer()  throws Exception
	{
		data = new TestData();
    	data.retryCount = 0;
    	
    	// simulating error case in servicebus service:
    	// first flow --> Receiver.open succeeds.
    	// after that this will continuously throw InternalError
    	recvFlowHandler = new Sender1MsgOnLinkFlowHandler()
    	{
			boolean firstRequest = true;
			
	    	// simulate open behavior of servicebus service
	    	@Override
	        public void onLinkRemoteOpen(Event event)
	    	{
	    		TestBase.TEST_LOGGER.log(Level.FINE, "onLinkRemoteOpen");
				Link link = event.getLink();
	            if (link.getLocalState() == EndpointState.UNINITIALIZED)
	            {
	                if (link.getRemoteTarget() != null)
	                {
	                    Target remoteTarget = link.getRemoteTarget();
	                    Source localSource = new Source();
	                    localSource.setAddress(remoteTarget.getAddress());
	                    link.setSource(localSource);
	                }
	            }

	            link.open();
	        }
	    	
			@Override
			public void onLinkFlow(Event event)
			{
				super.onLinkFlow(event);
				TestBase.TEST_LOGGER.log(Level.FINE, "onLinkFlow");
				if (firstRequest)
				{
					this.firstRequest = false;
				}
				else 
				{
					Link link = event.getLink();
					if (link.getLocalState()== EndpointState.ACTIVE)
					{
						link.setCondition(new ErrorCondition(ClientConstants.SERVER_BUSY_ERROR, "SimulateInternalError"));
						data.retryCount++;
						link.detach();
						link.close();
					}
				}
			}
		};
		
		server = MockServer.Create(recvFlowHandler);
	}
	
	public void testRetryWhenReceiveFails() throws Exception
	{
		factory = MessagingFactory.createFromConnectionString(
				new ConnectionStringBuilder("Endpoint=amqps://localhost;SharedAccessKeyName=somename;EntityPath=eventhub1;SharedAccessKey=somekey").toString()).get();
		
		MessageReceiver receiver = MessageReceiver.create(factory, 
					"receiver1", "eventhub1/consumergroups/$default/partitions/0", "-1", false, null, 100, 0, false).get();
		Collection<Message> messages = receiver.receive(10).get();
		if (messages != null)
		{
			receiver.receive(10).get();
		}
		
		TestBase.TEST_LOGGER.log(Level.FINE, String.format("actual retries: %s", data.retryCount));
		Assert.assertTrue(data.retryCount > 3);
	}
	
	@After
	public void cleanup() throws IOException
	{
		if (factory != null)
			factory.close();
	
		if (server != null)
			server.close();
	}

	public class TestData
	{
		public int retryCount;
	}
}
