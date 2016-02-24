
#Azure Event Hub Client for C

This C programmiong language library provides developers a means of connecting to an already created Event Hub and the ability to send data to it. 

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

