# solar_bat_manager_sw

This is an abandoned project of making a li-ion charger by a Solar battery.

# Using STM32G030F6 in PlatfomIO

As of Nov. 2023, STM32G030F6 is not yet available in PlatformIO, but you can use it using custom `platformio.ini`.

 + boards/genericSTM32G030F6.json is required to define generic STM32F030F6 board.
 + in `platformio.ini`:
   + specify `platform = ststm32` 
   + specify `board_build.ldscript = ldscript_G030F6Px.ld`, but it seems to work without using this ld script.
   + specify `framework = arduino` if you want to use Arduino.

# How to reduce the flash code size without giving up on using Arduino
 + `-DPIO_FRAMEWORK_ARDUINO_SERIAL_DISABLED` to disable HUGE serial driver.
 + Use can use Segger RTT (RealTimeTerminal) instead of serial connection for debugging. Segger J-Link software will be automatically installed via PlatformIO if you specify `jlink` for uploading/debugging. Also you can install J-Link firmware to STLink V2, using https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/ . SEE "Terms Of Use" BEFORE USING THE SOFTWARE AND/OR THE FIRMWARE. Alternatively you can use OpenOCD for communicating via RTT using arbitrary JTAG/SWD adapter: https://openocd.org/doc/html/General-Commands.html#Real-Time-Transfer-_0028RTT_0029
 + Define `-DI2C_TIMING_FMP=xxxx -DI2C_TIMING_FM=xxxx -DI2C_TIMING_SM=xxxx` constants to avoid linking brute-force, fat code for finding optimal I2C timing which is implemented in ST's HAL library. See `main.cpp` for generating these constants for your specific environment.
 + Do not use double precision floats if possible. Use postfix `f` (eg. `0.0f`, `3.14f` ...) to make the constant float.
 + Avoid using `logf` and `expf` (and other math functions) if possible. See thermistor.cpp for smaller implementations of `logf` and `expf`.
 + Avoid using `String::String(double)` and `String::String(float)`. These implementations use `dtostrf` to convert a double precision float to string; As the name suggests, it includes double precision computation and will make the binary large. Make-shift version of `dtostrf` which does not use double precision float, named `ftostrf`, is available in `lib/ftostrf` (LGPL license).
 + Dump your code with ` ${PLATFORMIO_CORE_DIR}/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-objdump -S .pio/build/genericSTM32G030F6/firmware.elf` and find large code block, or double-precision computation routine, then think how not to use it.
 + The default clock speed of the STM32G030F6 is 8MHz, internal RC oscillator. To use 64MHz clock for the CPU, refer to `rcc.cpp`. `rcc.cpp` can be modified to use external clock or XTAL.


