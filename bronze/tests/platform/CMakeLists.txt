# Platform tests

# Create test executable for alignment tests
add_hm11_test(alignment_test
    alignment_test.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)

# Add sources for cache tests
target_sources(alignment_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/platform/cache.cpp
)
