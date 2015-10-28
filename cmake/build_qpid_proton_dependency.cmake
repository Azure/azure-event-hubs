ExternalProject_Add(
    qpid_proton
    PREFIX ${EP_PREFIX}/qpid-proton
    BINARY_DIR ${EP_BINARY}/qpid-proton
    INSTALL_DIR ${EP_INSTALL}/qpid-proton
    GIT_REPOSITORY https://github.com/apache/qpid-proton.git
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${EP_INSTALL}/qpid-proton -DCMAKE_BINARY_DIR=${EP_BINARY}/qpid-proton
)

