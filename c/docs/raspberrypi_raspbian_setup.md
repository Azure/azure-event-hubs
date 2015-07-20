<properties
  pageTitle="Raspberry Pi 2 Raspbian Setup"
  description="Set up board"
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
  ms.date="06/23/2015"
  ms.author="dobett"/>

# Event Hubs SDK: Raspberry Pi 2 Raspbian setup

The following procedure describes the process of connecting a [Raspberry Pi 2](http://beagleboard.org/black) device running the Raspbian OS and building the Event Hubs SDK samples (**send** and **send_batch**).

## Requirements

- SSH client on your desktop computer, such as [PuTTY](http://www.putty.org/), so you can remotely access the command line on the Raspberry Pi.
- Required hardware:
  - [Raspberry Pi 2](http://www.amazon.com/Raspberry-Pi-Model-Project-Board/dp/B00T2U7R7I/ref=sr_1_1?ie=UTF8&qid=1429516842&sr=8-1&keywords=raspberry+pi)
  - 8GB MicroSD Card
  - A USB keyboard
  - A USB mouse
  - A USB Mini cable
  - A 5 Volt - 1 Amp USB power supply
  - An HDMI cable
  - TV/ Monitor that supports HDMI
  - An Ethernet cable or WiFi dongle

Note: You may wish to consider a Starter Kit such as [CanaKit](http://www.amazon.com/CanaKit-Raspberry-Complete-Original-Preloaded/dp/B008XVAVAW/ref=sr_1_4?ie=UTF8&qid=1429516842&sr=8-4&keywords=raspberry+pi) that includes some of these hardware requirements.

## Create an Event Hub

To run the sample applications you will need an Event Hub to which they can send messages.

To configure an Event Hub, see [Create an Event Hub](./create_event_hub.md). Be sure to make a note of the Event Hub name and the **SendRule** and **ReceiveRule** connection strings; you will need them to complete building the samples.

## Prepare the Raspberry Pi 2 device

1. Install the latest Raspbian operating system on your Raspberry Pi 2 by
following the instructions in the [NOOBS setup
guide](http://www.raspberrypi.org/help/noobs-setup/).
2. When the install process is complete, the Raspberry Pi configuration menu
(raspi-config) loads. Here you can perform tasks such as: set the time and date for your region, enable a Raspberry Pi camera board, and create users. Under **Advanced
Options** make sure to enable **ssh** so you can access the device remotely with
PuTTY or WinSCP from your desktop machine. For more information, see
https://www.raspberrypi.org/documentation/remote-access/ssh/.
3. Connect your Raspberry Pi to your network either by using an ethernet cable or by using a WiFi dongle on the device.
4. You need to discover the IP address of your Raspberry Pi before your can
connect using PuTTY. For more information see
https://www.raspberrypi.org/documentation/troubleshooting/hardware/networking/ip-address.md.
5. Once you see that your board is alive, open an SSH terminal program such as [PuTTY](http://www.putty.org/) on your desktop machine.
6. Use the IP address from step 4 as the Host name, Port=22, and Connection type=SSH to complete the connection.
7. When prompted, log in with username **pi**, and password **raspberry**.

## Download the Azure Event Hubs SDK and prerequisites

1. Open a PuTTY session and connect to the board, as described in the previous section.

2. Install the prerequisite packages by issuing the following commands in your PuTTY session:

	```
  sudo apt-get update
  sudo apt-get install -y curl libcurl4-openssl-dev uuid-dev uuid g++ make cmake git
	```

  Note: In Windows, you can right-click on a PuTTY window to paste in commands.

3. Download the SDK to the board by issuing the following command in your PuTTY session:

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

1. On the board, run the following command:

  ```
	nano azure-event-hubs/c/eventhub_client/samples/send/send.c
	```

2. This launches a console-based text editor. Scroll down to the connection information.

3. Replace the placeholder value for the **connectionString** variable (you can right-click a PuTTY window to paste a value) with your **SendRule** Event Hub connection string. Replace the placeholder value for the **eventHubPath** variable with your Event Hub name.

4. Save your changes by pressing **Ctrl+O**, and when nano prompts you to save it as the same file, just press **ENTER**.

5. Press **Ctrl+X** to exit nano.

6. Repeat the previous steps, but for step 1 run the following command, and paste your connection information in the same places:

  ```
	nano azure-event-hubs/c/eventhub_client/samples/send_batch/send_batch.c
	```

## Build the samples

1. On the board, run the following command to build and install the Apache Proton libraries:

  ```
	sudo ~/azure-event-hubs/c/build_all/linux/build_proton.sh --install /usr
	```

5. Assuming build\_proton.sh completed successfully, you can now build the SDK samples using the following command:

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

>Note: The tools folder in this repository includes the **CSharp_ConsumeEventsFromEventHub** C# application that can receive messages from an Event Hub. This is useful to help you verify that the samples are sending messages correctly to the Event Hub.
