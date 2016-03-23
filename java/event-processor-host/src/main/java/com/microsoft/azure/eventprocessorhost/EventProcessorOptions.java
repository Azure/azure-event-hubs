/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;


public final class EventProcessorOptions
{
    private Boolean invokeProcessorAfterReceiveTimeout = false;
    private int maxBatchSize = 10;
    private int prefetchCount = 300;
    private int receiveTimeOutMilliseconds = 60000; // default to one minute

    public static EventProcessorOptions getDefaultOptions()
    {
        return new EventProcessorOptions();
    }

    public EventProcessorOptions()
    {
    }

    public Boolean getInvokeProcessorAfterReceiveTimeout()
    {
        return this.invokeProcessorAfterReceiveTimeout;
    }

    /*
     * EPH uses javaClient's receive handler support to get callbacks when messages arrive, instead of
     * implementing its own receive loop. JavaClient does not call the callback when a receive call
     * times out, so EPH cannot pass that timeout down to the user's onEvents handler. Unless javaClient's
     * behavior changes, this option must remain false because we cannot provide any other behavior.
    public void setInvokeProcessorAfterReceiveTimeout(Boolean invokeProcessorAfterReceiveTimeout)
    {
        this.invokeProcessorAfterReceiveTimeout = invokeProcessorAfterReceiveTimeout;
    }
    */

    public int getMaxBatchSize()
    {
        return this.maxBatchSize;
    }

    public void setMaxBatchSize(int maxBatchSize)
    {
        this.maxBatchSize = maxBatchSize;
    }

    public int getPrefetchCount()
    {
        return this.prefetchCount;
    }

    public void setPrefetchCount(int prefetchCount)
    {
        this.prefetchCount = prefetchCount;
    }

    public int getReceiveTimeOut()
    {
        return this.receiveTimeOutMilliseconds;
    }

    public void setReceiveTimeOut(int receiveTimeOutMilliseconds)
    {
        this.receiveTimeOutMilliseconds = receiveTimeOutMilliseconds;
    }
}
