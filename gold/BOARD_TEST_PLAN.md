# Board Test Plan: STM32F429 Flash Build

The `morph251219` branch makes the C++ examples build into flashable images for the STM32F429 Discovery board. It has only been reviewed on paper. Nobody has compiled it with `arm-none-eabi-gcc` or run it on a board.

This plan lists what to check once an ARM toolchain and a board are available, in order. Each step depends on the one before it. Record results in the table at the end.

## What you need

- `arm-none-eabi-gcc` toolchain on `PATH`, or `ARM_TOOLCHAIN_PATH` set to its `bin/` directory
- CMSIS headers in `vendor/CMSIS` (see [BUILD_EXAMPLES.md](BUILD_EXAMPLES.md#cmsis-headers-setup))
- STM32F429 Discovery board with ST-Link (OpenOCD or `st-flash`)
- HM-11 module wired as below, at 9600 baud, 8N1:

  | Board | HM-11 |
  |---|---|
  | PC12 (UART5_TX) | RX (pin 4) |
  | PD2 (UART5_RX) | TX (pin 2) |
  | 3.3V | VCC (pin 1) |
  | GND | GND (pin 3) |

- A phone with a BLE terminal app (for example, nRF Connect) for the echo test
- Optional: a USB-serial adapter or logic analyzer on PC12/PD2, to see the AT traffic

All commands run from `gold/`. `B` is the build directory:

```bash
B=build/stm32f429-debug
```

## 1. Configure and build

```bash
cmake --preset stm32f429-debug
cmake --build --preset stm32f429-debug
```

**Pass:** all six examples link, and `.bin` and `.hex` files appear under `$B/examples/<name>/`.

**If it fails:**
- *Configure step:* check `cmake/toolchains/arm-none-eabi.cmake` and that the compiler is found.
- *Compile step:* most likely a clash with a CMSIS macro or a missing header. The branch renamed the `drivers.cpp` constants with a `k` prefix for this reason.
- *Link step:* look for undefined `_sbrk`, `_write` and the like, or for duplicate symbols between `syscalls.c` and `libnosys`.

Also do a release build (`stm32f429-release`), because optimization can expose different link errors.

## 2. Check the linked image (no board needed)

The startup code is compiled into the static library `hm11_platform`. The linker only pulls an object out of a static library when something references a symbol in it, so the vector table could be silently left out. Check `02_peripheral_echo` first, then the others.

```bash
ELF=$B/examples/02_peripheral_echo/02_peripheral_echo
```

The ELF file has no `.elf` extension.

### 2a. The vector table is present at the start of flash

```bash
arm-none-eabi-objdump -h $ELF | grep isr_vector
```

**Pass:** `.isr_vector` has VMA/LMA `08000000` and size `000001ac` (107 vectors × 4 bytes).

### 2b. The first two words of the binary are the stack pointer and the reset vector

```bash
xxd -e -l 8 $B/examples/02_peripheral_echo/02_peripheral_echo.bin
arm-none-eabi-nm $ELF | grep -w Reset_Handler
```

**Pass:**
- The first word is `20030000`, which is `_estack`, the top of the 192K RAM block.
- The second word is the `Reset_Handler` address with bit 0 set (Thumb mode).

### 2c. The interrupt handlers point to the real code, not to Default_Handler

```bash
arm-none-eabi-nm -n $ELF | grep -wE "Default_Handler|UART5_IRQHandler|DMA1_Stream0_IRQHandler"
```

**Pass:** `UART5_IRQHandler` and `DMA1_Stream0_IRQHandler` are type `T`, and each has a different address from `Default_Handler`.

**Fail:** they show as `W` or share the address of `Default_Handler`. The interrupt would then fire into an infinite loop. This is the bug fixed in commit `be793e0`, where the handler had been named `USART5_IRQHandler`.

### If any of 2a–2c fail

Link the startup files directly into each executable, as an `OBJECT` library or through `target_sources`, instead of through `hm11_platform`. Then repeat step 2.

## 3. Boot test: 01_basic_init

```bash
openocd -f board/stm32f429discovery.cfg \
  -c "program $B/examples/01_basic_init/01_basic_init verify reset exit"
# or: st-flash write $B/examples/01_basic_init/01_basic_init.bin 0x08000000
```

**Pass:** the green LED blinks three times (initialization, test command, software version), then stays on.

**If nothing happens at all:** attach a debugger and halt the CPU.
- *Halted in `Default_Handler`:* an interrupt has no handler.
- *Halted in `HardFault_Handler`:* likely causes are the vector table, the stack, or the FPU being enabled before use. `SystemInit` enables CP10/CP11, and the code is built with `-mfloat-abi=hard`.

**If the red LED blinks:** an AT command failed. Check the wiring and the baud rate.

## 4. UART interrupt and DMA receive: 02_peripheral_echo

Flash `02_peripheral_echo`, then connect to the HM-11 from the phone.

**Pass, step by step:**
- The green LED blinks while it waits for a connection.
- The LED stays on once the phone connects.
- Text sent from the phone comes back unchanged. Include a message longer than one DMA buffer.
- When the phone disconnects, the board sees `OK+LOST` and the LED blinks rapidly.

This is the first test that depends on `UART5_IRQHandler` and `DMA1_Stream0_IRQHandler` actually running. If the board hangs on the first received byte, go back to step 2c.

## 5. Delays and blink timing

The examples use `std::this_thread::sleep_for`. On this target, that resolves to the `sleep()` and `usleep()` stubs in `src/stm32/startup/syscalls.c`. Those stubs are busy-wait loops whose duration was never measured; the code assumes one loop pass is about 1 µs at 16 MHz.

Measure with a stopwatch, or a scope on the LED pin:
- the 200 ms blinks in `led_control.hpp`;
- the roughly 4 s startup sequence in `initialization.hpp` (2 s + 1 s + 1 s).

**Pass:** each delay is within about ±25% of its nominal value.

**Fail:** if delays are off by a consistent factor, recalibrate the loop. Or better, replace it with a SysTick-based delay. The CPU runs from the 16 MHz HSI clock; `SystemInit` does not set up the PLL.

## 6. Timeouts with the module disconnected

The watchdog and semaphore timeouts read `std::chrono::steady_clock`. With `nosys.specs` there is probably no time source behind it: `_gettimeofday` is only a stub. If so, timeouts never expire.

Test it:
1. Disconnect the HM-11 TX wire, or power the module off.
2. Reset the board while running `01_basic_init`.

**Pass:** within the documented timeout, the board reports an error (red LED blinking from `indicate_error()`).

**Fail:** the board waits forever. To fix it, provide a monotonic time source: a SysTick millisecond counter behind `_gettimeofday` or `clock_gettime`.

The same problem exists on `main`, so a failure here doesn't block the merge. It does mean error handling doesn't work on hardware.

## 7. Smoke test the remaining examples

Flash each of these and check it against the "Expected behavior" section for that example in [examples/README.md](examples/README.md):

| Example | What to check |
|---|---|
| 03_central_scan | Scans and finds nearby BLE devices; the green LED shows it finished |
| 04_gatt_client | Connects to a known peripheral and reads/writes a characteristic |
| 05_ibeacon | The module advertises as an iBeacon; check with a phone scanner app |
| 06_hmsensor | Sensor commands succeed; this example also uses `sleep_for` directly |

## 8. Known minor issues to watch for

- `_sbrk` checks the heap against the top of RAM, not against the current stack pointer. If you see corruption under heavy allocation, suspect the heap and stack colliding.
- `syscalls.c` is compiled into `hm11_platform` and also added again to `02_peripheral_echo`. That is harmless, but remove the extra copy once the build is confirmed.

## Results

| Step | Debug | Release | Notes |
|---|---|---|---|
| 1. Build | | | |
| 2a. Vector table at 0x08000000 | | | |
| 2b. SP / reset vector | | | |
| 2c. IRQ handlers linked | | | |
| 3. 01_basic_init boots | | | |
| 4. 02 echo + disconnect | | | |
| 5. Delay accuracy | | | |
| 6. Timeout fires | | | |
| 7. 03–06 smoke | | | |

Tested by: ______  Date: ______  Toolchain version (`arm-none-eabi-gcc --version`): ______
