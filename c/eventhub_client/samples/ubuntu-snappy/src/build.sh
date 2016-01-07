#!/bin/bash

TOPDIR=/var/lib/sbuild/build
CHRDIR=/build

ARMDIR=$CHRDIR/arm
X64DIR=$TOPDIR/x64

ARMCLIENT=$ARMDIR/azure-event-hubs/c
X64CLIENT=$X64DIR/azure-event-hubs/c

ARMINC="-I$ARMCLIENT/eventhub_client/inc -I$ARMCLIENT/common/inc"
X64INC="-I$X64CLIENT/eventhub_client/inc -I$X64CLIENT/common/inc"

ARMLIB="-L $ARMCLIENT/common/build/linux -lcommon -L $ARMCLIENT/eventhub_client/build/linux -leventhub_client"
X64LIB="-L $X64CLIENT/common/build/linux -lcommon -L $X64CLIENT/eventhub_client/build/linux -leventhub_client"

build_arm()
{
    echo "Building for arm..."
    cc $ARMINC eventhub_demo.c $ARMLIB -L/usr/local/lib -lpthread -lcurl -lqpid-proton -o eventhub_demo_arm
}

build_x64()
{
    echo "Building for x64..."
    cc $X64INC eventhub_demo.c $X64LIB -L/usr/local/lib -lpthread -lcurl -lqpid-proton -o eventhub_demo_x64
}

if [ -z "$1" ]
then
    echo "$0 arm or x64"
    exit 1
fi

if [ "$1" == "arm" ]
then
    build_arm
else [ "$1" == "x64" ]
    build_x64
fi

