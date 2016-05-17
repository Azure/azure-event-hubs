// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Amqp
{
    using System;
    using Microsoft.Azure.Amqp;

    abstract class ActiveClientLinkObject
    {
        readonly bool isClientToken;
        readonly string audience;
        readonly string endpointUri;
        readonly string[] requiredClaims;
        readonly AmqpObject amqpLinkObject;
        DateTime authorizationValidToUtc;

        public ActiveClientLinkObject(AmqpObject amqpLinkObject, string audience, string endpointUri, string[] requiredClaims, bool isClientToken, DateTime authorizationValidToUtc)
        {
            this.amqpLinkObject = amqpLinkObject;
            this.audience = audience;
            this.endpointUri = endpointUri;
            this.requiredClaims = requiredClaims;
            this.isClientToken = isClientToken;
            this.authorizationValidToUtc = authorizationValidToUtc;
        }

        public bool IsClientToken
        {
            get { return this.isClientToken; }
        }

        public string Audience
        {
            get { return this.audience; }
        }

        public string EndpointUri
        {
            get { return this.endpointUri; }
        }

        public string[] RequiredClaims
        {
            get { return (string[])this.requiredClaims.Clone(); }
        }

        public DateTime AuthorizationValidToUtc
        {
            get { return this.authorizationValidToUtc; }
            set { this.authorizationValidToUtc = value; }
        }

        public AmqpObject LinkObject
        {
            get
            {
                return this.amqpLinkObject;
            }
        }

        public abstract AmqpConnection Connection
        {
            get;
        }
    }
}
