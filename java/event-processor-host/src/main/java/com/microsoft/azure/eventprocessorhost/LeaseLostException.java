/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

public class LeaseLostException extends Exception
{
	private static final long serialVersionUID = -4625001822439809869L;
	
	private Lease lease = null;
	
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
