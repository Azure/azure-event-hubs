if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS "-std=c99 -pedantic -Wall -w")
  set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ -pedantic -Wall -w")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_C_FLAGS "-std=c99 -pedantic -Wall -w")
  set(CMAKE_CXX_FLAGS "-std=c++11 -pedantic -Wall -w")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
endif()
