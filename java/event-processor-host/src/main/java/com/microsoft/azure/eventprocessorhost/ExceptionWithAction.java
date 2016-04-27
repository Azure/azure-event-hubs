/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

class ExceptionWithAction extends Exception
{
	private static final long serialVersionUID = 7480590197418857145L;
	
	private String action = "";

	// Only instantiated to wrap an already existing exception with an action string
	public ExceptionWithAction(Throwable e, String action)
	{
		super(e);
		this.action = action;
	}
	
	public String getAction()
	{
		return this.action;
	}
}
