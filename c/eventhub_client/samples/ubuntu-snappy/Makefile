CHROOTNAME = vivid-armhf
CHROOTPATH = /build/arm/azure-event-hubs/c/eventhub_client/samples/ubuntu-snappy

all: compile libs
	mkdir -p loadavg_snap/bin/x86_64-linux-gnu
	cp src/eventhub_demo_x64 loadavg_snap/bin/x86_64-linux-gnu/eventhub_demo
	mkdir -p loadavg_snap/bin/arm-linux-gnueabihf
	cp src/eventhub_demo_arm loadavg_snap/bin/arm-linux-gnueabihf/eventhub_demo
	snappy build loadavg_snap

compile:
	cd src; ./build.sh x64
	schroot -c $(CHROOTNAME) -d $(CHROOTPATH)/src -- build.sh arm

libs:
	mkdir -p loadavg_snap/lib/x86_64-linux-gnu
	cp /usr/local/lib/libqpid-proton.so.2 loadavg_snap/lib/x86_64-linux-gnu
	mkdir -p loadavg_snap/lib/arm-linux-gnueabihf
	schroot -c $(CHROOTNAME) -d $(CHROOTPATH) -- cp /usr/local/lib/libqpid-proton.so.2 loadavg_snap/lib/arm-linux-gnueabihf

clean:
	rm -f src/eventhub_demo_arm loadavg_snap/bin/arm-linux-gnueabihf/eventhub_demo
	rm -f src/eventhub_demo_x64 loadavg_snap/bin/x86_64-linux-gnu/eventhub_demo
	rm -f loadavg_snap/lib/arm-linux-gnueabihf/* loadavg_snap/lib/x86_64-linux-gnu/*
	rm -f *.snap
