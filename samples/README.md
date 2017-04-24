# Azure Event Hubs samples

## .NET

### Microsoft.Azure.EventHubs
Any samples within the `Azure.Microsoft.EventHubs` folder target the newer .NET Standard library.

#### SampleSender

[This sample](./DotNet/Microsoft.Azure.EventHubs/SampleSender/readme.md) shows how to write a .NET Core console application that sends a set of messages to an Event Hub.

#### SampleEphReceiver

[This sample](./DotNet/Microsoft.Azure.EventHubs/SampleEphReceiver/readme.md) shows how to write a .NET Core console application that receives messages from an Event Hub using the **EventProcessorHost**.

## Java

### Get started sending events
[This sample](./Java/send.md) shows how to write a Java console application that sends a set of messages to an Event Hub.

### Get started receiving events using the Event Processor Host
[This sample](./Java/receive-using-eph.md) shows how to write a Java console application that receives messages from an Event Hub using the **EventProcessorHost**.

## Miscellaneous

### proton-c-sender-dotnet-framework-receiver

[This sample](./Miscellaneous/proton-c-sender-dotnet-framework-receiver/README.md) shows how to use Azure Event Hubs with clients that use different protocols. This scenario sends using an Apache Proton C++ client, and receives using the .NET Framework client.

