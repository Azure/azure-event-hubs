#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for send_batch

compileAsC99()

set(send_batch_c_files
    send_batch.c
)

if(WIN32)
    set(send_batch_c_files ${send_batch_c_files} ./windows/main.c)
else()
    set(send_batch_c_files ${send_batch_c_files} ./linux/main.c)
endif()

set(send_batch_h_files
    send_batch.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)

add_executable(send_batch ${send_batch_c_files} ${send_batch_h_files})

target_link_libraries(send_batch
        eventhub_client)

linkSharedUtil(send_batch)
linkProton(send_batch)

