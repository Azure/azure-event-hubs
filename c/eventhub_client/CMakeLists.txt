#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.0)
#this is CMakeLists for eventhub_client

compileAsC99()

set(eventhub_client_c_files
./src/eventdata.c
./src/eventhub_client.c
./src/eventhub_client_ll.c
./src/version.c
)

set(eventhub_client_h_files
./src/eventdata.h
./inc/eventhub_client.h
./inc/eventhub_client_ll.h
./inc/version.h
)

#these are the include folders
#the following "set" statetement exports across the project a global variable called SHARED_UTIL_INC_FOLDER that expands to whatever needs to included when using COMMON library

include_directories(${SHARED_UTIL_INC_FOLDER})

includeProton()

include_directories(${EVENTHUB_CLIENT_INC_FOLDER})

IF(WIN32)
	#windows needs this define
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

add_definitions(-Dqpid_proton_EXPORTS)

add_library(eventhub_client 
	${eventhub_client_c_files} 
	${eventhub_client_h_files}
)
linkSharedUtil(eventhub_client)

add_subdirectory(samples)

if(NOT IN_OPENWRT)
# Disable tests for OpenWRT
add_subdirectory(unittests)
endif()
