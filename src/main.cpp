#include <Arduino.h>
#include "debug.h"
#include <Wire.h>
#include "thermistor.h"
#include "adc.h"
#include "pins.h"
#include "control.h"
#include "ftostrf.h"


#if 0
/**
 * print I2C_TIMING_XXX constants required not to include large code in i2c_computeTiming() 
*/
static void print_i2c_timing_constants()
{
    Wire.begin(/*sda:*/ (uint32_t) /*PB7*/ D21, /*scl:*/ (uint32_t) /*PB6*/ D20); // use Arduino Dxxx pin number name; refer to "variant_generic.cpp"
//    dump_i2c();

    Wire.setClock(1000000);
    dbg_printf("-DI2C_TIMING_FMP=0x%08X ", Wire.getHandle()->Init.Timing);
    Wire.end();

    Wire.begin(/*sda:*/ (uint32_t) /*PB7*/ D21, /*scl:*/ (uint32_t) /*PB6*/ D20); // use Arduino Dxxx pin number name; refer to "variant_generic.cpp"
//    dump_i2c();

    Wire.setClock(400000);
    dbg_printf("-DI2C_TIMING_FM=0x%08X ", Wire.getHandle()->Init.Timing);
    Wire.end();

    Wire.begin(/*sda:*/ (uint32_t) /*PB7*/ D21, /*scl:*/ (uint32_t) /*PB6*/ D20); // use Arduino Dxxx pin number name; refer to "variant_generic.cpp"
//    dump_i2c();

    Wire.setClock(10000);
    dbg_printf("-DI2C_TIMING_SM=0x%08X\n", Wire.getHandle()->Init.Timing);
    Wire.end();
}
#endif

/**
 * Check OptionBytes whether boot0 pin is enabled or not.
 * Enable boot0 pin if the boot0 pin is not enabled.
 * This function will not return if boot0 pin activation process is needed,
 * because enabling boot0 requires MCU reset.
 */
static void check_boot0_pin()
{
    FLASH_OBProgramInitTypeDef OptionsBytesStruct;
    HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);

    if (OptionsBytesStruct.USERConfig & OB_USER_nBOOT_SEL)
    {
        // nBOOT0_SEL is 1.
        // This will deny bootloader regardless of BOOT0 pin

        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();

        OptionsBytesStruct.OptionType = OPTIONBYTE_USER;
        OptionsBytesStruct.USERType = OB_USER_nBOOT_SEL;
        OptionsBytesStruct.USERConfig = OB_BOOT0_FROM_PIN;

        if (HAL_FLASHEx_OBProgram(&OptionsBytesStruct) != HAL_OK)
        {
            HAL_FLASH_OB_Lock();
            HAL_FLASH_Lock();
            return;
        }

        HAL_FLASH_OB_Launch(); // will reset the chip

        /* unreachable here */

        /* We should not make it past the Launch, so lock
         * flash memory and return an error from function
         */
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
    }
}
/*
void dump_i2c()
{
    dbg_printf("I2C subsystem initializing...\n");

    dbg_printf("I2C map:\n  ");
    int addr;
    for (addr = 0; addr < 128; addr++)
    {
        if (addr == 0 || addr == 127)
        {
            dbg_printf("xx ");
            continue;
        }
        Wire.beginTransmission(addr);
        int error = Wire.endTransmission();

        if (error == 0)
        {
            dbg_printf("%02x ", addr);
        }
        else
        {
            dbg_printf("-- ");
        }
        if ((addr & 0x0f) == 0x0f)
            dbg_printf("\n  ");
    }
    dbg_printf("\n");
}
*/


void setup()
{

    check_boot0_pin();

    pinMode(PA4, INPUT);

    pinMode(D21, INPUT_PULLUP);
    pinMode(D20, INPUT_PULLUP);

    control_init();
    init_adc();

    Wire.begin(/*sda:*/ (uint32_t) /*PB7*/ D21, /*scl:*/ (uint32_t) /*PB6*/ D20); // use Arduino Dxxx pin number name; refer to "variant_generic.cpp"
//    dump_i2c();

    Wire.setClock(400000);

}

void loop()
{
    // put your main code here, to run repeatedly:
    /*
    thermistor_converter_t conv(25.0f, 2950, 100000.0f, 4700.0);
    analogReadResolution(12);
    dbg_printf("millis:%d read:%d, ref:%d, cal:%d\r\n", millis(), analogRead(PIN_ANALOG), analogRead(AVREF), *VREFINT_CAL_ADDR);
    VREFINT_CAL_ADDR;
    VREFINT_CAL_VREF;
    float temp = conv.calc(analogRead(PIN_ANALOG) * (1.0f / ((float)(1<<10) -1) ));
    dbg_printf("temp: %d", (int)(temp + 0.5f));
    */
    String s = float_to_string(thermistor_read_charger());
    dbg_printf("Charger IC temperature: %s\n", s.c_str());
    control_check();
}
