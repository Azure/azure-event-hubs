package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.ExecutorService;


// Implementations of ICheckpointManager and ILeaseManager must also implement this.
// These methods would be common to both interfaces and are only required once for
// a combined manager.
public interface IManagerBase
{
    // These methods are called by EventProcessorHost to set up the manager.
    // If your implementation would like to use different values, you can
    // implement these as no-ops.
    public void setEventHubPath(String eventHubPath);
    public void setConsumerGroupName(String consumerGroup);
    public void setExecutorService(ExecutorService executorService);

    // Returns true for combined managers.
    public Boolean isCombinedManager();
}
