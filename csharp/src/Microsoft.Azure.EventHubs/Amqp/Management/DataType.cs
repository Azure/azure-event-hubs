// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Amqp.Management
{
    using System;
    using Microsoft.Azure.Amqp.Serialization;

    class DataType
    {
        public Type Type
        {
            get;
            private set;
        }

        public bool HasValue
        {
            get;
            private set;
        }

        public SerializableType Serializable
        {
            get;
            private set;
        }

        public static DataType Wrap(Type type)
        {
            return new DataType()
            {
                Type = type,
                HasValue = type != typeof(void),
                Serializable = (type == typeof(void) || type.IsGenericParameter) ? null : type.GetSerializable()
            };
        }
    }
}
