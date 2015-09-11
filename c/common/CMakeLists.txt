#
# eventhub_common
#

IF(MSVC)
	file(GLOB_RECURSE EVENTHUB_COMMON_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/adapters/threadapi_c11.c 
		${CMAKE_CURRENT_SOURCE_DIR}/adapters/httpapi_wininet.c 
		${CMAKE_CURRENT_SOURCE_DIR}/adapters/lock_c11.c)
ELSE()
	file(GLOB_RECURSE EVENTHUB_COMMON_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/adapters/threadapi_pthreads.c 
		${CMAKE_CURRENT_SOURCE_DIR}/adapters/httpapi_curl.c 
		${CMAKE_CURRENT_SOURCE_DIR}/adapters/lock_pthreads.c)
ENDIF()

list(SORT EVENTHUB_COMMON_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}/src" ${EVENTHUB_COMMON_SOURCES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

find_package(Threads REQUIRED)

add_library(eventhub_common STATIC ${EVENTHUB_COMMON_SOURCES})

target_link_libraries (eventhub_common ${CMAKE_THREAD_LIBS_INIT})

IF (${CMAKE_MAJOR_VERSION} LESS 3 )
   GET_PROPERTY(eventhub_common_location_prop TARGET eventhub_common PROPERTY LOCATION)
   set(eventhub_common_location ${eventhub_common_location_prop} CACHE FILEPATH "")
ENDIF()


add_dependencies(core eventhub_common)

install (TARGETS eventhub_common 
               ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
               LIBRARY DESTINATION ${LIB_INSTALL_DIR})
 

# Install header files
file(GLOB headers "inc/*.[hi]")
install (FILES ${headers} DESTINATION ${INCLUDE_INSTALL_DIR}/eventhubs)
