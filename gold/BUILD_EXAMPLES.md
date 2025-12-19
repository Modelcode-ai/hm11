# Building HM-11 Driver Examples for STM32F429

This guide covers the steps required to build the embedded examples for the STM32F429 Discovery board.

## Prerequisites

### 1. ARM GCC Toolchain

Install the ARM Embedded GCC toolchain (`arm-none-eabi-gcc`).

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi
```

**macOS (Homebrew):**
```bash
brew install arm-none-eabi-gcc
```

**Arch Linux:**
```bash
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib
```

**Manual Installation:**
Download from [ARM Developer](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) and add to PATH.

Verify installation:
```bash
arm-none-eabi-gcc --version
arm-none-eabi-g++ --version
arm-none-eabi-objcopy --version
arm-none-eabi-size --version
```

### 2. CMake 3.20+

**Ubuntu/Debian:**
```bash
sudo apt-get install cmake
```

**macOS (Homebrew):**
```bash
brew install cmake
```

Verify:
```bash
cmake --version
```

### 3. Make or Ninja

**Ubuntu/Debian:**
```bash
sudo apt-get install make
# or for faster builds:
sudo apt-get install ninja-build
```

## CMSIS Headers Setup

The STM32F429 build requires ARM CMSIS Core headers and STM32F4 device headers. These must be downloaded and placed in the `vendor/CMSIS` directory.

### Download and Install CMSIS Headers (Required)

```bash
cd /path/to/hm11

# Create vendor directories
mkdir -p vendor/CMSIS/Include
mkdir -p vendor/CMSIS/Device/ST/STM32F4xx/Include

# Download CMSIS Core headers (ARM)
curl -L -o cmsis-core.tar.gz \
    "https://github.com/ARM-software/CMSIS_5/archive/refs/tags/5.9.0.tar.gz"
tar -xzf cmsis-core.tar.gz "CMSIS_5-5.9.0/CMSIS/Core/Include"
mv CMSIS_5-5.9.0/CMSIS/Core/Include/* vendor/CMSIS/Include/
rm -rf CMSIS_5-5.9.0 cmsis-core.tar.gz

# Download STM32F4 device headers (ST)
curl -L -o stm32f4.tar.gz \
    "https://github.com/STMicroelectronics/cmsis_device_f4/archive/refs/tags/v2.6.10.tar.gz"
tar -xzf stm32f4.tar.gz "cmsis-device-f4-2.6.10/Include"
mv cmsis-device-f4-2.6.10/Include/* vendor/CMSIS/Device/ST/STM32F4xx/Include/
rm -rf cmsis-device-f4-2.6.10 stm32f4.tar.gz
```

### Verify CMSIS Installation

```bash
# Should show ARM CMSIS Core headers
ls vendor/CMSIS/Include/
# Expected: core_cm4.h, cmsis_gcc.h, cmsis_compiler.h, etc.

# Should show STM32F4 device headers
ls vendor/CMSIS/Device/ST/STM32F4xx/Include/
# Expected: stm32f4xx.h, stm32f429xx.h, system_stm32f4xx.h, etc.
```

## STM32 Startup Files Setup

The build requires STM32 startup code (Reset_Handler, vector table, system initialization). These files are included in the repository under `src/stm32/startup/`:

- `startup_stm32f429zitx.s` - Vector table and Reset_Handler (entry point)
- `system_stm32f4xx.c` - SystemInit for clock configuration
- `syscalls.c` - POSIX stubs for bare-metal operation

### If Startup Files Are Missing

If the startup files are not present, download them from STM32CubeF4:

```bash
cd /path/to/hm11

# Create startup directory
mkdir -p src/stm32/startup

# Download STM32CubeF4
curl -L -o stm32cube.tar.gz \
    "https://github.com/STMicroelectronics/STM32CubeF4/archive/refs/tags/v1.28.1.tar.gz"

# Extract startup files
tar -xzf stm32cube.tar.gz \
    "STM32CubeF4-1.28.1/Projects/STM32F429I-Discovery/Applications/Display/LTDC_AnimatedPictureFromUSB/STM32CubeIDE/Example/Startup/startup_stm32f429zitx.s" \
    "STM32CubeF4-1.28.1/Projects/STM32F429I-Discovery/Applications/Display/LTDC_AnimatedPictureFromUSB/Src/system_stm32f4xx.c"

# Move to correct location
mv "STM32CubeF4-1.28.1/Projects/STM32F429I-Discovery/Applications/Display/LTDC_AnimatedPictureFromUSB/STM32CubeIDE/Example/Startup/startup_stm32f429zitx.s" src/stm32/startup/
mv "STM32CubeF4-1.28.1/Projects/STM32F429I-Discovery/Applications/Display/LTDC_AnimatedPictureFromUSB/Src/system_stm32f4xx.c" src/stm32/startup/

# Cleanup
rm -rf STM32CubeF4-1.28.1 stm32cube.tar.gz
```

### Verify Startup Files

```bash
ls src/stm32/startup/
# Expected: startup_stm32f429zitx.s, system_stm32f4xx.c, syscalls.c
```

## Building Examples

### Debug Build (Development)

Debug builds include debug symbols and minimal optimization, useful for development and debugging.

```bash
# Configure
cmake --preset stm32f429-debug

# Build all examples
cmake --build --preset stm32f429-debug

# Build specific example
cmake --build --preset stm32f429-debug --target 02_peripheral_echo
```

### Release Build (Production)

Release builds are optimized for size and performance, recommended for production firmware.

```bash
# Configure
cmake --preset stm32f429-release

# Build all examples
cmake --build --preset stm32f429-release

# Build specific example
cmake --build --preset stm32f429-release --target 02_peripheral_echo
```

### Available Example Targets

| Target | Description |
|--------|-------------|
| `01_basic_init` | Basic initialization and AT commands |
| `02_peripheral_echo` | Peripheral echo server |
| `03_central_scan` | Central role device discovery |
| `04_gatt_client` | GATT client operations |
| `05_ibeacon` | iBeacon configuration |
| `06_hmsensor` | HMSensor temperature/humidity |

## Output Files

After building, the following files are generated for each example:

| File | Description |
|------|-------------|
| `examples/<name>/<name>` | ELF file with debug symbols |
| `examples/<name>/<name>.bin` | Raw binary for flashing |
| `examples/<name>/<name>.hex` | Intel HEX format |

Example paths for `02_peripheral_echo`:

**Debug build:**
```
build/stm32f429-debug/examples/02_peripheral_echo/02_peripheral_echo
build/stm32f429-debug/examples/02_peripheral_echo/02_peripheral_echo.bin
build/stm32f429-debug/examples/02_peripheral_echo/02_peripheral_echo.hex
```

**Release build:**
```
build/stm32f429-release/examples/02_peripheral_echo/02_peripheral_echo
build/stm32f429-release/examples/02_peripheral_echo/02_peripheral_echo.bin
build/stm32f429-release/examples/02_peripheral_echo/02_peripheral_echo.hex
```

## Size Information

The build automatically displays memory usage after linking:

```
Memory region         Used Size  Region Size  %age Used
           FLASH:       XXXX B         2 MB      X.XX%
             RAM:       XXXX B       192 KB      X.XX%
          CCMRAM:          0 B        64 KB      0.00%
```

### Example Sizes (02_peripheral_echo)

| Metric | Debug | Release | Savings |
|--------|-------|---------|---------|
| **FLASH** | 29,648 B (1.41%) | 8,372 B (0.40%) | **71.8%** |
| **RAM** | 10,608 B (5.40%) | 10,552 B (5.37%) | 0.5% |

| Section | Debug | Release |
|---------|-------|---------|
| text (code) | 29,520 B | 8,272 B |
| data (initialized) | 108 B | 96 B |
| bss (uninitialized) | 10,480 B | 10,456 B |
| **Total** | **40,108 B** | **18,824 B** |

The release build is **~3.5x smaller** in FLASH due to compiler optimizations.

To manually check binary size:
```bash
# Debug build
arm-none-eabi-size build/stm32f429-debug/examples/02_peripheral_echo/02_peripheral_echo

# Release build
arm-none-eabi-size build/stm32f429-release/examples/02_peripheral_echo/02_peripheral_echo
```

### Comparing Debug vs Release

```bash
# Build both configurations
cmake --preset stm32f429-debug && cmake --build --preset stm32f429-debug
cmake --preset stm32f429-release && cmake --build --preset stm32f429-release

# Compare sizes
echo "=== Debug ===" && arm-none-eabi-size build/stm32f429-debug/examples/02_peripheral_echo/02_peripheral_echo
echo "=== Release ===" && arm-none-eabi-size build/stm32f429-release/examples/02_peripheral_echo/02_peripheral_echo
```

## Flashing to Hardware

### Using OpenOCD

```bash
# Flash example binary
openocd -f board/stm32f429discovery.cfg \
    -c "program build/stm32f429-debug/examples/02_peripheral_echo/02_peripheral_echo.elf verify reset exit"
```

### Using ST-Link

```bash
st-flash write build/stm32f429-debug/examples/02_peripheral_echo/02_peripheral_echo.bin 0x08000000
```

### Using STM32CubeProgrammer

1. Open STM32CubeProgrammer
2. Connect via ST-Link
3. Load the `.hex` or `.bin` file
4. Set start address to `0x08000000` (for `.bin` files)
5. Click "Start Programming"

## Troubleshooting

### CMSIS Headers Not Found

```
CMake Warning: CMSIS_PATH specified but not found
```

**Solution:** Follow the CMSIS Headers Setup section above, or specify a custom path:
```bash
cmake --preset stm32f429-debug -DCMSIS_PATH=/path/to/your/CMSIS
```

### ARM Toolchain Not Found

```
CMake Error: CMAKE_CXX_COMPILER not set
```

**Solution:** Ensure `arm-none-eabi-g++` is in your PATH:
```bash
which arm-none-eabi-g++
```

### Linker Script Not Found

```
cannot open linker script file
```

**Solution:** Ensure you're running cmake from the project root directory where `cmake/linker-scripts/STM32F429ZITx_FLASH.ld` exists.

### Reset_Handler Warning

```
warning: cannot find entry symbol Reset_Handler
```

This warning indicates missing startup code. For complete firmware, you'll need to add:
- Startup assembly file (`startup_stm32f429xx.s`)
- System initialization (`system_stm32f4xx.c`)

These can be obtained from STM32CubeF4 or generated by STM32CubeMX.

## Quick Reference

### Setup CMSIS Headers (one-time)

```bash
mkdir -p vendor/CMSIS/Include vendor/CMSIS/Device/ST/STM32F4xx/Include && \
curl -sL "https://github.com/ARM-software/CMSIS_5/archive/refs/tags/5.9.0.tar.gz" | \
    tar -xz --strip-components=3 -C vendor/CMSIS/Include "CMSIS_5-5.9.0/CMSIS/Core/Include" && \
curl -sL "https://github.com/STMicroelectronics/cmsis_device_f4/archive/refs/tags/v2.6.10.tar.gz" | \
    tar -xz --strip-components=2 -C vendor/CMSIS/Device/ST/STM32F4xx/Include "cmsis-device-f4-2.6.10/Include"
```

### Build Debug (development)

```bash
cmake --preset stm32f429-debug && cmake --build --preset stm32f429-debug
```

### Build Release (production)

```bash
cmake --preset stm32f429-release && cmake --build --preset stm32f429-release
```

### Build Both Configurations

```bash
cmake --preset stm32f429-debug && cmake --build --preset stm32f429-debug && \
cmake --preset stm32f429-release && cmake --build --preset stm32f429-release
```

## See Also

- [examples/README.md](examples/README.md) - Detailed example descriptions
- [STM32F429 Reference Manual (RM0090)](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [HM-11 Datasheet](http://www.jnhuamao.cn/bluetooth.asp)
