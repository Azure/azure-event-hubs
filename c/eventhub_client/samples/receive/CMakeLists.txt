#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for receive

compileAsC99()

set(receive_c_files
    ./receive.c
)

set(receive_h_files
    receive.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)

add_executable(receive ${receive_c_files} ${receive_h_files})

target_link_libraries(receive
        eventhub_client uamqp)

linkSharedUtil(receive)
