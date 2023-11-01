#include <Arduino.h>
#include "debug.h"
#include <Wire.h>
#include "thermistor.h"

#include "variant_generic.h"
// for pins, refer to ↑



#define PIN_LED PA1
#define PIN_ANALOG PA4

/*
  ADC (re)calibration procedure:

  Get *VREFINT_CAL_ADDR. which is a value measuread VREFint when the temperature is 30degC, VREF+ = 3.0V.
  Vref = *VREFINT_CAL_ADDR * (3.0/4096) (when the resolution = 12bit)
  Measure current AVREF virtual pin voltage. This is current VREFint value measured in current configuration.
  Vref = analogRead(AVREF) * (VREF+  / 4096.0 )
  solve VREF+. then, VREF+ is current VDDA applyed to the pin.

  read analog value in volts:
  volts = analogRead(PIN) * (VREF+ / 4096.0)
*/
void dump_i2c()
{
    dbg_printf("I2C subsystem initializing...\n");

    dbg_printf("I2C map:\n  ");
    int addr;
    for(addr = 0; addr < 128; addr++ )
    {
        if(addr == 0 || addr == 127)
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
        if((addr & 0x0f) == 0x0f) dbg_printf("\n  ");
    }
    dbg_printf("\n");
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PA4, INPUT);

  pinMode(D21, INPUT_PULLUP);
  pinMode(D20, INPUT_PULLUP);

  Wire.begin(/*sda:*/(uint32_t)/*PB7*/D21, /*scl:*/(uint32_t)/*PB6*/D20); // use Arduino Dxxx pin number name; refer to "variant_generic.cpp"
  dump_i2c();

  analogWriteFrequency(62500);
  analogWriteResolution(10);
}

void loop() {
  // put your main code here, to run repeatedly:
  thermistor_converter_t conv(25.0f, 2950, 100000.0f, 4700.0);

  
  analogReadResolution(12);
  dbg_printf("millis:%d read:%d, ref:%d, cal:%d\r\n", millis(), analogRead(PIN_ANALOG), analogRead(AVREF), *VREFINT_CAL_ADDR);
  VREFINT_CAL_ADDR;
  VREFINT_CAL_VREF;
  float temp = conv.calc(analogRead(PIN_ANALOG) * (1.0f / ((float)(1<<10) -1) ));
  dbg_printf("temp: %d", (int)(temp + 0.5f));

  FLASH_OBProgramInitTypeDef OptionsBytesStruct;
  HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);
#define P(X) dbg_printf(#X ": 0x%x (%d)\r\n", X)
  P(OptionsBytesStruct.OptionType);
  P(OptionsBytesStruct.RDPLevel);
  P(OptionsBytesStruct.USERConfig);
  P(OptionsBytesStruct.USERType);
  P(OptionsBytesStruct.WRPArea);
  P(OptionsBytesStruct.WRPEndOffset);
  P(OptionsBytesStruct.WRPStartOffset);
  do
  {
    if(OptionsBytesStruct.USERConfig & OB_USER_nBOOT_SEL)
    {
      // nBOOT0_SEL is 1.
      // This will deny bootloader regardless of BOOT0 pin
    
        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();

        OptionsBytesStruct.OptionType = OPTIONBYTE_USER;
        OptionsBytesStruct.USERType = OB_USER_nBOOT_SEL;
        OptionsBytesStruct.USERConfig = OB_BOOT0_FROM_PIN;

        if ( HAL_FLASHEx_OBProgram(&OptionsBytesStruct) != HAL_OK )
        {
          HAL_FLASH_OB_Lock();
          HAL_FLASH_Lock();
          break;
        }

        HAL_FLASH_OB_Launch(); // will reset the chip

        /* unreachable here */
        
        /* We should not make it past the Launch, so lock
        * flash memory and return an error from function
        */
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
        break;

    }
  } while(0);
  digitalWrite(PIN_LED, LOW);
  delay(1000);
  digitalWrite(PIN_LED, HIGH);
  delay(1000);
}
