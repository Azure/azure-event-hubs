#Azure Event Hubs Client for C (C99) 
====================

The C Version of this Azure Event Hub Client SDK is located under the folder [root]/c/.

The EventHubClient “C” library provides developers a means of connecting to an already created Event Hub and the ability to send data to it. 

The library includes the following features:
* The library communicates to an existing Event Hub over AMQP protocol.
* The library uses Proton-C to establish the AMQP communication necessary.
* Buffers data when network connection is down.
* Batches messages to improve communication efficiency.


The library code:
* Is written in ANSI C (C99) to maximize code portability.
* Avoids compiler extensions.
* Its output is a static lib.
* Exposes a platform abstraction layer to isolate OS dependencies (HTTPAPI). Refer to the porting guide for more information.


The library provides the following APIs:
* EventHubClientLib_CreateFromConnectionString
* EventHubClientLib_Send
* EventHubClientLib_SendAsync
* EventHubClientLib_SendBatch
* EventHubClientLib_SendBatchAsync
* EventHubClientLib_Destroy


#Tested platforms
The following platforms have been tested against this library:
* Windows 7
* Windows 8.1
* Ubuntu 14.04 LTS
* Fedora 20
* Debian 7.5
* Ubuntu Snappy 
* Raspbian (tested device Raspberry Pi 2)


#Directory structure of repository

#build_all
This folder contains platform-specific build scripts for the client library and dependent components

#docs
Contains device getting started and setup documentation.

#common
Contains components which are not specific to Event Hubs. It includes the following subdirectories:
 
   * build: Contains one subfolder for each platform (e.g. Windows, Linux). Subfolders contain makefiles, batch files, solutions that are used to generate the library binaries.
   * docs: Requirements, designs notes, manuals.
   * inc: Public include files.
   * src: Client library source files.
   * tools: Tools used for libraries.
   * unittests: Unit tests for source code.


#eventhub_client 

This folder contains Azure Event Hubs client components.

#testtools

Contain tools that are currently used in testing the client libraries: Mocking Framework (micromock), Generic Test Runner (CTest), Unit Test Project Template, etc.

#tools
Miscellaneous tools, e.g., Event Hubs data viewer.
