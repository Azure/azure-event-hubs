// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor
{
    using System;
    using Newtonsoft.Json;
    using WindowsAzure.Storage;
    using WindowsAzure.Storage.Blob;

    class AzureBlobLease : Lease
    {
        string offset = null;

        // ctor needed for deserialization
        internal AzureBlobLease()
        {
        }

        internal AzureBlobLease(string partitionId, CloudBlockBlob blob)
            : base(partitionId)
        {
            this.Blob = blob;
        }

        internal AzureBlobLease(AzureBlobLease source)
            : base(source)
        {
            this.Offset = source.Offset;
            this.SequenceNumber = source.SequenceNumber;
            this.Blob = source.Blob;
        }

        internal AzureBlobLease(AzureBlobLease source, CloudBlockBlob blob)
            : base(source)
        {
            this.Offset = source.Offset;
            this.SequenceNumber = source.SequenceNumber;
            this.Blob = blob;
        }

        // do not serialize
        [JsonIgnore]
        public CloudBlockBlob Blob { get; }

        public string Offset
        {
            get
            {
                return this.offset;
            }

            set
            {
                this.offset = value;
            }
        }

        public long SequenceNumber { get; set; }

        public override bool IsExpired()
        {
		    this.Blob.FetchAttributesAsync().GetAwaiter().GetResult(); // Get the latest metadata
            LeaseState currentState = this.Blob.Properties.LeaseState;
		    return (currentState != LeaseState.Leased);
        }

        public override string GetStateDebug()
        {
            string retval;
            try
            {
                this.Blob.FetchAttributesAsync().GetAwaiter().GetResult();
                BlobProperties props = this.Blob.Properties;
                retval = props.LeaseState + " " + props.LeaseStatus + " " + props.LeaseDuration;
            }
            catch (StorageException e)
            {
                retval = "FetchAttributesAsync on the Blob caught " + e;
            }

            return retval;
        }
    }
}