#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for eventhub_client

compileAsC99()

set(eventhub_client_c_files
    ./src/eventdata.c
    ./src/eventhubclient.c
    ./src/eventhubclient_ll.c
    ./src/version.c
)

set(eventhub_client_h_files
    ./inc/eventdata.h
    ./inc/eventhubclient.h
    ./inc/eventhubclient_ll.h
    ./inc/version.h
)

#these are the include folders
#the following "set" statetement exports across the project a global variable called SHARED_UTIL_INC_FOLDER that expands to whatever needs to included when using COMMON library

set(EVENTHUB_CLIENT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using eventhub_client lib" FORCE)

include_directories(${SHARED_UTIL_INC_FOLDER})

include_directories(${EVENTHUB_CLIENT_INC_FOLDER})

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

add_library(eventhub_client 
    ${eventhub_client_c_files} 
    ${eventhub_client_h_files}
)

target_link_libraries(eventhub_client uamqp)
linkSharedUtil(eventhub_client)

add_subdirectory(samples)

if (${run_unittests})
    add_subdirectory(tests)
endif()
