#include <Arduino.h>
#include "debug.h"
#include "adc.h"

static constexpr int adc_resolution = 12;
static constexpr uint32_t adc_max_value = (1<<adc_resolution) - 1;
static float VrefCal; // voltage reference ADC value as of manufacturing, 30 deg C, VREF+ = 3.0V.
static float VrefPos; // VREF+ voltage applied to the pin
static float VrefPos_div_adc_max_value; // pre-calculated value of VrefPos / adc_max_value

/**
 * Initialize and calibrate ADC
*/
void init_adc()
{
    analogReadResolution(adc_resolution); // set to 12bit resolution

    // Get *VREFINT_CAL_ADDR. which is a value measuread VREFint when the temperature is 30degC, VREF+ = 3.0V.
    // VrefCal = *VREFINT_CAL_ADDR * (3.0/4096) (when the resolution = 12bit)
    VrefCal = *VREFINT_CAL_ADDR * (VREFINT_CAL_VREF / 1000.0f  / adc_max_value);


    // Measure AVREF virtual pin voltage at the runtime.
    // This is VREFint value measured in actual configuration at the runtime.
    // VrefCal = analogRead(AVREF) * (VREF+  / 4096.0 )
    // Solve VREF+. then, VREF+ is current VDDA applyed to the pin.
    uint32_t raw_value = analogRead(AVREF);
    if(raw_value == 0)
    {
        // Impossible, could never happen in sane environment.
        // Give appropriate value.
        // 1.21V is nominal VREFINT value and 3.3V is nominal power line voltage.
        raw_value = 1.21f / 3.3f * adc_max_value;
    }
    VrefPos = VrefCal / raw_value * adc_max_value;
    VrefPos_div_adc_max_value = VrefPos * (1.0f /  adc_max_value);

    String s1(VrefCal);
    String s2(VrefPos);
    dbg_printf("init_adc(): *VREFINT_CAL_ADDR=%d, Vref_int=%sV, Vref+=%sV\n",
        *VREFINT_CAL_ADDR, s1.c_str(), s2.c_str());
}


/**
 * Read voltage from specified pin. The returned value's unit is volt.
*/
float adc_read_voltage(uint32_t pin)
{
    uint32_t raw_value = analogRead(pin);
    return raw_value * VrefPos_div_adc_max_value;
}

/**
 * Read normalized value from specified pin. The normalized value is:
 * 0.0 - voltage at GND (0V)
 * 1.0 - VREF+ (nominal 3.3V)
*/
float adc_read_normalized(uint32_t pin)
{
    uint32_t raw_value = analogRead(pin);
    return raw_value * (1.0f / adc_max_value);
}
