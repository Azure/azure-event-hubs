#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
project(eventhub_client)

#the following variables are project-wide and can be used with cmake-gui
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF) [if possible, they are always build]" OFF)
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(compileOption_C "passes a string to the command line of the C compiler" OFF)
option(compileOption_CXX "passes a string to the command line of the C++ compiler" OFF)

#Use solution folders. 
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions(-DREFCOUNT_ATOMIC_DONTCARE)
add_definitions(-D__STDC_NO_ATOMICS__=1)

if(IN_OPENWRT)
    ADD_DEFINITIONS("$ENV{TARGET_LDFLAGS}" "$ENV{TARGET_CPPFLAGS}" "$ENV{TARGET_CFLAGS}")
    INCLUDE_DIRECTORIES("$ENV{TOOLCHAIN_DIR}/usr/include" "$ENV{TARGET_LDFLAGS}" "$ENV{TARGET_CPPFLAGS}" "$ENV{TARGET_CFLAGS}")
endif()

add_subdirectory(azure-c-shared-utility)
add_subdirectory(azure-uamqp-c)

include_directories(${UAMQP_INC_FOLDER})

enable_testing()

#this project uses several other projects that are build not by these CMakeFiles
#this project also targets several OSes

include(CheckSymbolExists)
function(detect_architecture symbol arch)
    if (NOT DEFINED ARCHITECTURE OR ARCHITECTURE STREQUAL "")
        set(CMAKE_REQUIRED_QUIET 1)
        check_symbol_exists("${symbol}" "" ARCHITECTURE_${arch})
        unset(CMAKE_REQUIRED_QUIET)

        # The output variable needs to be unique across invocations otherwise
        # CMake's crazy scope rules will keep it defined
        if (ARCHITECTURE_${arch})
            set(ARCHITECTURE "${arch}" PARENT_SCOPE)
            set(ARCHITECTURE_${arch} 1 PARENT_SCOPE)
            add_definitions(-DARCHITECTURE_${arch}=1)
        endif()
    endif()
endfunction()
if (MSVC)
    detect_architecture("_M_AMD64" x86_64)
    detect_architecture("_M_IX86" x86)
    detect_architecture("_M_ARM" ARM)
else()
    detect_architecture("__x86_64__" x86_64)
    detect_architecture("__i386__" x86)
    detect_architecture("__arm__" ARM)
endif()
if (NOT DEFINED ARCHITECTURE OR ARCHITECTURE STREQUAL "")
    set(ARCHITECTURE "GENERIC")
endif()
message(STATUS "eventhub architecture: ${ARCHITECTURE}")

#if any compiler has a command line switch called "OFF" then it will need special care
if(NOT "${compileOption_C}" STREQUAL "OFF")
    set(CMAKE_C_FLAGS "${compileOption_C} ${CMAKE_C_FLAGS}")
endif()

if(NOT "${compileOption_CXX}" STREQUAL "OFF")
    set(CMAKE_CXX_FLAGS "${compileOption_CXX} ${CMAKE_CXX_FLAGS}")
endif()


#this project uses several other projects that are build not by these CMakeFiles
#this project also targets several OSes

function(linkSharedUtil whatIsBuilding)
    target_link_libraries(${whatIsBuilding} aziotsharedutil)
endfunction(linkSharedUtil)

macro(compileAsC99)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
    if (NOT IN_OPENWRT)
      set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()
    endif()
  else()
    set (CMAKE_C_STANDARD 99)
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(compileAsC99)

macro(compileAsC11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=c11 ${CMAKE_C_FLAGS}")
      set (CMAKE_C_FLAGS "-D_POSIX_C_SOURCE=200112L ${CMAKE_C_FLAGS}")
      set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()
  else()
    set (CMAKE_C_STANDARD 11)
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(compileAsC11)

function(windows_unittests_add_dll whatIsBuilding)	
    link_directories(${whatIsBuilding}_dll ${MICROMOCK_UNITTEST_LIB_DIR} ${CTEST_LIB_DIR} ${SHARED_UTIL_LIB_DIR} $ENV{VCInstallDir}UnitTest/lib)
    
    add_library(${whatIsBuilding}_dll SHARED 
        ${${whatIsBuilding}_cpp_files} 
        ${${whatIsBuilding}_h_files} 
        ${${whatIsBuilding}_c_files}
    )
    
    #Add UnitTests to their own folder
    set_target_properties(${whatIsBuilding}_dll
               PROPERTIES
               FOLDER "UnitTests") 
               
    target_include_directories(${whatIsBuilding}_dll PUBLIC ${sharedutil_include_directories} $ENV{VCInstallDir}UnitTest/include)
    target_compile_definitions(${whatIsBuilding}_dll PUBLIC -DCPP_UNITTEST)
    target_link_libraries(${whatIsBuilding}_dll micromock_cpp_unittest ctest ${ARGN})
endfunction()

function(windows_unittests_add_exe whatIsBuilding)
    link_directories(${whatIsBuilding}_exe ${MICROMOCK_UNITTEST_LIB_DIR} ${CTEST_LIB_DIR} ${SHARED_UTIL_LIB_DIR} $ENV{VCInstallDir}UnitTest/lib)
    
    add_executable(${whatIsBuilding}_exe
        ${${whatIsBuilding}_cpp_files} 
        ${${whatIsBuilding}_h_files} 
        ${${whatIsBuilding}_c_files}
        ${CMAKE_CURRENT_LIST_DIR}/main.c
    )
    
    #Add UnitTests to their own folder
    set_target_properties(${whatIsBuilding}_exe
               PROPERTIES
               FOLDER "UnitTests")
    
    target_compile_definitions(${whatIsBuilding}_exe PUBLIC -DUSE_CTEST)
    target_include_directories(${whatIsBuilding}_exe PUBLIC ${sharedutil_include_directories})
    target_link_libraries(${whatIsBuilding}_exe micromock_ctest ctest ${ARGN})
    add_test(NAME ${whatIsBuilding} COMMAND ${whatIsBuilding}_exe)
endfunction()

function(windows_unittests_add_lib whatIsBuilding)	
    link_directories(${whatIsBuilding}_lib ${MICROMOCK_UNITTEST_LIB_DIR} ${CTEST_LIB_DIR} ${SHARED_UTIL_LIB_DIR} $ENV{VCInstallDir}UnitTest/lib)
    
    add_library(${whatIsBuilding}_lib STATIC 
        ${${whatIsBuilding}_cpp_files} 
        ${${whatIsBuilding}_h_files} 
        ${${whatIsBuilding}_c_files}
    )
    
        #Add UnitTests to their own folder
    set_target_properties(${whatIsBuilding}_lib
               PROPERTIES
               FOLDER "UnitTests")
               
    target_include_directories(${whatIsBuilding}_lib PUBLIC ${sharedutil_include_directories})
    target_compile_definitions(${whatIsBuilding}_lib PUBLIC -DUSE_CTEST)
    target_link_libraries(${whatIsBuilding}_lib micromock_ctest ctest ${ARGN})
endfunction()

add_subdirectory(testtools)

set(SHARED_UTIL_ADAPTER_FOLDER "${CMAKE_CURRENT_LIST_DIR}/azure-c-shared-utility/adapters")
if(WIN32)
    set(LOCK_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/lock_win32.c)
    set(THREAD_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/threadapi_win32.c)
else()
    set(LOCK_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/lock_pthreads.c)
    set(THREAD_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/threadapi_pthreads.c)
endif()

add_subdirectory(eventhub_client)
