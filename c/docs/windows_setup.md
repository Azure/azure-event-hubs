<properties
	pageTitle="How to build the Windows sample"
	description="Build the Windows sample"
	services="azure-iot"
	documentationCenter=".net"
	authors="sethmanheim"
	manager="timlt"
	editor=""/>

<tags
	ms.service="azure-iot"
	ms.workload="tbd"
	ms.tgt_pltfrm="na"
	ms.devlang="na"
	ms.topic="article"
	ms.date="05/29/2015"
	ms.author="sethm"/>

# How to build the Windows sample

The following procedure describes the process of building and running the Event Hubs SDK samples (**send** and **send_batch**) on a Windows desktop machine.

The native client library depends on Apache Qpid Proton.

## Create an Event Hub

To run the sample applications you will need an Event Hub to which they can send messages.

To configure an Event Hub, see [Create an Event Hub](./create_event_hub.md). Be sure to make a note of the Event Hub name and the **SendRule** and **ReceiveRule** connection strings; you will need them to complete building the samples.

## Install the SDK
Download the SDK to the board by issuing the following Git command in a terminal Window:

  ```
	git clone https://github.com/Azure/azure-event-hubs.git
	```
You will be prompted for your GitHub username and password -- if you have two-factor authentication enabled for your account, you'll need to generate/use a personal access token in place of your password.

## To install and build Proton on Windows

1. Create a folder on your development machine in which to download the proton libraries. This example uses the location **C:\Proton**.

2. Create an environment variable **PROTON_PATH=C:\Proton**. This path is used by Visual Studio projects to include and link Proton-C components.

3. Install [cmake](http://www.cmake.org/) (make sure it is installed in your path).

4. Install [Python ver. 2.7.9](https://www.python.org/downloads/) (install it in your path).

5. Open a Visual Studio 2013 x86 Native Tools command prompt.

6. To build the proton libraries, run the script **build__proton.cmd** in the **\build_all\windows** directory.

When the build is completed, you can use the proton libraries in your Windows applications.

## To build the Windows samples

1) In Visual Studio 2013, open the solution for the samples you want to build. The Visual Studio solution files are located in:

 - \azure-event-hubs\c\eventhub_client\samples\send\windows

	or

 - \azure-event-hubs\c\eventhub_client\samples\send_batch\windows

2) Update the Event Hub connection information in the sample application. Save your changes.

3) Rebuild the sample project (right-click the project and click **Build**).

3) Press **F5** to run the sample application, or after a successful build, run the executable located in the directory:

- \azure-event-hubs\c\eventhub_client\samples\send\windows\Win32[Debug|Release]

or

- \azure-event-hubs\c\eventhub_client\samples\send_batch\windows\Win32[Debug|Release]

>Note: The tools folder in this repository includes the **CSharp_ConsumeEventsFromEventHub** C# application that can receive messages from an Event Hub. This is useful to help you verify that the samples are sending messages correctly to the Event Hub.
