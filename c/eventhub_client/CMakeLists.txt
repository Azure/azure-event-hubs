#
# eventhub_client
#

file(GLOB_RECURSE EVENTHUB_CLIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

list(SORT EVENTHUB_CLIENT_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}/src/" ${EVENTHUB_CLIENT_SOURCES})
set_source_files_properties(EVENTHUB_CLIENT_UNITTEST_SOURCES PROPERTIES LANGUAGE CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

find_package(Threads REQUIRED)

add_library(eventhub_client STATIC ${EVENTHUB_CLIENT_SOURCES})

IF (${CMAKE_MAJOR_VERSION} LESS 3)
     target_link_libraries (eventhub_client ${CMAKE_THREAD_LIBS_INIT}
        ${eventhub_common_location} 
        ${${PROJECT_NAME}_EXTERNAL_LIBRARIES})
     GET_PROPERTY(eventhub_client_location_prop TARGET eventhub_client PROPERTY LOCATION)
     set(eventhub_client_location ${eventhub_client_location_prop} CACHE FILEPATH "")
ELSE()
    target_link_libraries (eventhub_client ${CMAKE_THREAD_LIBS_INIT}
        $<TARGET_FILE:eventhub_common> 
        ${${PROJECT_NAME}_EXTERNAL_LIBRARIES})
ENDIF()



add_dependencies(eventhub_client eventhub_common)
add_dependencies(core eventhub_client)

install (TARGETS eventhub_client 
               ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
               LIBRARY DESTINATION ${LIB_INSTALL_DIR})
 

# Install header files
file(GLOB headers "inc/*.[hi]")
install (FILES ${headers} DESTINATION ${INCLUDE_INSTALL_DIR}/eventhubs)



add_subdirectory(samples)
add_subdirectory(unittests)
