// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Amqp.Management
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Reflection;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Serialization;

    static class SerializationHelper
    {
        static AmqpContractSerializer serializer = new AmqpContractSerializer();

        public static void GetParameters(this MethodInfo mi, int ignoreCount,
            out ManagementParamAttribute[] attributes, out DataType[] paramTypes)
        {
            ParameterInfo[] paramInfos = mi.GetParameters();
            int count = paramInfos.Length - ignoreCount;
            attributes = new ManagementParamAttribute[count];
            paramTypes = new DataType[count];
            for (int i = 0; i < count; i++)
            {
                if (paramInfos[i].ParameterType.IsGenericParameter)
                {
                    throw new NotSupportedException("Generic parameter not supported for " + paramInfos[i].Name);
                }

                var paramAttribute = paramInfos[i].GetCustomAttribute<ManagementParamAttribute>();
                if (paramAttribute == null)
                {
                    paramAttribute = new ManagementParamAttribute();
                }

                if (paramAttribute.Name == null)
                {
                    paramAttribute.Name = paramInfos[i].Name;
                }

                attributes[i] = paramAttribute;
                paramTypes[i] = DataType.Wrap(paramInfos[i].ParameterType);
            }
        }

        public static SerializableType GetSerializable(this Type type)
        {
            return serializer.GetType(type);
        }

        public static object ToAmqp(SerializableType serializable, object value)
        {
            if (value == null)
            {
                return null;
            }

            if (serializable.AmqpType == AmqpType.Primitive)
            {
                var listType = serializable as SerializableType.List;
                if (listType != null && listType.ItemType.AmqpType != AmqpType.Primitive)
                {
                    List<object> list = new List<object>();
                    foreach (var item in (IEnumerable)value)
                    {
                        list.Add(ToAmqp(listType.ItemType, item));
                    }

                    value = list;
                }

                return value;
            }

            if (serializable.AmqpType == AmqpType.Described)
            {
                return ((SerializableType.Converted)serializable).GetTarget(value);
            }

            if (serializable.AmqpType == AmqpType.Composite)
            {
                var composite = (SerializableType.Composite)serializable;
                AmqpMap map = new AmqpMap();
                foreach (var member in composite.Members)
                {
                    map[new MapKey(member.Name)] = member.Accessor.Get(value);
                }

                return map;
            }

            return value;
        }

        public static object FromAmqp(SerializableType serializable, object value)
        {
            if (value == null)
            {
                return null;
            }

            if (serializable.AmqpType == AmqpType.Primitive)
            {
                var listType = serializable as SerializableType.List;
                if (listType != null && listType.ItemType.AmqpType != AmqpType.Primitive)
                {
                    IList list = (IList)serializable.CreateInstance();
                    foreach (var item in (IEnumerable)value)
                    {
                        list.Add(FromAmqp(listType.ItemType, item));
                    }

                    value = list;
                }

                return value;
            }

            if (serializable.AmqpType == AmqpType.Described)
            {
                return ((SerializableType.Converted)serializable).GetSource(value);
            }

            if (serializable.AmqpType == AmqpType.Composite)
            {
                object container = serializable.CreateInstance();
                var composite = (SerializableType.Composite)serializable;
                AmqpMap map = (AmqpMap)value;
                foreach (var member in composite.Members)
                {
                    object obj = map[new MapKey(member.Name)];
                    if (obj != null)
                    {
                        member.Accessor.Set(container, obj);
                    }
                }

                return container;
            }

            return value;
        }
    }

}
