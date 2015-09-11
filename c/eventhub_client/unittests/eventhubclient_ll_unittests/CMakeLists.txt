
# ----------------------------------------
# eventhubclient_ll_unittest
# ----------------------------------------
add_definitions(-DUSE_CTEST)
add_definitions(-DPROTON_DECLARE_STATIC)
add_definitions(-DGB_DEBUG_ALLOC)
add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS)

file(GLOB_RECURSE EVENTHUBCLIENT_LL_UNITTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../src/eventhubclient_ll.c)

list(SORT EVENTHUBCLIENT_LL_UNITTEST_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}" ${EVENTHUBCLIENT_LL_UNITTEST_SOURCES})
set_source_files_properties(EVENTHUBCLIENT_LL_UNITTEST_SOURCES PROPERTIES LANGUAGE CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/testrunnerswitcher/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/micromock/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/ctest/inc)
IF (NOT MSVC)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/sal/inc)
ENDIF()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../common/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../common/src)

find_package(Threads REQUIRED)

add_executable (eventhubclient_ll_unittest ${EVENTHUBCLIENT_LL_UNITTEST_SOURCES})
set_target_properties(eventhubclient_ll_unittest PROPERTIES LINKER_LANGUAGE CXX)

IF (${CMAKE_MAJOR_VERSION} LESS 3 )
  set( micromock ${micromock_location})
  set( ctest ${ctest_location})
  set( eventhub_common ${eventhub_common_location})
ELSE()
  set(micromock  $<TARGET_FILE:micromock>) 
  set(ctest  $<TARGET_FILE:ctest>)
  set(eventhub_common  $<TARGET_FILE:eventhub_common>) 
ENDIF()




# Libraries to link in reverse order because that's what ld requires.
target_link_libraries (eventhubclient_ll_unittest
    ${micromock}  
    ${ctest}
    ${eventhub_common}  
    ${CMAKE_THREAD_LIBS_INIT}
    )
  
add_test(eventhubclient_ll_unittest eventhubclient_ll_unittest)

add_dependencies(eventhubclient_ll_unittest micromock ctest eventhub_common)
