#include <Arduino.h>
#include "debug.h"
#include "adc.h"
#include "pins.h"
#include "ftostrf.h"

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

    String s1 = float_to_string(VrefCal);
    String s2 = float_to_string(VrefPos);
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

void adc_connect_battery_voltage()
{
    pinMode(PIN_ENABLE_BATTERY_MEASUREMENT, OUTPUT);
    digitalWrite(PIN_ENABLE_BATTERY_MEASUREMENT, HIGH);
}

void adc_disconnect_battery_voltage()
{
    digitalWrite(PIN_ENABLE_BATTERY_MEASUREMENT, LOW);
}

#define R_DIVIDER(HIGHER_R, LOWER_R) ((((float)(LOWER_R)+(float)(HIGHER_R)) / ((float)(LOWER_R))))

#define DIVIDER_R_BATTERY_VOLTAGE_HIGH 22 // voltage divider value of higher resistor (in k Ohm)
#define DIVIDER_R_BATTERY_VOLTAGE_LOW  22 // voltage divider value of lower resistor (in k Ohm)
float adc_read_battery_voltage()
{
    float value = adc_read_voltage(PIN_BATTERY_VOLTAGE);
    return value * R_DIVIDER(DIVIDER_R_BATTERY_VOLTAGE_HIGH, DIVIDER_R_BATTERY_VOLTAGE_LOW);
}

#define DIVIDER_R_SOLAR_VOLTAGE_HIGH 47  // voltage divider value of higher resistor (in k Ohm)
#define DIVIDER_R_SOLAR_VOLTAGE_LOW 22 // voltage divider value of lower resistor (in k Ohm)
float adc_read_solar_voltage()
{
    return adc_read_voltage(PIN_SOLAR_VOLTAGE) * R_DIVIDER(DIVIDER_R_SOLAR_VOLTAGE_HIGH, DIVIDER_R_SOLAR_VOLTAGE_LOW);
}

#define CHARGER_INDICATION_LOAD_RESISTANCE 470 // value of R31 in Ohm
#define MCP73831_G 1000.0f // ratio of MCP73831 "prog" current to charge current setting
float adc_read_charger_current_indication()
{
    float value = adc_read_voltage(PIN_PROG_VOLTAGE_SENSE);
    return value * (MCP73831_G / (float)(CHARGER_INDICATION_LOAD_RESISTANCE) * 1000.0f /* A -> mA */);
}
