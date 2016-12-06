#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists for eventhub_test

compileAsC99()

set(eventhub_test_c_files
    ./src/eventhub_account.c
)

set(eventhub_test_h_files
    ./inc/eventhub_account.h
)

#these are the include folders
#the following "set" statement exports across the project a global variable called EVENTHUB_TEST_INC_FOLDER that expands to whatever needs to included when using eventhub_test library
set(EVENTHUB_TEST_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using eventhub_test" FORCE)

include_directories(${SHARED_UTIL_INC_FOLDER})

include_directories(./inc)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

add_library(eventhub_test ${eventhub_test_c_files} ${eventhub_test_h_files})
