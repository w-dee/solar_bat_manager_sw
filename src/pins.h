#pragma once

#include "variant_generic.h"
// for pins, refer to ↑

#define PIN_THERM_BAT PA0 // thermistor at li-ion battery
#define PIN_THERM_CHG PA1 // thermistor at charging controller IC
#define PIN_SOLAR_VOLTAGE PA12 // solar voltage sense
#define PIN_BATTERY_VOLTAGE PA6 // battery voltage
#define PIN_PROG_VOLTAGE_SENSE PA4 // PROG voltage of the charger chip
#define PIN_CHARGE_CURRENT PA7 // charge current PWM output
#define PIN_BATTERY_CHARGER_STAT PB0 // connected to the STAT pin of the charger chip. 3-STATE.
#define PIN_DEFAULT_CHARGE_INH PA5 // inhibit signal to disable circuit which provide default charge current to the charger 

/**
 * Battery is normally disconnected from the voltage divider, to prevent discharge.
 * This pin connect the battery to the voltage divider.
*/
#define PIN_ENABLE_BATTERY_MEASUREMENT PA11 