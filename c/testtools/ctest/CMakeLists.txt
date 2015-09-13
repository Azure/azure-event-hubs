#
# ctest
#

add_definitions(-DUSE_CTEST)

file(GLOB_RECURSE CTEST_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/inc/*.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

list(SORT CTEST_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}/src" ${CTEST_SOURCES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../testrunnerswitcher/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ctest/inc)

find_package(Threads REQUIRED)

add_library(ctest STATIC ${CTEST_SOURCES})

target_link_libraries (ctest ${CMAKE_THREAD_LIBS_INIT})

IF (${CMAKE_MAJOR_VERSION} LESS 3 )
   GET_PROPERTY(ctest_location_prop TARGET ctest PROPERTY LOCATION)
   set(ctest_location ${ctest_location_prop} CACHE FILEPATH "")
ENDIF()


add_dependencies(core ctest)

install (TARGETS ctest 
               ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
               LIBRARY DESTINATION ${LIB_INSTALL_DIR})
 

#add_subdirectory(unittests/ctesttest)