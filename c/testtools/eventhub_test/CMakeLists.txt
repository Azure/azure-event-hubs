
# ----------------------------------------
# Send sample
# ----------------------------------------
file(GLOB_RECURSE EVENTHUB_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

list(SORT EVENTHUB_TEST_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}" ${EVENTHUB_TEST_SOURCES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
include_directories(${CMAKE_SOURCE_DIR}/c/common/inc)

find_package(Threads REQUIRED)

add_library (eventhub_test ${EVENTHUB_TEST_SOURCES})


# Libraries to link in reverse order because that's what ld requires.
target_link_libraries (eventhub_test
    ${CMAKE_THREAD_LIBS_INIT}
    $<TARGET_FILE:eventhub_common>  
    $<TARGET_FILE:eventhub_client> 
    ${${PROJECT_NAME}_EXTERNAL_LIBRARIES})

add_dependencies(eventhub_test eventhub_client)
