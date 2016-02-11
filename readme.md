#Microsoft Azure Event Hubs Clients

This project provides client-side libraries to enable easy interaction with Microsoft Azure Event Hubs.


C Version 
====================

The C Version of this Azure Event Hub Client SDK is located under the folder [root]/c/.

The EventHubClient “C” library provides developers a means of connecting to an already created Event Hub and the ability to send data to it. 

The library includes the following features:
* The library communicates to an existing Event Hub over AMQP protocol (using uAMQP).
* Buffers data when network connection is down.
* Supports batching.


The library code:
* Is written in ANSI C (C99) to maximize code portability.
* Avoids compiler extensions.
* Its output is a static lib.

The library has a dependency on azure-uamqp-c and azure-c-shared-utility. They are used as submodules.
When switching branches remember to update the submodules by:

```
git submodule update --init --recursive
```

#Building it

* Create a folder named build under the c directory
* Run cmake ..
* Build

Node.js Version 
====================

The Node version is split into two pieces, both under the `[root]/node` directory. The `send_receive` directory contains the `azure-event-hubs` npm, with the ability to create an `EventHubClient` and `Sender` and `Receiver` from that. The `event_processor_host` directory (TBD) contains code to mimic and interoperate with the .NET `EventProcessorHost` class, managing receivers for each partition via blob leases and allowing checkpointing of offsets for easy restarts.

Please see the `README.md` in each directory for usage and additional details - they are managed there to ensure appropriate documentation in the npm releases. 
