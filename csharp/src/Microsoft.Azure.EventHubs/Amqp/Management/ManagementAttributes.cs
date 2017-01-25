// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Amqp.Management
{
    using System;

    enum ManagementParamLocation
    {
        ApplicationProperties,
        MapBody,
        ValueBody
    }

    enum AsyncPattern
    {
        None,
        Task
    }

    [AttributeUsage(AttributeTargets.Parameter, AllowMultiple = false, Inherited = true)]
    class ManagementParamAttribute : Attribute
    {
        public ManagementParamAttribute()
        {
            this.Location = ManagementParamLocation.MapBody;
        }

        public string Name { get; set; }

        public ManagementParamLocation Location { get; set; }
    }

    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = true)]
    class ManagementOperationAttribute : Attribute
    {
        public string Name { get; set; }

        public AsyncPattern AsyncPattern { get; set; }
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface,
        AllowMultiple = false, Inherited = true)]
    class ManagementAttribute : Attribute
    {
    }
}
