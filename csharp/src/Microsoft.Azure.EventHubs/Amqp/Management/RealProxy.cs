// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Amqp.Management
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Threading;

    abstract class RealProxy
    {
        internal const string DynamicAssemblyName = "DynamicProxyAssembly.F4A75B89";

        static Dictionary<Type, Type> proxyTypes = new Dictionary<Type, Type>();
        static ProxyAssembly proxyAssembly;
        Type proxyType;
        Type baseType;
        internal Type proxiedType;

        protected RealProxy(Type proxiedType, Type baseType)
        {
            this.proxiedType = proxiedType;
            this.baseType = baseType;
        }

        Type GetProxyType()
        {
            // lock?
            if (this.proxyType == null)
            {
                lock (proxyTypes)
                {
                    if (!proxyTypes.TryGetValue(this.proxiedType, out this.proxyType))
                    {
                        if (proxyAssembly == null)
                        {
                            proxyAssembly = new ProxyAssembly(DynamicAssemblyName);
                        }

                        Type proxyBaseType;
                        if (!proxyTypes.TryGetValue(this.baseType, out proxyBaseType))
                        {
                            ProxyBuilder pbt = proxyAssembly.CreateProxy("proxyBase", typeof(object));
                            foreach (Type t in baseType.GetInterfaces())
                                pbt.AddInterfaceImpl(t);
                            proxyTypes[this.baseType] = proxyBaseType = pbt.CreateType();
                        }

                        ProxyBuilder pb = proxyAssembly.CreateProxy("proxy", proxyBaseType);
                        foreach (Type t in this.proxiedType.GetInterfaces())
                        {
                            pb.AddInterfaceImpl(t);
                        }
                        pb.AddInterfaceImpl(this.proxiedType);
                        proxyTypes[this.proxiedType] = this.proxyType = pb.CreateType();
                    }
                }
            }
            return proxyType;
        }

        public object GetTransparentProxy()
        {
            return Activator.CreateInstance(GetProxyType(), this, new Action<object[]>(Invoke));
        }

        // this method is the callback from the RealProxy
        static void Invoke(object[] args)
        {
            PackedArgs packed = new PackedArgs(args);
            MethodBase method = proxyAssembly.ResolveMethodToken(packed.DeclaringType, packed.MethodToken);
            if (method.IsGenericMethodDefinition)
            {
                method = ((MethodInfo)method).MakeGenericMethod(packed.GenericTypes);
            }

            MethodCallMessage mcm = new MethodCallMessage(method, packed.Args, packed.GenericTypes);

            ReturnMessage mrm = (ReturnMessage)packed.RealProxy.Invoke(mcm);

            if (mrm.Exception != null)
            {
                throw mrm.Exception;
            }
            else
            {
                packed.ReturnValue = mrm.ReturnValue;
            }
        }

        protected abstract ReturnMessage Invoke(MethodCallMessage mcm);

        class PackedArgs
        {
            internal const int RealProxyPosition = 0;
            internal const int DeclaringTypePosition = 1;
            internal const int MethodTokenPosition = 2;
            internal const int ArgsPosition = 3;
            internal const int GenericTypesPosition = 4;
            internal const int ReturnValuePosition = 5;

            internal static readonly Type[] PackedTypes = new Type[] { typeof(object), typeof(Type), typeof(int), typeof(object[]), typeof(Type[]), typeof(object) };

            object[] args;
            internal PackedArgs() : this(new object[PackedTypes.Length]) { }
            internal PackedArgs(object[] args) { this.args = args; }

            internal RealProxy RealProxy { get { return (RealProxy)args[RealProxyPosition]; } }
            internal Type DeclaringType { get { return (Type)args[DeclaringTypePosition]; } }
            internal int MethodToken { get { return (int)args[MethodTokenPosition]; } }
            internal object[] Args { get { return (object[])args[ArgsPosition]; } }
            internal Type[] GenericTypes { get { return (Type[])args[GenericTypesPosition]; } }
            internal object ReturnValue { /*get { return args[ReturnValuePosition]; }*/ set { args[ReturnValuePosition] = value; } }
        }

        class ProxyAssembly
        {
            AssemblyBuilder ab;
            ModuleBuilder mb;
            int typeId = 0;
            // Method token is keyed by Type and MethodToken.
            Dictionary<Type, Dictionary<int, MethodBase>> methodLookup;

            public ProxyAssembly(string name)
            {
                AssemblyName assemblyName = new AssemblyName(name);
                assemblyName.SetPublicKey(this.GetType().GetTypeInfo().Assembly.GetName().GetPublicKey());
                this.ab = AssemblyBuilder.DefineDynamicAssembly(assemblyName, AssemblyBuilderAccess.Run);
                this.mb = ab.DefineDynamicModule("testmod");
            }

            public ProxyBuilder CreateProxy(string name, Type proxyBaseType)
            {
                int nextId = Interlocked.Increment(ref typeId);
                TypeBuilder tb = this.mb.DefineType(name + "_" + nextId, TypeAttributes.Public, proxyBaseType);
                return new ProxyBuilder(this, tb, proxyBaseType);
            }

            internal void GetTokenForMethod(MethodBase method, out Type type, out int token)
            {
                type = method.DeclaringType;

                // DNX_TODO: Figure out something to replace this.  For now, try hashcode
                // token = method.MetadataToken;
                token = method.GetHashCode();

                if (methodLookup == null)
                {
                    methodLookup = new Dictionary<Type, Dictionary<int, MethodBase>>();
                }
                Dictionary<int, MethodBase> tokens;
                if (!methodLookup.TryGetValue(type, out tokens))
                {
                    tokens = new Dictionary<int, MethodBase>();
                    methodLookup[type] = tokens;
                }
                tokens[token] = method;
            }

            internal MethodBase ResolveMethodToken(Type type, int token)
            {
                Debug.Assert(methodLookup != null && methodLookup.ContainsKey(type) && methodLookup[type].ContainsKey(token));
                return methodLookup[type][token];
            }
        }

        class ProxyBuilder
        {
            static readonly MethodInfo delegateInvoke = typeof(Action<object[]>).GetMethod("Invoke");

            ProxyAssembly assembly;
            TypeBuilder tb;
            Type proxyBaseType;
            List<FieldBuilder> fields;

            internal ProxyBuilder(ProxyAssembly assembly, TypeBuilder tb, Type proxyBaseType)
            {
                this.assembly = assembly;
                this.tb = tb;
                this.proxyBaseType = proxyBaseType;

                this.fields = new List<FieldBuilder>();
                this.fields.Add(tb.DefineField("proxy", typeof(object), FieldAttributes.Private));
                this.fields.Add(tb.DefineField("invoke", typeof(Action<object[]>), FieldAttributes.Private));
            }

            void Complete()
            {
                Type[] args = new Type[this.fields.Count];
                for (int i = 0; i < args.Length; i++)
                {
                    args[i] = this.fields[i].FieldType;
                }

                ConstructorBuilder cb = tb.DefineConstructor(MethodAttributes.Public, CallingConventions.HasThis, args);
                ILGenerator il = cb.GetILGenerator();

                // chained ctor call
                il.Emit(OpCodes.Ldarg_0);
                if (this.proxyBaseType == typeof(object))
                {
                    il.Emit(OpCodes.Call, typeof(object).GetConstructor(new Type[0]));
                }
                else
                {
                    for (int i = 0; i < args.Length; i++)
                    {
                        il.Emit(OpCodes.Ldarg, i + 1);
                    }
                    il.Emit(OpCodes.Call, this.proxyBaseType.GetConstructor(args));
                }

                // store all the fields
                for (int i = 0; i < args.Length; i++)
                {
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldarg, i + 1);
                    il.Emit(OpCodes.Stfld, this.fields[i]);
                }

                il.Emit(OpCodes.Ret);

                // GetRealProxy
                if (this.proxyBaseType == typeof(object))
                {
                    MethodBuilder mdb = tb.DefineMethod("GetRealProxy", MethodAttributes.Public, typeof(object), new Type[0]);
                    il = mdb.GetILGenerator();
                    il.Emit(OpCodes.Ldarg_0);
                    il.Emit(OpCodes.Ldfld, this.fields[0]);
                    il.Emit(OpCodes.Ret);
                }
            }

            internal Type CreateType()
            {
                this.Complete();
                return this.tb.CreateTypeInfo().AsType();
            }

            internal void AddInterfaceImpl(Type iface)
            {
                this.tb.AddInterfaceImplementation(iface);
                foreach (MethodInfo mi in iface.GetMethods())
                {
                    AddMethodImpl(mi);
                }
            }

            void AddMethodImpl(MethodInfo mi)
            {
                ParameterInfo[] parameters = mi.GetParameters();
                Type[] paramTypes = ParamTypes(parameters, false);

                MethodBuilder mdb = tb.DefineMethod(mi.Name, MethodAttributes.Public | MethodAttributes.Virtual, mi.ReturnType, paramTypes);
                if (mi.ContainsGenericParameters)
                {
                    Type[] ts = mi.GetGenericArguments();
                    string[] ss = new string[ts.Length];
                    for (int i = 0; i < ts.Length; i++)
                    {
                        ss[i] = ts[i].Name;
                    }
                    GenericTypeParameterBuilder[] genericParameters = mdb.DefineGenericParameters(ss);
                    for (int i = 0; i < genericParameters.Length; i++)
                    {
                        genericParameters[i].SetGenericParameterAttributes(ts[i].GetTypeInfo().GenericParameterAttributes);
                    }
                }
                ILGenerator il = mdb.GetILGenerator();

                ParametersArray args = new ParametersArray(il, paramTypes);

                // object[] args = new object[paramCount];
                il.Emit(OpCodes.Nop);
                GenericArray<object> argsArr = new GenericArray<object>(il, ParamTypes(parameters, true).Length);

                for (int i = 0; i < parameters.Length; i++)
                {
                    // args[i] = argi;
                    if (!parameters[i].IsOut)
                    {
                        argsArr.BeginSet(i);
                        args.Get(i);
                        argsArr.EndSet(parameters[i].ParameterType);
                    }
                }

                // object[] packed = new object[PackedArgs.PackedTypes.Length];
                GenericArray<object> packedArr = new GenericArray<object>(il, PackedArgs.PackedTypes.Length);

                // packed[PackedArgs.RealProxyPosition] = proxy;
                packedArr.BeginSet(PackedArgs.RealProxyPosition);
                il.Emit(OpCodes.Ldarg_0);
                il.Emit(OpCodes.Ldfld, this.fields[0]); // proxy reference
                packedArr.EndSet(this.fields[0].FieldType);

                // packed[PackedArgs.DeclaringTypePosition] = typeof(iface);
                MethodInfo Type_GetTypeFromHandle = typeof(Type).GetMethod("GetTypeFromHandle", new Type[] { typeof(RuntimeTypeHandle) });
                int methodToken;
                Type declaringType;
                assembly.GetTokenForMethod(mi, out declaringType, out methodToken);
                packedArr.BeginSet(PackedArgs.DeclaringTypePosition);
                il.Emit(OpCodes.Ldtoken, declaringType);
                il.Emit(OpCodes.Call, Type_GetTypeFromHandle);
                packedArr.EndSet(typeof(object));

                // packed[PackedArgs.MethodTokenPosition] = iface method token;
                packedArr.BeginSet(PackedArgs.MethodTokenPosition);
                il.Emit(OpCodes.Ldc_I4, methodToken);
                packedArr.EndSet(typeof(Int32));

                // packed[PackedArgs.ArgsPosition] = args;
                packedArr.BeginSet(PackedArgs.ArgsPosition);
                argsArr.Load();
                packedArr.EndSet(typeof(object[]));

                // packed[PackedArgs.GenericTypesPosition] = mi.GetGenericArguments();
                if (mi.ContainsGenericParameters)
                {
                    packedArr.BeginSet(PackedArgs.GenericTypesPosition);
                    Type[] genericTypes = mi.GetGenericArguments();
                    GenericArray<Type> typeArr = new GenericArray<Type>(il, genericTypes.Length);
                    for (int i = 0; i < genericTypes.Length; ++i)
                    {
                        typeArr.BeginSet(i);
                        il.Emit(OpCodes.Ldtoken, genericTypes[i]);
                        il.Emit(OpCodes.Call, Type_GetTypeFromHandle);
                        typeArr.EndSet(typeof(Type));
                    }
                    typeArr.Load();
                    packedArr.EndSet(typeof(Type[]));
                }

                // Call static RealProxy.Invoke(object[])
                il.Emit(OpCodes.Ldarg_0);
                il.Emit(OpCodes.Ldfld, this.fields[1]); // delegate
                packedArr.Load();
                il.Emit(OpCodes.Call, delegateInvoke);

                for (int i = 0; i < parameters.Length; i++)
                {
                    if (parameters[i].ParameterType.IsByRef)
                    {
                        args.BeginSet(i);
                        argsArr.Get(i);
                        args.EndSet(i, typeof(object));
                    }
                }

                if (mi.ReturnType != typeof(void))
                {
                    packedArr.Get(PackedArgs.ReturnValuePosition);
                    Convert(il, typeof(object), mi.ReturnType, false);
                }

                il.Emit(OpCodes.Ret);

                tb.DefineMethodOverride(mdb, mi);
            }

            static Type[] ParamTypes(ParameterInfo[] parms, bool noByRef)
            {
                Type[] types = new Type[parms.Length];
                for (int i = 0; i < parms.Length; i++)
                {
                    types[i] = parms[i].ParameterType;
                    if (noByRef && types[i].IsByRef)
                        types[i] = types[i].GetElementType();
                }
                return types;
            }

            static OpCode[] ConvOpCodes = new OpCode[] {
                OpCodes.Nop,//Empty = 0,
                OpCodes.Nop,//Object = 1,
                OpCodes.Nop,//DBNull = 2,
                OpCodes.Conv_I1,//Boolean = 3,
                OpCodes.Conv_I2,//Char = 4,
                OpCodes.Conv_I1,//SByte = 5,
                OpCodes.Conv_U1,//Byte = 6,
                OpCodes.Conv_I2,//Int16 = 7,
                OpCodes.Conv_U2,//UInt16 = 8,
                OpCodes.Conv_I4,//Int32 = 9,
                OpCodes.Conv_U4,//UInt32 = 10,
                OpCodes.Conv_I8,//Int64 = 11,
                OpCodes.Conv_U8,//UInt64 = 12,
                OpCodes.Conv_R4,//Single = 13,
                OpCodes.Conv_R8,//Double = 14,
                OpCodes.Nop,//Decimal = 15,
                OpCodes.Nop,//DateTime = 16,
                OpCodes.Nop,//17
                OpCodes.Nop,//String = 18,
            };

            static OpCode[] LdindOpCodes = new OpCode[] {
                OpCodes.Nop,//Empty = 0,
                OpCodes.Nop,//Object = 1,
                OpCodes.Nop,//DBNull = 2,
                OpCodes.Ldind_I1,//Boolean = 3,
                OpCodes.Ldind_I2,//Char = 4,
                OpCodes.Ldind_I1,//SByte = 5,
                OpCodes.Ldind_U1,//Byte = 6,
                OpCodes.Ldind_I2,//Int16 = 7,
                OpCodes.Ldind_U2,//UInt16 = 8,
                OpCodes.Ldind_I4,//Int32 = 9,
                OpCodes.Ldind_U4,//UInt32 = 10,
                OpCodes.Ldind_I8,//Int64 = 11,
                OpCodes.Ldind_I8,//UInt64 = 12,
                OpCodes.Ldind_R4,//Single = 13,
                OpCodes.Ldind_R8,//Double = 14,
                OpCodes.Nop,//Decimal = 15,
                OpCodes.Nop,//DateTime = 16,
                OpCodes.Nop,//17
                OpCodes.Ldind_Ref,//String = 18,
            };

            static OpCode[] StindOpCodes = new OpCode[] {
                OpCodes.Nop,//Empty = 0,
                OpCodes.Nop,//Object = 1,
                OpCodes.Nop,//DBNull = 2,
                OpCodes.Stind_I1,//Boolean = 3,
                OpCodes.Stind_I2,//Char = 4,
                OpCodes.Stind_I1,//SByte = 5,
                OpCodes.Stind_I1,//Byte = 6,
                OpCodes.Stind_I2,//Int16 = 7,
                OpCodes.Stind_I2,//UInt16 = 8,
                OpCodes.Stind_I4,//Int32 = 9,
                OpCodes.Stind_I4,//UInt32 = 10,
                OpCodes.Stind_I8,//Int64 = 11,
                OpCodes.Stind_I8,//UInt64 = 12,
                OpCodes.Stind_R4,//Single = 13,
                OpCodes.Stind_R8,//Double = 14,
                OpCodes.Nop,//Decimal = 15,
                OpCodes.Nop,//DateTime = 16,
                OpCodes.Nop,//17
                OpCodes.Stind_Ref,//String = 18,
            };

            static void Convert(ILGenerator il, Type source, Type target, bool isAddress)
            {
                Debug.Assert(!target.IsByRef);
                if (target == source)
                    return;
                if (source.IsByRef)
                {
                    Debug.Assert(!isAddress);
                    Type argType = source.GetElementType();
                    Ldind(il, argType);
                    Convert(il, argType, target, isAddress);
                    return;
                }
                if (target.GetTypeInfo().IsValueType)
                {
                    if (source.GetTypeInfo().IsValueType)
                    {
                        OpCode opCode = ConvOpCodes[(int)TypeHelper.FakeGetTypeCode(target)];
                        Debug.Assert(!opCode.Equals(OpCodes.Nop));
                        il.Emit(opCode);
                    }
                    else
                    {
                        Debug.Assert(source.IsAssignableFrom(target));
                        il.Emit(OpCodes.Unbox, target);
                        if (!isAddress)
                            Ldind(il, target);
                    }
                }
                else if (target.IsAssignableFrom(source))
                {
                    if (source.GetTypeInfo().IsValueType)
                    {
                        if (isAddress)
                            Ldind(il, source);
                        il.Emit(OpCodes.Box, source);
                    }
                }
                else
                {
                    Debug.Assert(source.IsAssignableFrom(target) || target.GetTypeInfo().IsInterface || source.GetTypeInfo().IsInterface);
                    if (target.IsGenericParameter)
                    {
                        il.Emit(OpCodes.Unbox_Any, target);
                    }
                    else
                    {
                        il.Emit(OpCodes.Castclass, target);
                    }
                }
            }

            static void Ldind(ILGenerator il, Type type)
            {
                OpCode opCode = LdindOpCodes[(int)TypeHelper.FakeGetTypeCode(type)];
                if (!opCode.Equals(OpCodes.Nop))
                {
                    il.Emit(opCode);
                }
                else
                {
                    il.Emit(OpCodes.Ldobj, type);
                }
            }

            static void Stind(ILGenerator il, Type type)
            {
                OpCode opCode = StindOpCodes[(int)TypeHelper.FakeGetTypeCode(type)];
                if (!opCode.Equals(OpCodes.Nop))
                {
                    il.Emit(opCode);
                }
                else
                {
                    il.Emit(OpCodes.Stobj, type);
                }
            }

            class ParametersArray
            {
                ILGenerator il;
                Type[] paramTypes;
                internal ParametersArray(ILGenerator il, Type[] paramTypes)
                {
                    this.il = il;
                    this.paramTypes = paramTypes;
                }

                internal void Get(int i)
                {
                    il.Emit(OpCodes.Ldarg, i + 1);
                }

                internal void BeginSet(int i)
                {
                    il.Emit(OpCodes.Ldarg, i + 1);
                }

                internal void EndSet(int i, Type stackType)
                {
                    Debug.Assert(this.paramTypes[i].IsByRef);
                    Type argType = this.paramTypes[i].GetElementType();
                    Convert(il, stackType, argType, false);
                    Stind(il, argType);
                }
            }

            class GenericArray<T>
            {
                ILGenerator il;
                LocalBuilder lb;
                internal GenericArray(ILGenerator il, int len)
                {
                    this.il = il;
                    this.lb = il.DeclareLocal(typeof(T[]));

                    il.Emit(OpCodes.Ldc_I4, len);
                    il.Emit(OpCodes.Newarr, typeof(T));
                    il.Emit(OpCodes.Stloc, lb);
                }

                internal void Load()
                {
                    il.Emit(OpCodes.Ldloc, lb);
                }

                internal void Get(int i)
                {
                    il.Emit(OpCodes.Ldloc, lb);
                    il.Emit(OpCodes.Ldc_I4, i);
                    il.Emit(OpCodes.Ldelem_Ref);
                }

                internal void BeginSet(int i)
                {
                    il.Emit(OpCodes.Ldloc, lb);
                    il.Emit(OpCodes.Ldc_I4, i);
                }

                internal void EndSet(Type stackType)
                {
                    Convert(il, stackType, typeof(T), false);
                    il.Emit(OpCodes.Stelem_Ref);
                }
            }
        }
    }

    abstract class MethodMessageBase
    {
        readonly MethodBase method;

        protected MethodMessageBase(MethodBase method)
        {
            this.method = method;
        }

        public MethodBase MethodBase
        {
            get { return this.method; }
        }
    }

    class MethodCallMessage : MethodMessageBase
    {
        public MethodCallMessage(MethodBase method, object[] inArgs, Type[] genericTypes)
            : base(method)
        {
            this.InArgs = inArgs;
            this.GenericTypes = genericTypes;
        }

        public object[] InArgs
        {
            get;
            private set;
        }

        public Type[] GenericTypes
        {
            get;
            private set;
        }
    }

    class ReturnMessage : MethodMessageBase
    {
        readonly object ret;
        readonly Exception exception;

        public ReturnMessage(object ret, object[] outArgs, int outArgsCount, MethodCallMessage mcm)
            : base(mcm.MethodBase)
        {
            this.ret = ret;
        }

        public ReturnMessage(Exception e, MethodCallMessage mcm)
            : base(mcm.MethodBase)
        {
            this.exception = e;
        }

        public object ReturnValue
        {
            get { return this.ret; }
        }

        public Exception Exception
        {
            get { return this.exception; }
        }
    }

    static class TypeHelper
    {
        // Replace this once we're on a CoreCLR version which supports Type.GetTypeCode(Type t)
        public static TypeCode FakeGetTypeCode(Type type)
        {
            if (type.GetTypeInfo().IsClass)
            {
                if (type == typeof(string))
                {
                    return TypeCode.String;
                }
                //else if (type == typeof(DBNull))
                //{
                //    // TypeCode.DBNull not in CoreCLR
                //    return TypeCode.DBNull;
                //}

                return TypeCode.Object;
            }
            else
            {
                if (type.GetTypeInfo().IsEnum)
                {
                    type = Enum.GetUnderlyingType(type);
                }

                if (type == typeof(bool))
                {
                    return TypeCode.Boolean;
                }
                else if (type == typeof(byte))
                {
                    return TypeCode.Byte;
                }
                else if (type == typeof(char))
                {
                    return TypeCode.Char;
                }
                else if (type == typeof(DateTime))
                {
                    return TypeCode.DateTime;
                }
                else if (type == typeof(decimal))
                {
                    return TypeCode.Decimal;
                }
                else if (type == typeof(double))
                {
                    return TypeCode.Double;
                }
                else if (type == typeof(short))
                {
                    return TypeCode.Int16;
                }
                else if (type == typeof(int))
                {
                    return TypeCode.Int32;
                }
                else if (type == typeof(long))
                {
                    return TypeCode.Int64;
                }
                else if (type == typeof(sbyte))
                {
                    return TypeCode.SByte;
                }
                else if (type == typeof(float))
                {
                    return TypeCode.Single;
                }
                else if (type == typeof(ushort))
                {
                    return TypeCode.UInt16;
                }
                else if (type == typeof(uint))
                {
                    return TypeCode.UInt32;
                }
                else if (type == typeof(ulong))
                {
                    return TypeCode.UInt64;
                }

                throw new NotImplementedException(type.ToString());
            }
        }
    }
}
