# Find dependencies


if(NOT CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/external/${CMAKE_BUILD_TYPE})
endif()

#include(ExternalProject)
set(EP_PREFIX ${PROJECT_SOURCE_DIR}/external/${CMAKE_BUILD_TYPE} CACHE PATH "External projects prefix")
set(EP_BINARY ${PROJECT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE} CACHE PATH "External libraries build")
set(EP_INSTALL ${PROJECT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE} CACHE PATH "External libraries")


# QPID PROTON-C
find_package(QPID_PROTON REQUIRED)
if(QPID_PROTON_FOUND)
    include_directories(${QPID_PROTON_INCLUDE_DIR})
    list(APPEND ${PROJECT_NAME}_EXTERNAL_LIBRARIES "${QPID_PROTON_LIBRARY}")
endif()


