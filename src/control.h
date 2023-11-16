#pragma once

#include <Arduino.h>

/**
 * battery charging state
*/
enum charging_state_t
{
    cs_not_charging, // not charging
    cs_charging // charging, or charging complete
};

/**
 * the reason why the charging is not proceeding
*/
enum not_charging_reason_t
{
    ncr_unknown, // unknown reason,
    ncr_battery_temp_too_high, // battery temperature too high
    ncr_battery_temp_too_low, // battery temperature too low
    ncr_charger_temp_too_high, // charging chip temperature is too high
    ncr_solar_voltage_too_low // solar voltage is too low
};

/**
 * charging state read from the charger chip
*/
enum chip_charging_state_t
{
    ccs_not_charging_nor_complete,
    ccs_charging,
    ccs_complete
};




void control_check();

void control_init();

String control_charging_status_to_string();
