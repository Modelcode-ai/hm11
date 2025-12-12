# HAL tests

add_library(hm11_hal_tests OBJECT
    hal_test.cpp
)

target_include_directories(hm11_hal_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(hm11_hal_tests
    PRIVATE
    GTest::gtest
)