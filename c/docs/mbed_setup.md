<properties
	pageTitle="mbed Freescale K64F setup"
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

# Freescale K64F Setup

The following procedure describes the process of connecting an [mbed-enabled Freescale K64F](http://developer.mbed.org/platforms/IBMEthernetKit) device and building the Event Hubs SDK samples (**send** and **send_batch**).

## Requirements

- Computer with GitHub installed, so that you can access the [azure-event-hubs](https://github.com/Azure/azure-event-hubs) code.
- SSH client, such as [PuTTY](http://www.putty.org/), so you can access the command line.
- Required hardware: [mbed-enabled Freescale K64F](http://developer.mbed.org/platforms/IBMEthernetKit).

## Create an Event Hub

To run the sample applications you will need an Event Hub to which they can send messages.

To configure an Event Hub, see [Create an Event Hub](./create_event_hub.md). Be sure to make a note of the Event Hub name and the **SendRule** and **ReceiveRule** connection strings; you will need them to complete building the samples.

## Connect the device

1. Connect the board to your network using an Ethernet cable. This step is required, as the sample depends on internet access.

2. Plug the device into your computer using a micro-USB cable. Be sure to attach the cable to the correct USB port on the device, as pictured [here](https://developer.mbed.org/platforms/IBMEthernetKit/), in the "Getting started" section.

2. Install the Windows serial port drivers located [here](http://developer.mbed.org/handbook/Windows-serial-configuration#1-download-the-mbed-windows-serial-port).

3. Install the 7-Zip software from [www.7-zip.org](http://www.7-zip.org) and place 7z.exe in your environment PATH variable.

## Build the send sample

**Note** You can use the following procedure for both samples (**send** and **send_batch**). These steps build the **send** sample, but you can replace **send** with **send_batch** to build that sample.

1. Navigate to the [GitHub SDK](https://github.com/Azure/azure-event-hubs) repository, in the folder **\azure-event-hubs\c\eventhub_client\samples\send\mbed**.

2. Run the "mkmbedzip.bat" file. This will generate a "send.zip" file in the same folder. NOTE: When the zip file is uploaded to the compiler web page, the browser will keep the zip file open. Attempting to create another zip while your browsers are open will fail.

3. In your web browser, go to the mbed.org site. Select the **Developer Site** in the upper right-hand corner. If you haven't signed up, you will see an option to create a new account (it's free). Otherwise, log in with your account credentials. Then click on **Compiler** in the upper right-hand corner of the page. This should bring you to the Workspace Management interface.

4. Make sure the hardware platform you're using appears in the upper right-hand corner of the window, or click the icon in the right-hand corner to select your hardware platform.

5. Click **New**, then **New Program...**

	![][13]

7. The **Create new program** dialog is displayed. The platform field should be pre-populated with the hardware platform you selected. Make sure the **Template** field is set to **Empty Program**. Use any program name you want in the **Program Name** field, then click **OK**.

	![][1]

6. Click **Import** on the main menu. Then click the **Upload** tab. Click the **Choose File** button at the bottom of the page. Browse to the directory in which you created your zip file and double-click it. You will be back at the **Import Wizard** page, and the zip file name is displayed in the **Name** column. Click the large **Import!** button in the upper right-hand corner of the page. The **Import Library** dialog is displayed. Ensure that the target path matches the name of the project you created and then click **Import**. NOTE: The target path matches the name of the program that was last selected before **Import** was selected from the main menu.

	![][2]

7. Open the send.c file, and replace the highlighted lines of code with your Event Hub name and connection string (which you can obtain from the Azure management portal):

	![][14]

7. Ensure that your project is highlighted in the **Program Workspace** page and click the **Import** menu item again. At the top of the window, click the **Click Here** link to import from a URL. The **Import Library** dialog appears.

	![][3]

8. Enter the following URL into the **Source URL** field: `http://developer.mbed.org/users/donatien/code/NTPClient/`, and then click **Import**.

	![][8]

9. Repeat the previous step for each of the following libraries. Be sure to highlight the project name before importing the URL:
	- http://developer.mbed.org/users/AzureIoTClient/code/TLS_CyaSSL_no_cert/
	- http://developer.mbed.org/users/mbed_official/code/EthernetInterface/
	- http://developer.mbed.org/users/mbed_official/code/mbed-rtos/
	- http://mbed.org/users/mbed_official/code/mbed/
	- http://developer.mbed.org/users/AzureIoTClient/code/proton-c-mbed/

## Build and run the program

1. Click **Compile** to build the program. You can safely ignore any warnings, but if the build generates errors, fix them before proceeding.

	![][9]

2. If the build is successful, a .bin file with the name of your project is generated. Copy the .bin file to the device. Saving the .bin file to the device causes the current terminal session to the device to reset. When it reconnects, reset the terminal again manually, or start a new terminal. This enables the mbed device to reset and start executing the program.

3. Connect to the device using an SSH client application, such as PuTTY. You can determine which serial port your device uses by checking the Windows Device Manager:

	![][17]

4. In PuTTY, click the **Serial** connection type. The device most likely connects at 115200, so enter that value in the **Speed** box. Then click **Open**:

	![][18]

The send program starts executing. You may have to reset the board (press CTRL+Break) if the program does not start automatically when you connect.

5. Repeat the above procedure for the send_batch sample.

>Note: The tools folder in this repository includes the **CSharp_ConsumeEventsFromEventHub** C# application that can receive messages from an Event Hub. This is useful to help you verify that the samples are sending messages correctly to the Event Hub.

[1]: ./media/mbed_setup/mbed1.png
[2]: ./media/mbed_setup/mbed3.png
[3]: ./media/mbed_setup/mbed4.png
[8]: ./media/mbed_setup/mbed8.png
[9]: ./media/mbed_setup/mbed9.png
[13]: ./media/mbed_setup/mbed13.png
[14]: ./media/mbed_setup/mbed14.png
[17]: ./media/mbed_setup/mbed17.png
[18]: ./media/mbed_setup/mbed18.png
