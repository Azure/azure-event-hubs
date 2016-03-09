package com.microsoft.azure.eventprocessorhost;

public class LeaseLostException extends Exception
{
	private Lease lease = null;
	
	// TODO is serialVersionUid needed?
	
	public LeaseLostException()
	{
		super();
	}
	
	public LeaseLostException(Lease lease)
	{
		super();
		this.lease = lease;
	}
	
	public LeaseLostException(Lease lease, Throwable cause)
	{
		super(null, cause);
		this.lease = lease;
	}
	
	public LeaseLostException(String message, Throwable cause)
	{
		super(message, cause);
	}
	
	public Lease getLease()
	{
		return this.lease;
	}
}
