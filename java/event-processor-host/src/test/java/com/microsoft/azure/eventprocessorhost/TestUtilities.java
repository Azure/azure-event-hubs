package com.microsoft.azure.eventprocessorhost;

class TestUtilities
{
	static String getStorageConnectionString()
	{
		String retval = System.getenv("EPHTESTSTORAGE");
		return ((retval != null) ? retval : "");
	}
}
