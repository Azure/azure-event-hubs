// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    public static class EventProcessorHostActionStrings
    {
        public static readonly string CheckingLeases = "Checking Leases";
        public static readonly string ClosingEventProcessor = "Closing Event Processor";
        public static readonly string CreatingCheckpoint = "Creating Checkpoint";
        public static readonly string CreatingCheckpointStore = "Creating Checkpoint Store";
        public static readonly string CreatingEventProcessor = "Creating Event Processor";
        public static readonly string CreatingLease = "Creating Lease";
        public static readonly string CreatingLeaseStore = "Creating Lease Store";
        public static readonly string InitializingStores = "Initializing Stores";
        public static readonly string OpeningEventProcessor = "Opening Event Processor";
        public static readonly string ParitionManagerCleanup = "Partition Manager Cleanup";
        public static readonly string ParitionManagerMainLoop = "Partition Manager Main Loop";
        public static readonly string StealingLease = "Stealing Lease";
    }

}