/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

package com.microsoft.azure.eventprocessorhost;

import java.time.Duration;

public final class EventProcessorOptions
{
    private Boolean invokeProcessorAfterReceiveTimeout = false;
    private int maxBatchSize = 10;
    private int prefetchCount = 300;
    private Duration receiveTimeOut = Duration.ofMinutes(1);

    public static EventProcessorOptions getDefaultOptions()
    {
        return new EventProcessorOptions();
    }

    public EventProcessorOptions()
    {
    }
    
    //
    // TODO User exception handler goes here.
    //

    public int getMaxBatchSize()
    {
        return this.maxBatchSize;
    }

    /*
     * JavaClient does not have a max batch size setting for receive.
    public void setMaxBatchSize(int maxBatchSize)
    {
        this.maxBatchSize = maxBatchSize;
    }
    */

    public Duration getReceiveTimeOut()
    {
        return this.receiveTimeOut;
    }

    public void setReceiveTimeOut(Duration receiveTimeOut)
    {
        this.receiveTimeOut = receiveTimeOut;
    }

    public int getPrefetchCount()
    {
        return this.prefetchCount;
    }

    public void setPrefetchCount(int prefetchCount)
    {
        this.prefetchCount = prefetchCount;
    }

    //
    // TODO Initial offset provider goes here.
    //
    
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
}
