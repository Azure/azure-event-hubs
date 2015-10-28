
# ----------------------------------------
# eventdata_unittest
# ----------------------------------------

add_definitions(-DUSE_CTEST)
add_definitions(-DGB_DEBUG_ALLOC)
add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS)


file(GLOB_RECURSE EVENTDATA_UNITTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../src/eventdata.c)

list(SORT EVENTDATA_UNITTEST_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}" ${EVENTDATA_UNITTEST_SOURCES})
set_source_files_properties(EVENTDATA_UNITTEST_SOURCES PROPERTIES LANGUAGE CXX)


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

add_executable (eventdata_unittest ${EVENTDATA_UNITTEST_SOURCES})
set_target_properties(eventdata_unittest PROPERTIES LINKER_LANGUAGE CXX)

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
target_link_libraries (eventdata_unittest
	${micromock}  
	${ctest}
  ${eventhub_common}  
    ${CMAKE_THREAD_LIBS_INIT}
    ${${PROJECT_NAME}_EXTERNAL_LIBRARIES}
    )


add_test(eventdata_unittest eventdata_unittest)

add_dependencies(eventdata_unittest micromock ctest eventhub_common)
