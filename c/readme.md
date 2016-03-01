
# Azure Event Hub Client for C

The Azure Event Hubs Client for C is a library specifically aimed at clients looking to send event data 
into an existing Event Hub. The library builds on the compact uAMQP library for AMQP 1.0 and we include build
and usage instructions for a variety of Linux flavors and also for Windows.

As the primary audience for this library is the embedded devices development community, the library also 
provides some level of insulation against intermittent network availability interruptions common with 
devices connected via wireless radio networks of any kind. 

The library supports sending individual events as well as batched event submission.

To maximize portability, the code is written in ANSI C (C99) and avoids any compiler extensions. The build 
output is a static library.b.

The library has a dependency on azure-uamqp-c and azure-c-shared-utility; those projects are external to this project, 
and referenced as [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules). 

When switching branches in this repository, remember to update the submodules by:

```
git submodule update --init --recursive
```

## Building the library

The build requires [CMake 2.8.11 or better](https://cmake.org/) and a CMake supported C/C++ compiler and linker
on the target platform. CMake will create all required files for building the library with the chosen tool chain.
 
1.  Create a folder named "build" underneath the "c" directory
2.  Run ```cmake ..```
3.  Build. The build process will vary by platform; for Linux the default is "make". 

## Samples

The build process will also build the available samples, which reside under [samples](./samples) for your review. 

Specific instructions are available for the following platforms:

* [Beaglebone Black](.\docs\beagleboneblack_debian_setup.md)
* [Fedora Linux](.\docs\desktop_fedora_setup.md)
* [Ubuntu Linux](.\docs\desktop_ubuntu_setup.md)
* [Raspbian Linux](.\docs\raspberrypi_raspbian_setup.md)
* [Windows](.\docs\windows_setup.md) 

> Several of these documents are known to be outdated 

