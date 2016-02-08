#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

#this is CMakeLists for the eventhub_client tests folder

add_subdirectory(eventdata_unittests)
add_subdirectory(eventhubclient_unittests)
add_subdirectory(eventhubclient_ll_unittests)
add_subdirectory(connectionstringparser_unittests)
#add_subdirectory(eventhubclient_e2etests)
