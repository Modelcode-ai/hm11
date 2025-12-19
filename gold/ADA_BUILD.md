# Building the HM-11 Examples for STM32F429 Discovery

This guide provides the exact steps to build the HM-11 Bluetooth examples for the STM32F429 Discovery board.

Two example variants are available:
- **hm11_f429disco** - Full example with LCD display output
- **hm11_nolcd_f429disco** - Minimal example using LEDs only (smaller footprint)

## Prerequisites

- Linux system (tested on Ubuntu/Debian-based)
- Internet connection for downloading toolchains
- Basic build tools (wget, unzip)
- Ada_Drivers_Library repo

## Build Steps

### 1. Install Alire Package Manager

Download and install Alire for your architecture:

```bash
# For aarch64/ARM64 systems:
wget https://github.com/alire-project/alire/releases/download/v2.1.0/alr-2.1.0-bin-aarch64-linux.zip
unzip alr-2.1.0-bin-aarch64-linux.zip
chmod +x bin/alr

# For x86_64 systems:
# wget https://github.com/alire-project/alire/releases/download/v2.1.0/alr-2.1.0-bin-x86_64-linux.zip
# unzip alr-2.1.0-bin-x86_64-linux.zip
# chmod +x bin/alr
```

### 2. Install ARM Cross-Compiler Toolchain

Use Alire to install the GNAT ARM ELF cross-compiler:

```bash
./bin/alr toolchain --select gnat_arm_elf
# This will download and install gnat_arm_elf v15.2.1 (approximately 500MB)
```

### 3. Install GPRbuild

Install the Ada build system:

```bash
./bin/alr toolchain --select gprbuild
# This will download and install gprbuild v25.0.1
```

### 4. Build the HM-11 Example

Navigate to the STM32F429_Discovery examples directory and build:

```bash
cd examples/STM32F429_Discovery

# Set up the PATH to include the toolchains
export PATH="$HOME/.local/share/alire/toolchains/gprbuild_25.0.1_5e58c08e/bin:$HOME/.local/share/alire/toolchains/gnat_arm_elf_15.2.1_f9a42d4d/bin:$PATH"
```

#### Option A: Full Example (with LCD)

```bash
gprbuild -P hm11_f429disco.gpr -p --target=arm-eabi --RTS=embedded-stm32f429disco
```

#### Option B: No-LCD Example (LEDs only)

```bash
gprbuild -P hm11_nolcd_f429disco.gpr -p --target=arm-eabi --RTS=light-tasking-stm32f429disco
```

Note: The no-LCD example uses the SFP (Small Footprint Profile) runtime for reduced size.

## Build Output

After successful compilation, the binaries will be located at:

**Full Example (with LCD):**
- **ELF file:** `../shared/hm11/obj/stm32f429disco/hm11_example`
- Flash: ~248KB out of 2MB (11.84%)
- RAM: ~23KB out of 192KB (11.78%)

**No-LCD Example:**
- **ELF file:** `../shared/hm11/obj/stm32f429disco_nolcd/hm11_example_nolcd`
- Flash: ~181KB out of 2MB (8.62%)
- RAM: ~18KB out of 192KB (9.08%)

## Optional: Generate Binary File for Flashing

To create a binary file suitable for flashing to the board:

```bash
# Full example (with LCD)
arm-eabi-objcopy -O binary ../shared/hm11/obj/stm32f429disco/hm11_example ../shared/hm11/obj/stm32f429disco/hm11_example.bin

# No-LCD example
arm-eabi-objcopy -O binary ../shared/hm11/obj/stm32f429disco_nolcd/hm11_example_nolcd ../shared/hm11/obj/stm32f429disco_nolcd/hm11_example_nolcd.bin
```

## Checking Flash and RAM Usage

### Method 1: Using arm-eabi-size

The `arm-eabi-size` command shows the memory usage of your compiled binary:

```bash
# Full example (with LCD)
arm-eabi-size ../shared/hm11/obj/stm32f429disco/hm11_example

# No-LCD example
arm-eabi-size ../shared/hm11/obj/stm32f429disco_nolcd/hm11_example_nolcd
```

Example output (full example):
```
   text    data     bss     dec     hex filename
 246932    1356   21812  270100   41f14 hm11_example
```

Example output (no-LCD example):
```
   text    data     bss     dec     hex filename
 179412    1356   16500  197268   30294 hm11_example_nolcd
```

**Understanding the output:**
- **text**: Code and read-only data (goes to Flash)
- **data**: Initialized global/static variables (stored in Flash, copied to RAM at startup)
- **bss**: Uninitialized global/static variables (goes to RAM, zeroed at startup)
- **dec/hex**: Total size in decimal/hexadecimal

**Memory calculations:**
- **Flash usage**: `text + data` = code + initialized data
- **RAM usage**: `data + bss` = initialized + uninitialized variables

### Method 2: Detailed Section Analysis

For a detailed breakdown of all sections:

```bash
arm-eabi-size -A ../shared/hm11/obj/stm32f429disco/hm11_example
```

This shows every section with its size and address.

### Method 3: Build Output

The `gprbuild` linker output shows memory region usage directly:

```
Memory region         Used Size  Region Size  %age Used
           flash:      248288 B         2 MB     11.84%
         sram123:       23168 B       192 KB     11.78%
```

This is the most accurate representation of actual device memory usage.

### Method 4: objdump for Section Details

For complete section information including addresses:

```bash
arm-eabi-objdump -h ../shared/hm11/obj/stm32f429disco/hm11_example
```

## Hardware Setup

Connect the HM-11 Bluetooth module to your STM32F429 Discovery board:

| HM-11 Pin | STM32 Pin | Description |
|-----------|-----------|-------------|
| RX (4)    | PC12      | UART5_TX    |
| TX (2)    | PD2       | UART5_RX    |
| GND       | GND       | Ground      |
| VCC       | 3.3V      | Power (3.3V only!) |

## Troubleshooting

### Toolchain Path Issues
If the build fails with "can't find a toolchain", ensure the PATH is correctly set. The exact paths may vary slightly based on the version hash:
```bash
# Find your toolchain paths:
ls ~/.local/share/alire/toolchains/
```

### Architecture Mismatch
- Ensure you download the correct Alire version for your system architecture (aarch64 or x86_64)
- Run `uname -m` to check your system architecture

## Notes

- Both examples implement a Bluetooth Low Energy echo server
- Default HM-11 baud rate: 9600
- The device will appear as "HMTest" when scanning for Bluetooth devices
- Use a BLE scanner app on your phone to test the connection

### Example Differences

| Feature | Full Example | No-LCD Example |
|---------|--------------|----------------|
| Status output | LCD display | LED indicators |
| BSP | Full | SFP (Small Footprint) |
| Runtime | embedded-stm32f429disco | light-tasking-stm32f429disco |
| Flash usage | ~248KB | ~181KB |
| RAM usage | ~23KB | ~18KB |

The no-LCD example provides equivalent functionality using only LEDs:
- **Green LED blinking**: Waiting for connection
- **Green LED solid**: Connected
- **Green LED off**: Disconnected
- **Red LED blinking**: Error state


  The ELF file contains:
  1. Actual program (252KB) - What gets flashed
  2. Debug information (~1.1MB) - Symbol tables, source line info, variable names
  3. ELF headers and metadata - Section headers, program headers
  4. Relocation information - Not needed for embedded systems
  5. String tables - Function/variable names for debugging

  When you flash the board, only the actual program code and initialized data (the .bin file content) gets written to the microcontroller's flash memory. The debug
  information stays on your development machine for use with debuggers like GDB.

  This is why the build output correctly reports:
  Memory region         Used Size  Region Size  %age Used
             flash:      248288 B         2 MB     11.84%
           sram123:       23168 B       192 KB     11.78%

  The build system calculates the actual memory usage on the target device, not the size of the development files.