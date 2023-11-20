# solar_bat_manager_sw

This is abandoned project of making a li-ion charger by a Solar battery.

# Using STM32F030F6 in PlatfomIO

As of Nov 2023, STM32F030F6 is not yet available in PlatformIO but using custom platformio.ini you can use it.


 + boards/genericSTM32FG030F6.json is required to define generic STM32F030F6 board.
 + in platformio.ini:
   + specify `platform = ststm32` 
   + specify `board_build.ldscript = ldscript_G030F6Px.ld`, but it seems to work without using this ld script.
   + specify `framework = arduino` if you want to use Arduino.

# How to reduce the flash code size without giving up on using Arduino
 + `-DPIO_FRAMEWORK_ARDUINO_SERIAL_DISABLED` to disable HUGE serial driver.
 + Use can use Segger RTT (RealTimeTerminal) instead of serial connection. Segger J-Link software will be automatically installed via PlatformIO if you specify JLink for uploading/debugging. Also you can install JLink firmware to STLink V2, using https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/ . SEE "Terms Of Use" BEFORE USING THE SOFTWARE AND/OR THE FIRMWARE.
 + Define -DI2C_TIMING_FMP=xxxx -DI2C_TIMING_FM=xxxx -DI2C_TIMING_SM=xxxx constants to avoid linking brute-force, fat code for finding optimal I2C timing. See main.cpp .
 + Do not use double precision floats if possible.
 + Avoid using logf and expf (and other math functions) if possible. See thermistor.cpp for smaller implementations of logf and expf.
 + Dump your code with  `.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-objdump -S .pio/build/genericSTM32G030F6/firmware.elf` and find large code block, then think how not to use it.

