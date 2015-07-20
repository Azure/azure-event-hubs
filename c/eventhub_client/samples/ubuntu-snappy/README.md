# Ubuntu Snappy sample

This sample shows how to use the Event Hubs Client SDK C library to build an Ubuntu "Snappy" Core application. The application will be able to run both on amd64 (i.e. Ubuntu Core running in a VM) and ARM (i.e. Ubuntu Core running on a BeagleBone Black or Raspberry Pi 2).

# Compiling the sample from source code

Before you can compile the Ubuntu Core Snappy sample, you need to compile the Client SDK C library (and its pre-requisites) for both ARM and X64. To that end, you should first prepare a cross-compiling environment that will allow you to build for both hardware architectures from a single Ubuntu VM.

The article "[cross-compilation on Ubuntu using sbuild](http://hypernephelist.com/2015/03/09/cross-compilation-made-easy-on-ubuntu-with-sbuild.html)" contains the instructions on how to setup this environment. Please first read this article and follow the instructions! Do not compile the Qpid Proton library yet, as this will be included in the Client SDK build process.

## Compiling the Event Hubs C library for ARM

Let's compile the Azure C library for the ARM target, using the chroot environment described in the article above. This will give us an ARM build of the library.

You should follow the same process as described in the article for Proton:

- Clone or copy the GitHub project to the `/build` directory in the chroot, via the mirror directory in `/var/lib/sbuild/build`. I suggest creating a directory "arm" to keep all ARM binaries together.

```
cd /var/lib/sbuild/build
mkdir arm
cd arm
git clone https://github.com/Azure/azure-event-hubs
```

- Use the `sbuild-shell` command to enter the chroot:

```
sudo sbuild-shell vivid-armhf
```

- You are now in the ARM chroot and you can use normal Ubuntu commands to install the pre-requisites for Qpid Proton:

```
apt-get install python cmake git
apt-get install libssl-dev uuid-dev libcurl4-openssl-dev
```

- You can then build the C library from the `build_all/linux` directory:

```
cd /build/arm/azure-event-hubs/c/build_all/linux
bash ./build_proton.sh -s /build/arm/qpid-proton -i /usr/local
bash ./build.sh -c
```

Everything should compile nicely for ARM.

## Compiling the Event Hubs C library for x64

Building for x64 is of course straightforward: you will work in the host itself, not the ARM chroot. You only have to make sure to keep both arm and x64 binaries in separate locations so you can reference them separately, e.g.:

```
/var/lib/sbuild/build
	/arm/
		azure-event-hubs
		qpid-proton
	/x64/
		azure-event-hubs
		qpid-proton
```

You will need to install the Proton pre-requisites on the host (use the same commands as above for the ARM chroot).

```
sudo apt-get install python cmake git
sudo apt-get install libssl-dev uuid-dev libcurl4-openssl-dev
```

Then clone the library and compile Proton like this:

```
cd /var/lib/sbuild/build
mkdir x64
cd x64
git clone https://github.com/Azure/azure-event-hubs
cd azure-event-hubs/c/build_all/linux
./build_proton.sh -s /var/lib/sbuild/build/x64/qpid-proton -i /usr/local
```

"make install" will fail because of privileges, go to `/var/lib/sbuild/build/x64/qpid-proton/build` and run `sudo make install`.

Now Proton is installed in the same location in both environments (`/usr/local`).

You can then compile the C library in the same way as before:

```
./build.sh -c
```

You now have two copies of the Azure Event Hubs Client SDK, one compiled for ARM and one for x64.

## Building the Snappy sample client

Based on the above organisation, the Snappy sample contains a build script you can use to build the arm and x64 executables from the same machine.

To build the Snappy application, first install the appropriate development tools:

```
sudo add-apt-repository ppa:snappy-dev/tools
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install snappy-tools
```

The sample source code is in `azure-event-hubs/c/eventhub_client/samples/ubuntu-snappy` and you can build it from either the arm or x64 directory.

Check the paths in `src/build.sh` to make sure they point to the right locations for both arm (chroot) and x64.

To build the executables for x64:

```
cd src
./build.sh x64
```

To build the executables for ARM:

```
sudo sbuild-shell vivid-armhf
cd /build/arm/azure-event-hubs/c/eventhub_client/samples/ubuntu-snappy/src
./build.sh arm
```

To build the snap, from the top-level directory, just run "make" (make sure the paths in the Makefile are correct). The Makefile will build the binaries and copy the Qpid Proton libraries into the snap. This should produce a "snappy" package, e.g.:

- `azureloadavg_1.2_all.snap`

# Running the sample

You can install this package on an Ubuntu Core machine manually or using `snappy-remote`. The packages are configured to run as services and should start automatically sending telemetry data to Event Hubs.

Create a configuration file e.g. `conf.yaml` that contains your Service Bus connection string and Event Hub name. There is a sample `example-conf.yaml` file at the root of this directory. Just replace the sample Event Hubs connection string with your own, and enter the name of your Event Hub in the `eventhubname` parameter.

Install the Snap on an Ubuntu Core system:

```
sudo snappy install azureloadavg_1.2_all.snap example-conf.yaml --allow-unauthenticated
```

If you need to remove the snap:

```
sudo snappy remove azureloadavg
```

To troubleshoot, you can follow the system log, which should show any error messages:

```
sudo journalctl -f
```

