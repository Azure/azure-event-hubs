#
# micromock
#

add_definitions(-DUSE_CTEST)

file(GLOB_RECURSE MICROMOCK_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/inc/*.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

list(SORT MICROMOCK_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}/src" ${MICROMOCK_SOURCES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../testrunnerswitcher/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ctest/inc)
IF (NOT MSVC)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sal/inc)
ENDIF()

find_package(Threads REQUIRED)

add_library(micromock STATIC ${MICROMOCK_SOURCES})

target_link_libraries (micromock ${CMAKE_THREAD_LIBS_INIT})

IF (${CMAKE_MAJOR_VERSION} LESS 3 )
   GET_PROPERTY(micromock_location_prop TARGET micromock PROPERTY LOCATION)
   set(micromock_location ${micromock_location_prop} CACHE FILEPATH "")
ENDIF()


add_dependencies(core micromock)

install (TARGETS micromock
               ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
               LIBRARY DESTINATION ${LIB_INSTALL_DIR})
 

add_subdirectory(tools/micromockgenerator)
#add_subdirectory(unittests/micromocktest)