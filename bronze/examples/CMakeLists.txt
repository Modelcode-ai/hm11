# CMakeLists.txt for examples directory
#
# This file manages the build process for all HM11 driver examples.
# Each example is contained in its own subdirectory and demonstrates
# a specific aspect of the driver's functionality.
#
# The examples are organized in order of increasing complexity:
# 1. Basic example - Shows core functionality and basic communication
# 2. Configuration example - Demonstrates comprehensive configuration options
# 3. Peripheral echo - Implements a complete echo server as a peripheral device
#
# Additional examples will be added as they are developed.

# Add the basic example subdirectory
# This example demonstrates initialization, basic commands and simple data exchange
add_subdirectory(01_basic_example)

# Add the configuration example subdirectory
# This example shows how to configure all aspects of the HM11 module
add_subdirectory(02_configuration_example)

# Add the peripheral echo example subdirectory
# This example implements a complete echo server that matches the original Ada example
add_subdirectory(03_peripheral_echo)