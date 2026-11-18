# Standalone build for the ada compatibility library tests (independent of the hm11 project build).
#   cmake -S tests/ada -B build/ada && cmake --build build/ada && ctest --test-dir build/ada
cmake_minimum_required(VERSION 3.20)
project(ada_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(ADA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

# Use an installed GTest if available, otherwise fetch it.
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

set(ADA_TEST_SOURCES test_ranged_integer.cpp test_integer_subtype.cpp test_fixed_string.cpp)
set(ADA_WARNINGS -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Werror)

# Built once per out-of-range behaviour mode.
foreach(variant IN ITEMS default saturate ndebug)
    if(variant STREQUAL "default")
        set(target ada_tests)
    else()
        set(target ada_tests_${variant})
    endif()
    add_executable(${target} ${ADA_TEST_SOURCES})
    target_include_directories(${target} PRIVATE ${ADA_INCLUDE_DIR})
    target_link_libraries(${target} PRIVATE GTest::gtest_main)
    target_compile_options(${target} PRIVATE ${ADA_WARNINGS})
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
    if(variant STREQUAL "saturate")
        target_compile_definitions(${target} PRIVATE ENABLE_RUNTIME_RANGE_CHECKS)
    elseif(variant STREQUAL "ndebug")
        target_compile_definitions(${target} PRIVATE NDEBUG)
    endif()
    add_test(NAME ${target} COMMAND ${target})
endforeach()
