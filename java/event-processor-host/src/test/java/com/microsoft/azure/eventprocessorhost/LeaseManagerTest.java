package com.microsoft.azure.eventprocessorhost;

import java.util.UUID;

import org.junit.Test;
import static org.junit.Assert.*;

public class LeaseManagerTest
{
	//
	// Setup variables.
	//
	private boolean useAzureStorage = false; // false tests InMemoryLeaseManager, true tests AzureStorageCheckpointLeaseManager
	//private String azureStorageConnectionString = ""; // must be set to valid connection string to test AzureStorageCheckpointLeaseManager
	private String azureStorageConnectionString = "DefaultEndpointsProtocol=https;AccountName=jbird4javaeph;AccountKey=GPA8M1kT2iYn1KSUioDNZMjp1r7K8trG8RIYL0U0bxcK91GgrSOMUxWuiP7CHpJYRC0e/jA6VLod4U5gCfPvsg=="; // must be set to valid connection string to test AzureStorageCheckpointLeaseManager
	private int partitionCount = 4;
	
	private ILeaseManager[] leaseManagers;
	private EventProcessorHost[] hosts;
	
	@Test
	public void singleManagerSmokeTest() throws Exception
	{
		this.leaseManagers = new ILeaseManager[1];
		this.hosts = new EventProcessorHost[1];
		setupOneManager(0);

		Boolean boolret = this.leaseManagers[0].leaseStoreExists().get();
		assertFalse("lease store should not exist yet", boolret);
		
		boolret = this.leaseManagers[0].createLeaseStoreIfNotExists().get();
		assertTrue("creating lease store returned false", boolret);

		boolret = this.leaseManagers[0].leaseStoreExists().get();
		assertTrue("lease store should exist but does not", boolret);
		
		Lease[] leases = new Lease[this.partitionCount];
		for (int i = 0; i < this.partitionCount; i++)
		{
			Lease createdLease = this.leaseManagers[0].createLeaseIfNotExists(String.valueOf(i)).get();
			leases[i] = createdLease;
			assertNotNull("failed creating lease for " + i, createdLease);
		}
	
		for (int i = 0; i < partitionCount; i++)
		{
			System.out.println("Partition " + i + " state before acquire: " + leases[i].getStateDebug());
			boolret = this.leaseManagers[0].acquireLease(leases[i]).get();
			assertTrue("failed to acquire lease for " + i, boolret);
			System.out.println("Partition " + i + " state after acquire: " + leases[i].getStateDebug());
		}
	
		Thread.sleep(5000);
		
		for (int i = 0; i < partitionCount; i++)
		{
			System.out.println("Partition " + i + " state before: " + leases[i].getStateDebug());
			boolret = this.leaseManagers[0].renewLease(leases[i]).get();
			assertTrue("failed to renew lease for " + i, boolret);
			System.out.println("Partition " + i + " state after: " + leases[i].getStateDebug());
		}
		
		int x = 1;
		while (!leases[0].isExpired())
		{
			Thread.sleep(5000);
			System.out.println("Still waiting for lease on 0 to expire: " + (5 * x));
			assertFalse("lease 0 expiration is overdue", (5000 * x) > (this.leaseManagers[0].getLeaseDurationInMilliseconds() + 10000));
			for (int i = 1; i < partitionCount; i++)
			{
				boolret = this.leaseManagers[0].renewLease(leases[i]).get();
				assertTrue("failed to renew lease for " + i, boolret);
			}
			x++;
		}
		
		leases[1].setEpoch(5);
		if (!this.useAzureStorage)
		{
			// AzureStorageCheckpointLeaseManager uses the token to manage Storage leases, only test when using InMemory
			leases[1].setToken("it's a cloudy day");
		}
		boolret = this.leaseManagers[0].updateLease(leases[1]).get();
		assertTrue("failed to update lease for 1", boolret);
		Lease retrievedLease = this.leaseManagers[0].getLease("1").get();
		assertNotNull("failed to get lease for 1", retrievedLease);
		assertEquals("epoch was not persisted, expected " + leases[1].getEpoch() + " got " + retrievedLease.getEpoch(), leases[1].getEpoch(), retrievedLease.getEpoch());
		if (!this.useAzureStorage)
		{
			assertEquals("token was not persisted, expected [" + leases[1].getToken() + "] got [" + retrievedLease.getToken() + "]", leases[1].getToken(), retrievedLease.getToken());
		}
	
		// Release for 0 is expected to fail because it has expired
		boolret = this.leaseManagers[0].releaseLease(leases[0]).get();
		assertFalse("release lease on 0 succeeded unexpectedly", boolret);
		for (int i = 1; i < partitionCount; i++)
		{
			System.out.println("Partition " + i + " state before: " + leases[i].getStateDebug());
			boolret = this.leaseManagers[0].releaseLease(leases[i]).get();
			assertTrue("failed to release lease for " + i, boolret);
			System.out.println("Partition " + i + " state after: " + leases[i].getStateDebug());
		}
	}
	
	private void setupOneManager(int index) throws Exception
	{
		ILeaseManager leaseMgr = null;
		ICheckpointManager checkpointMgr = null;
		
		if (!this.useAzureStorage)
		{
			leaseMgr = new InMemoryLeaseManager();
			checkpointMgr = new InMemoryCheckpointManager();
		}
		else
		{
			String containerName = "leasemgrtest-" + String.valueOf(index) + "-" + UUID.randomUUID().toString(); 
			System.out.println("Container name: " + containerName);
			AzureStorageCheckpointLeaseManager azMgr = new AzureStorageCheckpointLeaseManager(this.azureStorageConnectionString, containerName);
			leaseMgr = azMgr;
			checkpointMgr = azMgr;
		}
		
		String suffix = String.valueOf(index);
    	EventProcessorHost host = new EventProcessorHost("dummyHost" + suffix, "dummyNamespace" + suffix, "dummyEventHub" + suffix, "dummyKeyName" + suffix,
				"dummyKey" + suffix, "dummyConsumerGroup" + suffix, checkpointMgr, leaseMgr);
    	
    	try
    	{
    		if (!this.useAzureStorage)
    		{
    			((InMemoryLeaseManager)leaseMgr).initialize(host);
    			((InMemoryCheckpointManager)checkpointMgr).initialize(host);
    		}
    		else
    		{
    			((AzureStorageCheckpointLeaseManager)leaseMgr).initialize(host);
    		}
		}
    	catch (Exception e)
    	{
    		System.out.println("Manager initializion failed");
    		throw e;
		}
		
    	this.leaseManagers[index] = leaseMgr;
    	this.hosts[index] = host;
	}
}
