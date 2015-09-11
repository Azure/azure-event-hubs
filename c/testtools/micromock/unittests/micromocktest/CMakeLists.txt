
# ----------------------------------------
# Micromock test
# ----------------------------------------
file(GLOB_RECURSE MICROMOCKTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

list(SORT MICROMOCKTEST_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}" ${MICROMOCKTEST_SOURCES})
#include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)

add_executable (micromocktest ${MICROMOCKTEST_SOURCES})

IF (${CMAKE_MAJOR_VERSION} LESS 3 )
  set( libmicromock ${micromock_location})
ELSE()
  set(libmicromock  $<TARGET_FILE:libmicromock>) 
ENDIF()


# Libraries to link in reverse order because that's what ld requires.
target_link_libraries (micromocktest
    ${libmicromock}  
    ${CMAKE_THREAD_LIBS_INIT}
    ${${PROJECT_NAME}_EXTERNAL_LIBRARIES}
    )
  

add_dependencies(micromocktest libmicromock)
add_dependencies(samples micromocktest)
