# Platform tests

add_library(hm11_platform_tests OBJECT
    alignment_test.cpp
)

target_include_directories(hm11_platform_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(hm11_platform_tests
    PRIVATE
    GTest::gtest
)