<properties
	pageTitle="BeagleBone Black Setup"
	description="Set up board"
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
	ms.date="05/28/2015"
	ms.author="sethm"/>

# Event Hubs SDK: BeagleBone Black setup

The following procedure describes the process of connecting a [BeagleBone Black](http://beagleboard.org/black) device and building the Event Hubs SDK samples (**send** and **send_batch**).

## Requirements

- Computer with GitHub installed and access to the [azure-event-hubs](https://github.com/Azure/azure-event-hubs) GitHub private repository.
- SSH client, such as [PuTTY](http://www.putty.org/), so you can access the command line.
- Either Chrome or Firefox, so you can browse into your board.
- The boards come with all the required software; however, to update the board and run the current sample you must use FTP or a USB driver to copy your files. You can also use [WinSCP](http://winscp.net/eng/index.php).
- Required hardware: [Beagleboard-Beaglebone](http://www.amazon.com/Beagleboard-Beaglebone-Starter-Case--Power-Supply--Micro/dp/B00P6TV9V4/ref=sr_1_3?ie=UTF8&qid=1426002583&sr=8-3&keywords=beaglebone).

## Create an Event Hub

To run the sample applications you will need an Event Hub to which they can send messages.

To configure an Event Hub, see [Create an Event Hub](./create_event_hub.md). Be sure to make a note of the Event Hub name and the **SendRule** and **ReceiveRule** connection strings; you will need them to complete building the samples.

## Connect the board

1. Plug the USB cable into your computer, so you can access the board (your PC will install drivers, etc., the first time). You can use the same USB cable to power the board; it's not necessary to use the separate power block.
2. Use one of the following two options to install the Windows drivers:
	1.  Download the [x64](http://beagleboard.org/static/Drivers/Windows/BONE_D64.exe) or [x86 (32-bit)](http://beagleboard.org/static/Drivers/Windows/BONE_DRV.exe) drivers. On x64, you only need the x64 drivers.
	2.  The drivers (Linux and Windows) are already on the board. When you connect it via USB, the board appears as another drive letter called **BeagleBone Getting Started**. You can find the drivers under **[DRIVE LETTER]:\Drivers\Windows\[BONE\_D64.exe, and BONE\_DRV.exe]**.

## Verify that you can connect to the device

1. Once you see that your board is alive, use Chrome or Firefox to open **http://192.168.7.2**.
2. (Recommended)  Go to the [Interactive Guide](http://192.168.7.2/Support/BoneScript/) page and go through the basic samples. For example, turn the LEDs on, turn LEDs off, etc. Verify that your board behaves as expected.
3. Open an SSH terminal program (e.g., [PuTTY](http://www.putty.org/)).
4. Use Host name=192.168.7.2, Port=22, Connection type=SSH.
5. When prompted, log in with username **root**, no password.
6. Connect an Ethernet cable to your board. Ping a website, e.g., bing.com, to verify connectivity.
	- If ping doesn't work, reboot your board, log in again using PuTTY, and run the command **ifconfig**. You should see the **eth0**, **lo**, and **usb0** interfaces.

## Load the Azure Event Hubs SDK and prerequisites

1. Open a PuTTY session and connect to the board, as described in the previous section "Verify that you can connect to the device."
1. Install the prerequisite packages by issuing the following commands  in your PuTTY session:

		sudo apt-get update
		sudo apt-get install -y curl libcurl4-openssl-dev uuid-dev uuid g++ make cmake git

  Note: In Windows, you can right-click on a PuTTY window to paste in commands.

2. Download the SDK to the board by issuing the following command in PuTTY:

		git clone https://github.com/Azure/azure-event-hubs.git

	You will be prompted for your GitHub username and password -- if you have two-factor authentication enabled for your account, you'll need to generate/use a personal access token in place of your password.

3. Verify that you now have a copy of our source code under the directory ~/azure-event-hubs.

## Update the send and send_batch samples

Before performing these steps, you'll need the following prerequisite information:

- Event Hub name.
- Event Hub **SendRule** connection string.

Now, configure the sample:

1. On the board, run the following command:

		nano azure-event-hubs/c/eventhub_client/samples/send/send.c

2. This launches a console-based text editor. Scroll down to the connection information.

3. Replace the placeholder value for the **connectionString** variable (you can right-click a PuTTY window to paste a value) with your **SendRule** Event Hub connection string. Replace the placeholder value for the **eventHubPath** variable with your Event Hub name.

4. Save your changes by pressing **Ctrl+O**, and when nano prompts you to save it as the same file, just press **ENTER**.

5. Press **Ctrl+X** to exit nano.

6. Repeat the previous steps, but for step 1 run the following command, and paste your connection information in the same place:

		nano azure-event-hubs/c/eventhub_client/samples/send_batch/send_batch.c

## Build the samples

You can now build the SDK code using the following command:

		~/azure-event-hubs/c/build_all/linux/build.sh

## Run the samples

1. Run the **send** sample by issuing the following command:

		~/azure-event-hubs/c/eventhub_client/samples/send/linux/send

2. Run the **send_batch** sample by issuing the following command:

		~/azure-event-hubs/c/eventhub_client/samples/send_batch/linux/send_batch

3. For each sample, verify that the sample output messages show **Successful**. If not, then you may have incorrectly pasted the Event Hub connection information.

>Note: The tools folder in this repository includes the **CSharp_ConsumeEventsFromEventHub** C# application that can receive messages from an Event Hub. This is useful to help you verify that the samples are sending messages correctly to the Event Hub.
