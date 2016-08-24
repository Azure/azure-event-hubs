package com.microsoft.azure.eventhubs.exceptioncontracts;

import java.util.concurrent.ExecutionException;
import org.junit.Assume;
import org.junit.Test;

import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.lib.TestBase;
import com.microsoft.azure.eventhubs.lib.TestEventHubInfo;
import com.microsoft.azure.servicebus.AuthorizationFailedException;
import com.microsoft.azure.servicebus.ConnectionStringBuilder;

public class SecurityExceptionsTest extends TestBase
{
	@Test (expected = AuthorizationFailedException.class)
	public void testEventHubClientUnAuthorizedAccess() throws Throwable
	{
		Assume.assumeTrue(TestBase.isTestConfigurationSet());
		
		TestEventHubInfo eventHubInfo = TestBase.checkoutTestEventHub();
		try {
			ConnectionStringBuilder connectionString = new ConnectionStringBuilder(eventHubInfo.getNamespaceName(), eventHubInfo.getName(), "random", "wrongvalue");
			
			try
			{
				EventHubClient.createFromConnectionString(connectionString.toString()).get();		
			}
			catch(ExecutionException exp) {
				throw exp.getCause();
			}
		}
		finally {
			TestBase.checkinTestEventHub(eventHubInfo.getName());
		}
	}
}