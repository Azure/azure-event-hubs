#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for sendasync

compileAsC99()

set(sendasync_c_files
    sendasync.c
)

if(WIN32)
    set(sendasync_c_files ${sendasync_c_files} ./windows/main.c)
else()
    set(sendasync_c_files ${sendasync_c_files} ./linux/main.c)
endif()

set(sendasync_h_files
    sendasync.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)

add_executable(sendasync ${sendasync_c_files} ${sendasync_h_files})

target_link_libraries(sendasync
        eventhub_client)

linkSharedUtil(sendasync)
linkProton(sendasync)

