# Microsoft Azure Event Hubs Client for .NET

This libary is built using .NET Standard 1.6. For more information on what platforms are supported see [.NET Platforms Support](https://docs.microsoft.com/en-us/dotnet/articles/standard/library#net-platforms-support).

Azure Event Hubs is a highly scalable publish-subscribe service that can ingest millions of events per second and stream them into multiple applications. This lets you process and analyze the massive amounts of data produced by your connected devices and applications. Once Event Hubs has collected the data, you can retrieve, transform and store it by using any real-time analytics provider or with batching/storage adapters. 

Refer to the [online documentation](https://azure.microsoft.com/services/event-hubs/) to learn more about Event Hubs in general.

## Overview

The .NET client library for Azure Event Hubs allows for both sending events to and receiving events from an Azure Event Hub. 

An **event publisher** is a source of telemetry data, diagnostics information, usage logs, or other log data, as 
part of an emvbedded device solution, a mobile device application, a game title running on a console or other device, 
some client or server based business solution, or a web site.  

An **event consumer** picks up such information from the Event Hub and processes it. Processing may involve aggregation, complex 
computation and filtering. Processing may also involve distribution or storage of the information in a raw or transformed fashion.
Event Hub consumers are often robust and high-scale platform infrastructure parts with built-in analytics capabilites, like Azure 
Stream Analytics, Apache Spark, or Apache Storm.   
   
Most applications will act either as an event publisher or an event consumer, but rarely both. The exception are event 
consumers that filter and/or transform event streams and then forward them on to another Event Hub; an example for such is Azure Stream Analytics.

### Getting Started

To get started sending events to an Event Hub refer to [Get started sending messages to Event Hubs in .NET Core](./samples/SampleSender/getting-started-sending.md).

To get started receiving events with the **EventProcessorHost** refer to [Get started receiving messages with the EventProcessorHost in .NET Core](./samples/SampleEphReceiver/getting-started-receiving-eph.md).  

### Running the unit tests 

In order to run the unit tests, you will need to set the following Environment Variables:

1. `EVENTHUBCONNECTIONSTRING`

2. `EVENTPROCESSORSTORAGECONNECTIONSTRING`

## How to provide feedback

First, if you experience any issues with the runtime behavior of the Azure Event Hubs service, please consider filing a support request
right away. Your options for [getting support are enumerated here](https://azure.microsoft.com/support/options/). In the Azure portal, 
you can file a support request from the "Help and support" menu in the upper right hand corner of the page.   

If you find issues in this library or have suggestions for improvement of code or documentation, [you can file an issue in the project's 
GitHub repository](https://github.com/Azure/azure-event-hubs/issues). Issues related to runtime behavior of the service, such as 
sporadic exceptions or apparent service-side performance or reliability issues can not be handled here.

Generally, if you want to discuss Azure Event Hubs or this client library with the community and the maintainers, you can turn to 
[stackoverflow.com under the #azure-eventhub tag](http://stackoverflow.com/questions/tagged/azure-eventhub) or the 
[MSDN Service Bus Forum](https://social.msdn.microsoft.com/Forums/en-US/home?forum=servbus). 
