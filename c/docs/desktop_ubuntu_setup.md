<properties
  pageTitle="How to build the samples in Ubuntu"
  description="Build the Event Hubs SDK samples in Ubuntu"
  services="azure-iot"
  documentationCenter=".net"
  authors="dominicbetts"
  manager="timlt"
  editor=""/>

<tags
  ms.service="azure-iot"
  ms.workload="tbd"
  ms.tgt_pltfrm="na"
  ms.devlang="na"
  ms.topic="article"
  ms.date="05/29/2015"
  ms.author="dobett"/>

# How to build the samples in Ubuntu

The following procedure describes the process of building and running the Event Hubs SDK samples (**send** and **send_batch**) in an Ubuntu desktop environment. The samples enable your Ubuntu machine to act as a device that can connect to an Azure Event Hub.

## Requirements

A computer running the Ubuntu OS.

## Create an Event Hub

To run the sample applications you will need an Event Hub to which they can send messages.

To configure an Event Hub, see [Create an Event Hub](./create_event_hub.md). Be sure to make a note of the Event Hub name and the **SendRule** and **ReceiveRule** connection strings; you will need them to complete building the samples.

## Download the Azure Event Hubs SDK and prerequisites

1. Open a terminal window in Ubuntu.

2. Install the prerequisite packages by issuing the following commands in a terminal window:

	```
  sudo apt-get update
  sudo apt-get install -y curl libcurl4-openssl-dev uuid-dev uuid g++ make cmake git
	```

3. Download the SDK to the machine by issuing the following command in your terminal window:

  ```
	git clone https://github.com/Azure/azure-event-hubs.git
	```

  You will be prompted for your GitHub username and password -- if you have two-factor authentication enabled for your account, you'll need to generate and then use a personal access token in place of your password.

4. Verify that you now have a copy of our source code under the directory ~/azure-event-hubs.

## Update the send and send_batch samples

Before performing these steps, you'll need the following prerequisite information:

- Event Hub name.
- Event Hub **SendRule** connection string.

Now, configure the sample:

1. On your Ubuntu machine, open the file ~/azure-event-hubs/c/eventhub_client/samples/send/send.c in a text editor.

2. Scroll down to locate the connection information.

3. Replace the placeholder value for the **connectionString** variable with your **SendRule** Event Hub connection string. Replace the placeholder value for the **eventHubPath** variable with your Event Hub name.

4. Save your changes.

6. Repeat the previous steps, but for step 1 open the file ~/azure-event-hubs/c/eventhub_client/samples/send_batch/send_batch.c.

## Build the samples

You can now build the SDK samples using the following command:

	```
  ~/azure-event-hubs/c/build_all/linux/build.sh
	```

## Run the samples

1. Run the **send** sample by issuing the following command:

  ```
	~/azure-event-hubs/c/eventhub_client/samples/send/linux/send
	```

2. Run the **send_batch** sample by issuing the following command:

  ```
	~/azure-event-hubs/c/eventhub_client/samples/send_batch/linux/send_batch
	```

3. For each sample, verify that the sample output messages show **Successful**. If not, then you may have incorrectly pasted the Event Hub connection information.

>Note: The tools folder in this repository includes the **CSharp_ConsumeEventsFromEventHub** C# Windows application that can receive messages from an Event Hub. This is useful to help you verify that the samples are sending messages correctly to the Event Hub.
