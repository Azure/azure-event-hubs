#Microsoft Azure Event Hubs Clients

This repository contains open-source, client-side libraries for interacting with Microsoft Azure Event Hubs from 
several different programming languages. 

At present, there are clients available in [C (C99)](./c), [Node.js](./node), and [Java](./java). 

> The client library for the .NET Framework (C#/F#/VB) is [available from NuGet](https://www.nuget.org/packages/WindowsAzure.ServiceBus/). 
> An open source alternative to the officially supported, closed-source client on NuGet is the open-source [AMQP.NET Lite](https://github.com/Azure/amqpnetlite) 
> library that works on a broad variety on .NET flavors from the .NET Micro Framework to the newest .NET Core platform, and allows 
> [interaction with Event Hubs](https://github.com/Azure/amqpnetlite/blob/master/Examples/ServiceBus/Scenarios/EventHubsExample.cs). There is no product support service available for this library, but [reported issues](https://github.com/Azure/amqpnetlite/issues) 
> will be enthusiastically addressed.  

## Projects 
The clients in this repository are distinct projects and optimized for the respective language; they differ in terms of 
API shape and capabilities.   

Details about how to build and use the clients and how to explore and run the samples can be found in the respective READMEs

* The *Azure Event Hub Client for Java* is **production-ready with full Microsoft product support**. You can learn about its
  capabilities and also about how to contribute extensions and fixes in the  [README for the Java client](./java/readme.md)
* The *Azure Event Hub Client for C* is currently in "preview" state, meaning that there is ongoing work to bring it 
  to a state where Microsoft can provide full product support. Learn more in the [README for the C client](./c/README.md)
* The *Azure Event Hub Client for Node* is likewise in "preview" state. Learn more in the [README for the Node client](./node/README.md)

