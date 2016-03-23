package com.microsoft.azure.eventprocessorhost;

public class EPHConfigurationException extends RuntimeException
{
	private static final long serialVersionUID = 1L;

	EPHConfigurationException()
	{
		super();
	}

	EPHConfigurationException(String message, Exception cause)
	{
		super(message, cause);
	}
	
	EPHConfigurationException(final String message)
	{
		super(message);
	}

	EPHConfigurationException(final Throwable cause)
	{
		super(cause);
	}
}
