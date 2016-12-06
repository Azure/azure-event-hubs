#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for eventhubclient_e2etests
cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName eventhubclient_e2etests)

set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
)

include_directories(${EVENTHUB_TEST_INC_FOLDER})

build_test_artifacts(${theseTestsName} OFF)

if(WIN32)
	if(TARGET ${theseTestsName}_dll)
		target_link_libraries(${theseTestsName}_dll
			eventhub_client
			eventhub_test
			aziotsharedutil
			uamqp
			testrunnerswitcher
		)
	endif()

	if(TARGET ${theseTestsName}_exe)
		target_link_libraries(${theseTestsName}_exe
			eventhub_client
			eventhub_test
			aziotsharedutil
			uamqp
		)
	endif()
else()
	if(TARGET ${theseTestsName}_exe)
		target_link_libraries(${theseTestsName}_exe
			eventhub_client
			eventhub_test
			aziotsharedutil
		)
		target_link_libraries(${theseTestsName}_exe pthread)
	endif()
endif()

