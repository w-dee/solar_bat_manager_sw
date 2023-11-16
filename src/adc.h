#pragma once

void init_adc();
float adc_read_voltage(uint32_t pin);
float adc_read_normalized(uint32_t pin);

/**
 * Connect battery to the resistive divider
*/
void adc_connect_battery_voltage();

/**
 * Disconnect battery from the resistive divicer
*/
void adc_disconnect_battery_voltage();

/**
 * Reading battery voltage needs special care because
 * the battery is usually disconnected from the voltage
 * divider to prevent discharge.
 * Use adc_connect_battery_voltage() and wait 1ms, then use this function,
 * and call adc_disconnect_battery_voltage().
*/
float adc_read_battery_voltage();


/**
 *  Read solar battery voltage
*/
float adc_read_solar_voltage();


/**
 * Read current indication to the charger chip, in mA
*/
float adc_read_charger_current_indication();