/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

// BLAH

package com.microsoft.azure.eventprocessorhost;

import java.util.concurrent.Future;

// WILL NORMALLY BE IMPLEMENTED ON THE SAME CLASS AS ILeaseManager
public interface ICheckpointManager
{
    public Future<Boolean> checkpointStoreExists();

    public Future<Boolean> createCheckpointStoreIfNotExists();

    public Future<Checkpoint> getCheckpoint(String partitionId);
    
    public Future<Checkpoint> createCheckpointIfNotExists(String partitionId);

    public Future<Void> updateCheckpoint(Checkpoint checkpoint);
    public Future<Void> updateCheckpoint(Checkpoint checkpoint, String offset, long sequenceNumber);

    public Future<Void> deleteCheckpoint(String partitionId);

}
