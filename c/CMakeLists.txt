
# ----------------------------------------
# EventHub Client
# ----------------------------------------

include(ExternalProject)
include(version)
include(compiler_options)
include(dependencies)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if (NOT DEFINED LIB_SUFFIX)
        get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
        if ("${LIB64}" STREQUAL "TRUE" AND ${CMAKE_SIZEOF_VOID_P} STREQUAL "8")
        set(LIB_SUFFIX 64)
        else()
            set(LIB_SUFFIX "")
        endif()
    endif()
else()
     set(LIB_SUFFIX "")
endif()


# Start of variables used during install
set (INCLUDE_INSTALL_DIR include CACHE PATH "Include file directory" FORCE)
set (LIB_INSTALL_DIR "lib${LIB_SUFFIX}" CACHE PATH "Library object file directory" FORCE)

mark_as_advanced (INCLUDE_INSTALL_DIR LIB_INSTALL_DIR)


add_custom_target(core)

add_subdirectory(common)
add_subdirectory(eventhub_client)
add_subdirectory(testtools)



