# Sync tests

# Add binary semaphore test executable
add_hm11_test(binary_semaphore_test binary_semaphore_test.cpp)
target_link_libraries(binary_semaphore_test PRIVATE hm11_sync)

# Add watchdog test executable
add_hm11_test(watchdog_test watchdog_test.cpp)
target_link_libraries(watchdog_test PRIVATE hm11_sync)