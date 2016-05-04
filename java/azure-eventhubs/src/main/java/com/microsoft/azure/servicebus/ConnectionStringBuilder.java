/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.servicebus;

import java.net.*;
import java.time.*;
import java.time.format.DateTimeParseException;
import java.util.*;
import java.util.regex.*;
import com.microsoft.azure.eventhubs.*;

/**
 * {@link ConnectionStringBuilder} can be used to construct a connection string which can establish communication with ServiceBus entities.
 * It can also be used to perform basic validation on an existing connection string.
 * <p> Sample Code:
 * <pre>{@code
 * 	ConnectionStringBuilder connectionStringBuilder = new ConnectionStringBuilder(
 *     "ServiceBusNamespaceName", 
 *     "ServiceBusEntityName", // eventHubName or QueueName or TopicName 
 *     "SharedAccessSignatureKeyName", 
 *     "SharedAccessSignatureKey");
 *  
 * String connectionString = connectionStringBuilder.toString();
 * }</pre>
 * <p>
 * A connection string is basically a string consisted of key-value pair separated by ";". 
 * Basic format is {{@literal <}key{@literal >}={@literal <}value{@literal >}[;{@literal <}key{@literal >}={@literal <}value{@literal >}]} where supported key name are as follow:
 * <ul>
 * <li> Endpoint - the URL that contains the servicebus namespace
 * <li> EntityPath - the path to the service bus entity (queue/topic/eventhub/subscription/consumergroup/partition)
 * <li> SharedAccessKeyName - the key name to the corresponding shared access policy rule for the namespace, or entity.
 * <li> SharedAccessKey - the key for the corresponding shared access policy rule of the namespace or entity.
 * </ul>
 */
public class ConnectionStringBuilder
{
	final static String endpointFormat = "amqps://%s.servicebus.windows.net";
	
	final static String EndpointConfigName = "Endpoint";
	final static String SharedAccessKeyNameConfigName = "SharedAccessKeyName";
	final static String SharedAccessKeyConfigName = "SharedAccessKey";
	final static String EntityPathConfigName = "EntityPath";
	final static String OperationTimeoutConfigName = "OperationTimeout";
	final static String KeyValueSeparator = "=";
	final static String KeyValuePairDelimiter = ";";

	private static final String AllKeyEnumerateRegex = "(" + EndpointConfigName + "|" + SharedAccessKeyNameConfigName
			+ "|" + SharedAccessKeyConfigName + "|" + EntityPathConfigName + "|" + OperationTimeoutConfigName + ")";

	private static final String KeysWithDelimitersRegex = KeyValuePairDelimiter + AllKeyEnumerateRegex
			+ KeyValueSeparator;

	private String connectionString;
	private URI endpoint;
	private String sharedAccessKeyName;
	private String sharedAccessKey;
	private String entityPath;
	private Duration operationTimeout;
	private RetryPolicy retryPolicy;

	private ConnectionStringBuilder(final String namespaceName, final String entityPath, final String sharedAccessKeyName,
			final String sharedAccessKey, final Duration operationTimeout, final RetryPolicy retryPolicy)
	{
		try
		{
			this.endpoint = new URI(String.format(Locale.US, endpointFormat, namespaceName));
		} 
		catch(URISyntaxException exception)
		{
			throw new IllegalConnectionStringFormatException(
					String.format(Locale.US, "Invalid namespace name: %s", namespaceName),
					exception);
		}
		
		this.sharedAccessKey = sharedAccessKey;
		this.sharedAccessKeyName = sharedAccessKeyName;
		this.operationTimeout = operationTimeout;
		this.retryPolicy = retryPolicy;
		this.entityPath = entityPath;
	}

	/**
	 * Build a connection string consumable by {@link EventHubClient#createFromConnectionString(String)}
	 * @param namespaceName Namespace name (dns suffix - ex: .servicebus.windows.net is not required) 
	 * @param entityPath Entity path. For eventHubs case specify - eventHub name.
	 * @param sharedAccessKeyName Shared Access Key name
	 * @param sharedAccessKey Shared Access Key
	 */
	public ConnectionStringBuilder(final String namespaceName, final String entityPath, final String sharedAccessKeyName,
			final String sharedAccessKey)
	{
		this(namespaceName, entityPath, sharedAccessKeyName, sharedAccessKey, MessagingFactory.DefaultOperationTimeout, RetryPolicy.getDefault());
	}

	/**
	 * ConnectionString format:
	 * 		Endpoint=sb://namespace_DNS_Name;EntityPath=EVENT_HUB_NAME;SharedAccessKeyName=SHARED_ACCESS_KEY_NAME;SharedAccessKey=SHARED_ACCESS_KEY
	 * @param connectionString ServiceBus ConnectionString
	 * @throws IllegalConnectionStringFormatException when the format of the ConnectionString is not valid
	 */
	public ConnectionStringBuilder(String connectionString)
	{
		this.parseConnectionString(connectionString);
		this.connectionString = connectionString;
	}
	
	/**
	 * Get the endpoint which can be used to connect to the ServiceBus Namespace
	 * @return Endpoint
	 */
	public URI getEndpoint()
	{
		return this.endpoint;
	}

	/**
	 * Get the shared access policy key value from the connection string
	 * @return Shared Access Signature key
	 */
	public String getSasKey()
	{
		return this.sharedAccessKey;
	}

	/**
	 * Get the shared access policy owner name from the connection string
	 * @return Shared Access Signature key name.
	 */
	public String getSasKeyName()
	{
		return this.sharedAccessKeyName;
	}
	
	/**
	 * Get the entity path value from the connection string
	 * @return Entity Path
	 */
	public String getEntityPath()
	{
		return this.entityPath;
	}
	
	/**
	 * OperationTimeout is applied in erroneous situations to notify the caller about the relevant {@link ServiceBusException}
	 * @return operationTimeout
	 */
	public Duration getOperationTimeout()
	{
		return (this.operationTimeout == null ? MessagingFactory.DefaultOperationTimeout : this.operationTimeout);
	}
	
	/**
	 * Set the OperationTimeout value in the Connection String. This value will be used by all operations which uses this {@link ConnectionStringBuilder}, unless explicitly over-ridden.
	 * @param operationTimeout Operation Timeout
	 */
	public void setOperationTimeout(final Duration operationTimeout)
	{
		this.operationTimeout = operationTimeout;
	}
	
	/**
	 * Get the retry policy instance that was created as part of this builder's creation.
	 * @return RetryPolicy applied for any operation performed using this ConnectionString
	 */
	public RetryPolicy getRetryPolicy()
	{
		return (this.retryPolicy == null ? RetryPolicy.getDefault() : this.retryPolicy);
	}

	/**
	 * Returns an inter-operable connection string that can be used to connect to ServiceBus Namespace
	 * @return connection string
	 */
	@Override
	public String toString()
	{
		if (StringUtil.isNullOrWhiteSpace(this.connectionString))
		{
			StringBuilder connectionStringBuilder = new StringBuilder();
			if (this.endpoint != null)
			{
				connectionStringBuilder.append(String.format(Locale.US, "%s%s%s%s", EndpointConfigName, KeyValueSeparator,
					this.endpoint.toString(), KeyValuePairDelimiter));
			}
			
			if (!StringUtil.isNullOrWhiteSpace(this.entityPath))
			{
				connectionStringBuilder.append(String.format(Locale.US, "%s%s%s%s", EntityPathConfigName,
					KeyValueSeparator, this.entityPath, KeyValuePairDelimiter));
			}
			
			if (!StringUtil.isNullOrWhiteSpace(this.sharedAccessKeyName))
			{
				connectionStringBuilder.append(String.format(Locale.US, "%s%s%s%s", SharedAccessKeyNameConfigName,
					KeyValueSeparator, this.sharedAccessKeyName, KeyValuePairDelimiter));
			}
			
			if (!StringUtil.isNullOrWhiteSpace(this.sharedAccessKey))
			{
				connectionStringBuilder.append(String.format(Locale.US, "%s%s%s", SharedAccessKeyConfigName,
					KeyValueSeparator, this.sharedAccessKey));
			}
			
			if (this.operationTimeout != null)
			{
				connectionStringBuilder.append(String.format(Locale.US, "%s%s%s%s", KeyValuePairDelimiter, OperationTimeoutConfigName,
						KeyValueSeparator, this.operationTimeout.toString()));
			}
			
			this.connectionString = connectionStringBuilder.toString();
		}

		return this.connectionString;
	}

	private void parseConnectionString(String connectionString)
	{
		// TODO: Trace and throw
		if (StringUtil.isNullOrWhiteSpace(connectionString))
		{
			throw new IllegalConnectionStringFormatException(String.format("connectionString cannot be empty"));
		}

		String connection = KeyValuePairDelimiter + connectionString;

		Pattern keyValuePattern = Pattern.compile(KeysWithDelimitersRegex, Pattern.CASE_INSENSITIVE);
		String[] values = keyValuePattern.split(connection);
		Matcher keys = keyValuePattern.matcher(connection);

		if (values == null || values.length <= 1 || keys.groupCount() == 0)
		{
			throw new IllegalConnectionStringFormatException("Connection String cannot be parsed.");
		}

		if (!StringUtil.isNullOrWhiteSpace((values[0])))
		{
			throw new IllegalConnectionStringFormatException(
					String.format(Locale.US, "Cannot parse part of ConnectionString: %s", values[0]));
		}

		int valueIndex = 0;
		while (keys.find())
		{
			valueIndex++;

			String key = keys.group();
			key = key.substring(1, key.length() - 1);
			
			if (values.length < valueIndex + 1)
			{
				throw new IllegalConnectionStringFormatException(
						String.format(Locale.US, "Value for the connection string parameter name: %s, not found", key));
			}
			
			if (key.equalsIgnoreCase(EndpointConfigName))
			{
				try
				{
					this.endpoint = new URI(values[valueIndex]); 
				}
				catch(URISyntaxException exception)
				{
					throw new IllegalConnectionStringFormatException(
							String.format(Locale.US, "%s should be in format scheme://fullyQualifiedServiceBusNamespaceEndpointName", EndpointConfigName),
							exception);
				}
			}
			else if(key.equalsIgnoreCase(SharedAccessKeyNameConfigName))
			{
				this.sharedAccessKeyName = values[valueIndex];
			}
			else if(key.equalsIgnoreCase(SharedAccessKeyConfigName))
			{
				this.sharedAccessKey = values[valueIndex];
			}
			else if (key.equalsIgnoreCase(EntityPathConfigName))
			{
				this.entityPath = values[valueIndex];
			}
			else if (key.equalsIgnoreCase(OperationTimeoutConfigName))
			{
				try
				{
					this.operationTimeout = Duration.parse(values[valueIndex]);
				}
				catch(DateTimeParseException exception)
				{
					throw new IllegalConnectionStringFormatException("Invalid value specified for property 'Duration' in the ConnectionString.", exception);
				}
			}
			else
			{
				throw new IllegalConnectionStringFormatException(
						String.format(Locale.US, "Illegal connection string parameter name: %s", key));
			}
		}
	}
}
