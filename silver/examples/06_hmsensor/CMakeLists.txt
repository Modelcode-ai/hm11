# Example 06: HMSensor-Specific Features
# Demonstrates sensor type configuration, work interval management, 
# and temperature/humidity reading capabilities

add_executable(hm11_example_06_hmsensor
    main.cpp
)

target_link_libraries(hm11_example_06_hmsensor
    PRIVATE
        hm11::driver
        hm11::hal
        hm11::types
)

# Configure compiler options for example
target_compile_options(hm11_example_06_hmsensor PRIVATE
    ${HM11_COMPILE_OPTIONS}
)

# STM32-specific configuration
if(HM11_STM32_TARGET STREQUAL "STM32F429")
    target_sources(hm11_example_06_hmsensor PRIVATE
        ${CMAKE_SOURCE_DIR}/src/platform/stm32f429_startup.cpp
    )
    
    target_link_options(hm11_example_06_hmsensor PRIVATE
        -T${CMAKE_SOURCE_DIR}/cmake/linker-scripts/STM32F429ZITx_FLASH.ld
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -specs=nano.specs
        -lc
        -lm
        -lnosys
        -Wl,--gc-sections
        -static
        -Wl,--start-group
        -lc
        -lm
        -Wl,--end-group
    )
endif()