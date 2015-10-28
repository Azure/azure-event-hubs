
# ----------------------------------------
# eventhubclient_e2etest
# ----------------------------------------

add_definitions(-DUSE_CTEST)
add_definitions(-DGB_DEBUG_ALLOC)
add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS)



file(GLOB_RECURSE EVENTHUBCLIENT_E2ETEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/eventhub_test/src/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/eventhub_test/src/*.cpp)

list(SORT EVENTHUBCLIENT_E2ETEST_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}" ${EVENTHUBCLIENT_E2ETEST_SOURCES})
set_source_files_properties(EVENTHUBCLIENT_E2ETEST_SOURCES PROPERTIES LANGUAGE CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/testrunnerswitcher/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/micromock/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/ctest/inc)
IF (NOT MSVC)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/sal/inc)
ENDIF()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../common/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../testtools/eventhub_test/inc)

find_package(Threads REQUIRED)

add_executable (eventhubclient_e2etest ${EVENTHUBCLIENT_E2ETEST_SOURCES})
set_target_properties(eventhubclient_e2etest PROPERTIES LINKER_LANGUAGE CXX)

IF (${CMAKE_MAJOR_VERSION} LESS 3 )
  set( micromock ${micromock_location})
  set( ctest ${ctest_location})
  set( eventhub_common ${eventhub_common_location})
  set( eventhub_client ${eventhub_client_location})
ELSE()
  set(micromock  $<TARGET_FILE:micromock>) 
  set(ctest  $<TARGET_FILE:ctest>)
  set(eventhub_common  $<TARGET_FILE:eventhub_common>) 
  set(eventhub_client  $<TARGET_FILE:eventhub_client>) 
ENDIF()


# Libraries to link in reverse order because that's what ld requires.
target_link_libraries (eventhubclient_e2etest
    ${micromock}  
	  ${ctest}
    ${eventhub_client}  
	${eventhub_common}  
    ${CMAKE_THREAD_LIBS_INIT}
    ${${PROJECT_NAME}_EXTERNAL_LIBRARIES}
    )
  
add_test(eventhubclient_e2etest eventhubclient_e2etest)

add_dependencies(eventhubclient_e2etest micromock ctest eventhub_common eventhub_client)

