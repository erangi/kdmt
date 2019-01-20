cmake_minimum_required(VERSION 3.5)
project(KeyDomet)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
        message(STATUS "Using default build type Release; CMAKE_BUILD_TYPE can also be set to Debug or RelWithDebInfo.")
    endif()
endif()

add_compile_options(-Wall -Wextra -pedantic -Werror)

add_subdirectory(lib)
add_subdirectory(ext/catch)
add_subdirectory(min_bench)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(ext/gbench)
add_subdirectory(full_bench)
add_subdirectory(tests)
