// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Amqp.Management
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.Serialization;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Serialization;

    class AmqpServiceClient<T> : ClientEntity
    {
        static readonly string[] RequiredClaims = { ClaimConstants.Manage, ClaimConstants.Listen };
        readonly AmqpEventHubClient eventHubClient;
        readonly string address;
        readonly T channel;
        readonly FaultTolerantAmqpObject<RequestResponseAmqpLink> link;
        readonly ActiveClientLinkManager clientLinkManager;

        public AmqpServiceClient(AmqpEventHubClient eventHubClient, string address)
            : base(nameof(AmqpServiceClient<T>) + StringUtility.GetRandomString())
        {
            if (!typeof(T).GetTypeInfo().IsInterface)
            {
                throw new NotSupportedException("Not an interface");
            }

            this.eventHubClient = eventHubClient;
            this.address = address;
            this.channel = new AmqpClientProxy(this, typeof(T)).GetChannel();
            this.link = new FaultTolerantAmqpObject<RequestResponseAmqpLink>((t) => this.OpenLinkAsync(t), (rrlink) => rrlink.CloseAsync(TimeSpan.FromSeconds(10)));
            this.clientLinkManager = new ActiveClientLinkManager(this.eventHubClient);
        }

        public string Address
        {
            get { return this.address; }
        }

        public T Channel
        {
            get { return this.channel; }
        }

        public override Task CloseAsync()
        {
            return this.link.CloseAsync();
        }

        internal void OnAbort()
        {
            RequestResponseAmqpLink innerLink;
            if (this.link.TryGetOpenedObject(out innerLink) && innerLink != null)
            {
                innerLink.Abort();
            }
        }

        async Task<RequestResponseAmqpLink> OpenLinkAsync(TimeSpan timeout)
        {
            ActiveClientRequestResponseLink activeClientLink = await this.eventHubClient.OpenRequestResponseLinkAsync(
                "svc", this.address, null, AmqpServiceClient<T>.RequiredClaims, timeout);
            this.clientLinkManager.SetActiveLink(activeClientLink);
            return activeClientLink.Link;
        }

        Task<object> RequestAsync(MethodData md, MethodCallMessage mcm, int argCount)
        {
            return new RequestTask(this, md, mcm, argCount).Start();
        }

        sealed class RequestTask
        {
            readonly AmqpServiceClient<T> client;
            readonly MethodData md;
            readonly MethodCallMessage mcm;
            readonly int argCount;
            AmqpMessage request;
            AmqpMessage response;

            public RequestTask(AmqpServiceClient<T> client, MethodData md, MethodCallMessage mcm, int argCount)
            {
                this.client = client;
                this.md = md;
                this.mcm = mcm;
                this.argCount = argCount;
            }

            public async Task<object> Start()
            {
                RequestResponseAmqpLink requestLink = await this.client.link.GetOrCreateAsync(TimeSpan.FromMinutes(1));

                ApplicationProperties properties = new ApplicationProperties();
                properties.Map[AmqpClientConstants.ManagementOperationKey] = this.md.Operation.Name;
                // generate message sections containing the arguments
                // convert custom-type parameters if needed
                object bodyValue = null;
                AmqpMap bodyMap = null;
                for (int i = 0; i < this.argCount; i++)
                {
                    ManagementParamAttribute paramAttribute = this.md.Parameters[i];
                    object value = SerializationHelper.ToAmqp(this.md.ParameterTypes[i].Serializable, this.mcm.InArgs[i]);

                    if (paramAttribute.Location == ManagementParamLocation.ApplicationProperties)
                    {
                        properties.Map[paramAttribute.Name] = value;
                    }
                    else if (paramAttribute.Location == ManagementParamLocation.MapBody)
                    {
                        if (bodyMap == null)
                        {
                            bodyMap = new AmqpMap();
                        }

                        bodyMap[new MapKey(paramAttribute.Name)] = value;
                    }
                    else
                    {
                        bodyValue = value;
                    }
                }

                // Upsert link RequestProperties to ApplicationProperties 
                foreach (var requestProperty in requestLink.RequestProperties)
                {
                    properties.Map[requestProperty.Key] = requestProperty.Value;
                }

                this.request = AmqpMessage.Create(new AmqpValue() { Value = bodyMap ?? bodyValue });
                this.request.ApplicationProperties = properties;

                this.response = await requestLink.RequestAsync(request, TimeSpan.FromMinutes(1));
                int statusCode = (int)this.response.ApplicationProperties.Map[AmqpClientConstants.ResponseStatusCode];
                string statusDescription = (string)this.response.ApplicationProperties.Map[AmqpClientConstants.ResponseStatusDescription];
                if (statusCode != (int)AmqpResponseStatusCode.Accepted && statusCode != (int)AmqpResponseStatusCode.OK)
                {
                    AmqpSymbol errorCondition = AmqpExceptionHelper.GetResponseErrorCondition(this.response, (AmqpResponseStatusCode)statusCode);
                    Error error = new Error { Condition = errorCondition, Description = statusDescription };
                    throw new AmqpException(error);
                }

                object returnValue = null;
                if (this.response.ValueBody != null)
                {
                    returnValue = this.response.ValueBody.Value;
                }

                if (md.ReturnType.HasValue && returnValue != null)
                {
                    Type expected = md.ReturnType.Type;
                    SerializableType serializable = md.ReturnType.Serializable;
                    if (serializable == null)
                    {
                        // must be a generic parameter
                        expected = mcm.GenericTypes[md.ReturnType.Type.GenericParameterPosition];
                        serializable = expected.GetSerializable();
                    }

                    returnValue = SerializationHelper.FromAmqp(serializable, returnValue);
                    if (!expected.IsAssignableFrom(returnValue.GetType()))
                    {
                        throw new InvalidOperationException(string.Format("Return type mismatch in {0}. Expect {1} Actual {2}",
                            mcm.MethodBase.Name, expected.Name, returnValue.GetType().Name));
                    }
                }

                return returnValue;
            }
        }

        const BindingFlags MethodFlags = BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public;
        static Dictionary<Type, Dictionary<string, MethodData>> typeCache = new Dictionary<Type, Dictionary<string, MethodData>>();

        class AmqpClientProxy : RealProxy
        {
            readonly AmqpServiceClient<T> client;
            readonly Dictionary<string, MethodData> methodCache;

            public AmqpClientProxy(AmqpServiceClient<T> client, Type proxiedtype)
                : base(proxiedtype, typeof(object))
            {
                this.client = client;
                this.methodCache = GetMethodCache(proxiedtype);
            }

            public T GetChannel()
            {
                return (T)this.GetTransparentProxy();
            }

            protected override ReturnMessage Invoke(MethodCallMessage mcm)
            {
                MethodData md;
                if (!this.methodCache.TryGetValue(mcm.MethodBase.Name, out md))
                {
                    throw new NotImplementedException();
                }

                object result = null;
                if (md.Operation.AsyncPattern == AsyncPattern.None)
                {
                    result = this.client.RequestAsync(md, mcm, mcm.InArgs.Length).GetAwaiter().GetResult();
                }                
                else if (md.Operation.AsyncPattern == AsyncPattern.Task)
                {
                    Fx.Assert(md.TcsType != null, "Must be a task method");
                    Type tcsType = md.TcsType;
                    if (md.ReturnType.Type.IsGenericParameter)
                    {
                        tcsType = typeof(TaskSource<>).MakeGenericType(typeof(T),
                            mcm.GenericTypes[md.ReturnType.Type.GenericParameterPosition]);
                    }

                    ITaskSource ts = (ITaskSource)Activator.CreateInstance(tcsType);
                    ts.Proxy = this;
                    ts.MethodData = md;
                    ts.CallMessage = mcm;
                    this.client.RequestAsync(md, mcm, mcm.InArgs.Length)
                        .ContinueWith(t =>
                        {
                            if (t.Status == TaskStatus.RanToCompletion)
                            {
                                ts.Complete(t.Result, null);
                            }
                            else
                            {
                                ts.Complete(null, t.Exception);
                            }
                        });
                    result = ts.TaskObject;
                }
                else
                {
                    throw new InvalidOperationException();
                }

                return new ReturnMessage(result, null, 0, mcm);
            }
        }

        static Dictionary<string, MethodData> GetMethodCache(Type type)
        {
            Dictionary<string, MethodData> cache;
            if (typeCache.TryGetValue(type, out cache))
            {
                return cache;
            }

            cache = new Dictionary<string, MethodData>();
            foreach (var mi in type.GetMethods(MethodFlags))
            {
                var attribute = mi.GetCustomAttribute<ManagementOperationAttribute>();
                if (attribute != null)
                {
                    var existing = cache.Values.FirstOrDefault(v => v.Operation.Name == attribute.Name);
                    if (existing != null)
                    {
                        throw new SerializationException(existing.Operation.Name + " operation already exists.");
                    }

                    int ignoreCount = /*attribute.AsyncPattern == AsyncPattern.APM ? 2 :*/ 0;
                    ManagementParamAttribute[] paramAttributes;
                    DataType[] paramTypes;
                    mi.GetParameters(ignoreCount, out paramAttributes, out paramTypes);

                    if (paramAttributes.Where(p => p.Location == ManagementParamLocation.MapBody).Any() &&
                        paramAttributes.Where(p => p.Location == ManagementParamLocation.ValueBody).Any())
                    {
                        throw new SerializationException("Cannot have both MapBody and ValueBody for parameters in method " + mi.Name);
                    }

                    if (paramAttributes.Where(p => p.Location == ManagementParamLocation.ValueBody).Count() > 2)
                    {
                        throw new SerializationException("Cannot have more than one ValueBody params in method " + mi.Name);
                    }

                    MethodData md = new MethodData()
                    {
                        Operation = attribute,
                        Parameters = paramAttributes,
                        ParameterTypes = paramTypes
                    };

                    if (typeof(Task).IsAssignableFrom(mi.ReturnType))
                    {
                        attribute.AsyncPattern = AsyncPattern.Task;
                        Type[] genericArgs = null;
                        if (mi.ReturnType.GetTypeInfo().IsGenericType)
                        {
                            genericArgs = mi.ReturnType.GetGenericArguments();
                            if (genericArgs.Length > 1)
                            {
                                throw new NotSupportedException(mi.Name + ": Return type can have at most one generic argument");
                            }

                            md.ReturnType = DataType.Wrap(genericArgs[0]);
                            md.TcsType = typeof(TaskSource<>).MakeGenericType(typeof(T), genericArgs[0]);
                        }
                        else
                        {
                            md.ReturnType = DataType.Wrap(typeof(void));
                            md.TcsType = typeof(TaskSource<object>);
                        }

                        cache[mi.Name] = md;
                    }
                    else
                    {
                        attribute.AsyncPattern = AsyncPattern.None;
                        md.ReturnType = DataType.Wrap(mi.ReturnType);
                        cache[mi.Name] = md;
                    }
                }
            }

            typeCache[type] = cache;

            return cache;
        }

        class MethodData
        {
            public ManagementOperationAttribute Operation;

            public ManagementParamAttribute[] Parameters;

            public DataType[] ParameterTypes;

            public DataType ReturnType;

            //public bool IsBeginCall;

            //public bool IsEndCall;

            public Type TcsType;
        }

        interface ITaskSource
        {
            object TaskObject { get; }

            AmqpClientProxy Proxy { get; set; }

            MethodData MethodData { get; set; }

            MethodCallMessage CallMessage { get; set; }

            void Complete(object result, Exception exception);
        }

        class TaskSource<TResult> : TaskCompletionSource<TResult>, ITaskSource
        {
            public object TaskObject
            {
                get { return this.Task; }
            }

            public AmqpClientProxy Proxy { get; set; }

            public MethodData MethodData { get; set; }

            public MethodCallMessage CallMessage { get; set; }

            public void Complete(object result, Exception exception)
            {
                if (exception != null)
                {
                    this.SetException(exception);
                }
                else
                {
                    this.SetResult((TResult)result);
                }
            }
        }
    }
}
