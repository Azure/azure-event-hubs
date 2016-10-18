#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for eventdata_unittests
cmake_minimum_required(VERSION 2.8.11)

compileAsC99()
set(theseTestsName eventdata_unittests)
set(${theseTestsName}_cpp_files
${theseTestsName}.cpp
)

set(${theseTestsName}_c_files
../../src/eventdata.c
${SHARED_UTIL_SRC_FOLDER}/xlogging.c
${SHARED_UTIL_SRC_FOLDER}/consolelogger.c
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} ON)

target_link_libraries(${theseTestsName}_dll testrunnerswitcher)
