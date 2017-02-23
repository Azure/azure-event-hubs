# proton-c-sender-dotnet-framework-receiver

## Overview

This sample shows how to use Azure Event Hubs with clients that utilize different protocols. This scenario sends using an Apache Proton C++ client, and receives using the .NET Framework client.

The hello_world sample sends 200 messages to an Event Hub.

Follow the instructions here to build and install Proton.
https://git-wip-us.apache.org/repos/asf?p=qpid-proton.git;a=blob;f=INSTALL.md;hb=0.17.0

## Build the sample

You may need to modify the compiler options based on your config.

`g++ -L/usr/lib64 -lqpid-proton-cpp -o sender helloworld.cpp`

## Run the sample

Replace SASKeyName, SASKey, YourNamespace and YourEventHub with the values from your setup.

Note that SASKeyName and SASKey need to be URL encoded.

`./sender amqps://SASKeyName:SASKey@YourNamespace.servicebus.windows.net/YourEventHub`

This example was based on the Proton C++ sample located here:
https://qpid.apache.org/releases/qpid-proton-0.17.0/proton/cpp/examples/helloworld.cpp.html

## Contributing

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
