/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.util.List;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLConnection;
import java.nio.charset.StandardCharsets;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.logging.Level;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import com.microsoft.azure.servicebus.ConnectionStringBuilder;
import com.microsoft.azure.servicebus.SharedAccessSignatureTokenProvider;


class PartitionManager implements Runnable
{
    private EventProcessorHost host;
    private Pump pump;

    private List<String> partitionIds = null;
    
    private boolean keepGoing = true;

    public PartitionManager(EventProcessorHost host)
    {
        this.host = host;
        this.pump = new Pump(this.host);
    }
    
    public <T extends PartitionPump> void setPumpClass(Class<T> pumpClass)
    {
    	this.pump.setPumpClass(pumpClass);
    }
    
    public Iterable<String> getPartitionIds()
    {
        if (this.partitionIds == null)
        {
        	try
        	{
	        	String contentEncoding = StandardCharsets.UTF_8.name();
	        	ConnectionStringBuilder connectionString = new ConnectionStringBuilder(this.host.getEventHubConnectionString());
	        	URI namespaceUri = new URI("https", connectionString.getEndpoint().getHost(), null, null);
	        	String resourcePath = String.join("/", namespaceUri.toString(), connectionString.getEntityPath());
	        	
	        	final String authorizationToken = SharedAccessSignatureTokenProvider.generateSharedAccessSignature(
	        			connectionString.getSasKeyName(), connectionString.getSasKey(), 
	        			resourcePath, Duration.ofMinutes(20));
	        	        	
	            URLConnection connection = new URL(resourcePath).openConnection();
	        	connection.addRequestProperty("Authorization", authorizationToken);
	        	connection.setRequestProperty("Content-Type", "application/atom+xml;type=entry");
	        	connection.setRequestProperty("charset", contentEncoding);
	        	InputStream responseStream = connection.getInputStream();
	        	
	        	DocumentBuilderFactory docFactory = DocumentBuilderFactory.newInstance();
	        	DocumentBuilder docBuilder = docFactory.newDocumentBuilder();
	        	Document doc = docBuilder.parse(responseStream);
	        	
	        	XPath xpath = XPathFactory.newInstance().newXPath();
	        	Node partitionIdsNode = ((NodeList) xpath.evaluate("//entry/content/EventHubDescription/PartitionIds", doc.getDocumentElement(), XPathConstants.NODESET)).item(0);
	        	NodeList partitionIdsNodes = partitionIdsNode.getChildNodes();
	        	
	        	this.partitionIds = new ArrayList<String>();
	            for (int partitionIndex = 0; partitionIndex < partitionIdsNodes.getLength(); partitionIndex++)
	        	{
	        		this.partitionIds.add(partitionIdsNodes.item(partitionIndex).getTextContent());    		
	        	}
        	}
        	catch(XPathExpressionException|ParserConfigurationException|IOException|InvalidKeyException|NoSuchAlgorithmException|URISyntaxException|SAXException exception)
        	{
        		throw new EPHConfigurationException("Encountered error while fetching the list of EventHub PartitionIds", exception);
        	}
            
            this.host.logWithHost(Level.INFO, "Eventhub " + this.host.getEventHubPath() + " count of partitions: " + this.partitionIds.size());
            for (String id : this.partitionIds)
            {
            	this.host.logWithHost(Level.FINE, "Found partition with id: " + id);
            }
        }
        
        return this.partitionIds;
    }
    
    public void stopPartitions()
    {
    	this.keepGoing = false;
    }
    
    public void run()
    {
    	try
    	{
    		initializeStores();
    		runLoop();
    		this.host.logWithHost(Level.INFO, "Partition manager main loop exited normally, shutting down");
    	}
    	catch (Exception e)
    	{
    		this.host.logWithHost(Level.SEVERE, "Exception, shutting down partition manager", e);
    	}
    	
    	// Cleanup
    	this.host.logWithHost(Level.INFO, "Shutting down all pumps");
    	Iterable<Future<?>> pumpRemovals = this.pump.removeAllPumps(CloseReason.Shutdown);
    	
    	// All of the shutdown threads have been launched, we can shut down the executor now.
    	// Shutting down the executor only prevents new tasks from being submitted.
    	// We can't wait for executor termination here because this thread is in the executor.
    	this.host.stopExecutor();
    	
    	// Wait for shutdown threads.
    	for (Future<?> removal : pumpRemovals)
    	{
    		try
    		{
				removal.get();
			}
    		catch (InterruptedException | ExecutionException e)
    		{
    			this.host.logWithHost(Level.SEVERE, "Failure during shutdown", e);
    			// By convention, bail immediately on interrupt, even though we're just cleaning
    			// up on the way out. Fortunately, we ARE just cleaning up on the way out, so we're
    			// free to bail without serious side effects.
    			if (e instanceof InterruptedException)
    			{
    				Thread.currentThread().interrupt();
    				throw new RuntimeException(e);
    			}
			}
    	}
    	
    	this.host.logWithHost(Level.INFO, "Partition manager exiting");
    }
    
    private void initializeStores() throws InterruptedException, ExecutionException
    {
        ILeaseManager leaseManager = this.host.getLeaseManager();
        
        // Make sure the lease store exists
        if (!leaseManager.leaseStoreExists().get())
        {
            if (!leaseManager.createLeaseStoreIfNotExists().get())
            {
                throw new RuntimeException("Creating lease store returned false");
            }
        }
        // else
        //	lease store already exists, no work needed
        
        // Now make sure the leases exist
        for (String id : getPartitionIds())
        {
        	boolean createdOK = false;
        	int retryCount = 0;
        	do
        	{
	        	try
	        	{
	                leaseManager.createLeaseIfNotExists(id).get();
	                createdOK = true;
	        	}
	        	catch (ExecutionException e)
	        	{
	        		this.host.logWithHostAndPartition(Level.SEVERE, id, "Failure creating lease for this partition, retrying", e);
	        		retryCount++;
	        	}
        	} while (!createdOK && (retryCount < 5));
        	if (!createdOK)
        	{
        		this.host.logWithHostAndPartition(Level.SEVERE, id, "Out of retries creating lease for this partition");
        		throw new RuntimeException("Out of retries creating lease blob for partition " + id);
        	}
        }
        
        ICheckpointManager checkpointManager = this.host.getCheckpointManager();
        
        // Make sure the checkpoint store exists
        if (!checkpointManager.checkpointStoreExists().get())
        {
        	if (!checkpointManager.createCheckpointStoreIfNotExists().get())
        	{
        		throw new RuntimeException("Creating checkpoint store returned false");
        	}
        }
        // else
        //	checkpoint store already exists, no work needed
        
        // Now make sure the checkpoints exist
        for (String id : getPartitionIds())
        {
        	boolean createdOK = false;
        	int retryCount = 0;
        	do
        	{
	        	try
	        	{
	                checkpointManager.createCheckpointIfNotExists(id).get();
	                createdOK = true;
	        	}
	        	catch (ExecutionException e)
	        	{
	        		this.host.logWithHostAndPartition(Level.SEVERE, id, "Failure creating checkpoint for this partition, skipping", e);
	        		retryCount++;
	        	}
        	} while (!createdOK && (retryCount < 5));
        	if (!createdOK)
        	{
        		this.host.logWithHostAndPartition(Level.SEVERE, id, "Out of retries creating checkpoint for this partition");
        		throw new RuntimeException("Out of retries creating checkpoint blob for partition " + id);
        	}
        }
    }
    
    private void runLoop() throws Exception
    {
    	while (this.keepGoing)
    	{
            ILeaseManager leaseManager = this.host.getLeaseManager();
            HashMap<String, Lease> allLeases = new HashMap<String, Lease>();

            // Inspect all leases.
            // Acquire any expired leases.
            // Renew any leases that currently belong to us.
            Iterable<Future<Lease>> gettingAllLeases = leaseManager.getAllLeases();
            ArrayList<Lease> leasesOwnedByOthers = new ArrayList<Lease>();
            int ourLeasesCount = 0;
            for (Future<Lease> future : gettingAllLeases)
            {
            	try
            	{
                    Lease possibleLease = future.get();
                    if (possibleLease.isExpired())
                    {
                    	if (leaseManager.acquireLease(possibleLease).get())
                    	{
                    		allLeases.put(possibleLease.getPartitionId(), possibleLease);
                    	}
                    }
                    else if (possibleLease.getOwner().compareTo(this.host.getHostName()) == 0)
                    {
                        if (leaseManager.renewLease(possibleLease).get())
                        {
                            allLeases.put(possibleLease.getPartitionId(), possibleLease);
                            ourLeasesCount++;
                        }
                    }
                    else
                    {
                    	allLeases.put(possibleLease.getPartitionId(), possibleLease);
                    	leasesOwnedByOthers.add(possibleLease);
                    }
            	}
            	catch (ExecutionException e)
            	{
            		this.host.logWithHost(Level.WARNING, "Failure getting/acquiring/renewing lease, skipping", e);
            	}
            }
            
            // Grab more leases if available and needed for load balancing
            if (leasesOwnedByOthers.size() > 0)
            {
	            Iterable<Lease> stealTheseLeases = whichLeasesToSteal(leasesOwnedByOthers, ourLeasesCount);
	            if (stealTheseLeases != null)
	            {
	            	for (Lease stealee : stealTheseLeases)
	            	{
	            		try
	            		{
    	                	if (leaseManager.acquireLease(stealee).get())
    	                	{
    	                		this.host.logWithHostAndPartition(Level.INFO, stealee.getPartitionId(), "Stole lease");
    	                		allLeases.put(stealee.getPartitionId(), stealee);
    	                		ourLeasesCount++;
    	                	}
    	                	else
    	                	{
    	                		this.host.logWithHost(Level.WARNING, "Failed to steal lease for partition " + stealee.getPartitionId());
    	                	}
	            		}
	            		catch (ExecutionException e)
	            		{
	            			this.host.logWithHost(Level.SEVERE, "Exception stealing lease for partition " + stealee.getPartitionId(), e);
	            		}
	            	}
	            }
            }

            // Update pump with new state of leases.
            for (String partitionId : allLeases.keySet())
            {
            	Lease updatedLease = allLeases.get(partitionId);
            	this.host.logWithHost(Level.FINE, "Lease on partition " + updatedLease.getPartitionId() + " owned by " + updatedLease.getOwner()); // DEBUG
            	if (updatedLease.getOwner().compareTo(this.host.getHostName()) == 0)
            	{
            		this.pump.addPump(partitionId, updatedLease);
            	}
            	else
            	{
            		this.pump.removePump(partitionId, CloseReason.LeaseLost);
            	}
            }
    		
            try
            {
                Thread.sleep(leaseManager.getLeaseRenewIntervalInMilliseconds());
            }
            catch (InterruptedException e)
            {
            	// Bail on the thread if we are interrupted.
                this.host.logWithHost(Level.WARNING, "Sleep was interrupted", e);
                this.keepGoing = false;
				Thread.currentThread().interrupt();
				throw new RuntimeException(e);
            }
    	}
    }
    
    private Iterable<Lease> whichLeasesToSteal(ArrayList<Lease> stealableLeases, int haveLeaseCount)
    {
    	HashMap<String, Integer> countsByOwner = countLeasesByOwner(stealableLeases);
    	int totalLeases = stealableLeases.size() + haveLeaseCount;
    	int hostCount = countsByOwner.size() + 1;
    	int desiredToHave = (totalLeases + hostCount - 1) / hostCount; // round up
    	ArrayList<Lease> stealTheseLeases = null;
    	if (((desiredToHave - haveLeaseCount) == 1) && (haveLeaseCount > 0))
    	{
    		this.host.logWithHost(Level.FINE, "Have only one less than desired, skipping lease stealing");
    	}
    	else if (haveLeaseCount < desiredToHave)
    	{
    		this.host.logWithHost(Level.FINE, "Has " + haveLeaseCount + " leases, wants " + desiredToHave);
    		stealTheseLeases = new ArrayList<Lease>();
    		String stealFrom = findBiggestOwner(countsByOwner);
    		this.host.logWithHost(Level.FINE, "Proposed to steal leases from " + stealFrom);
    		for (Lease l : stealableLeases)
    		{
    			if (l.getOwner().compareTo(stealFrom) == 0)
    			{
    				stealTheseLeases.add(l);
    				this.host.logWithHost(Level.FINE, "Proposed to steal lease for partition " + l.getPartitionId());
    				haveLeaseCount++;
    				if (haveLeaseCount >= desiredToHave)
    				{
    					break;
    				}
    			}
    		}
    	}
    	return stealTheseLeases;
    }
    
    private String findBiggestOwner(HashMap<String, Integer> countsByOwner)
    {
    	int biggestCount = 0;
    	String biggestOwner = null;
    	for (String owner : countsByOwner.keySet())
    	{
    		if (countsByOwner.get(owner) > biggestCount)
    		{
    			biggestCount = countsByOwner.get(owner);
    			biggestOwner = owner;
    		}
    	}
    	return biggestOwner;
    }
    
    private HashMap<String, Integer> countLeasesByOwner(Iterable<Lease> leases)
    {
    	HashMap<String, Integer> counts = new HashMap<String, Integer>();
    	for (Lease l : leases)
    	{
    		if (counts.containsKey(l.getOwner()))
    		{
    			Integer oldCount = counts.get(l.getOwner());
    			counts.put(l.getOwner(), oldCount + 1);
    		}
    		else
    		{
    			counts.put(l.getOwner(), 1);
    		}
    	}
    	for (String owner : counts.keySet())
    	{
    		this.host.log(Level.FINE, "host " + owner + " owns " + counts.get(owner) + " leases");
    	}
    	this.host.log(Level.FINE, "total hosts in sorted list: " + counts.size());
    	
    	return counts;
    }
}
