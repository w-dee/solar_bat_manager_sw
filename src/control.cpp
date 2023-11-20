#include <Arduino.h>
#include "control.h"
#include "adc.h"
#include "thermistor.h"
#include "pins.h"
#include "ftostrf.h"
#include "debug.h"

#define BATTERY_CHARGEABLE_TEMP_LOW 0
#define BATTERY_CHARGEABLE_TEMP_HIGH 45
#define CHARGER_MAX_TEMP 80

#define WAIT_TIME 200 // control loop wait time in ms
#define CHARGE_CURRENT_STABILIZE_TIME 20 // in ms

#define CHARGING_CURRENT_PWM_WIDTH 10 // pwm width in bits
#define CHARGING_CURRENT_PWM_FREQUENCY 62500
#define MAX_CHARGING_CURRENT_INDEX ((1<<CHARGING_CURRENT_PWM_WIDTH)-1)
#define MAX_CHARGING_CURRENT_INDEX_STEP MAX_CHARGING_CURRENT_INDEX // maximum step count for increase/decrease charging current index
#define MIN_BATTERY_VOLTAGE 3.6f // Minimum battery voltage  
//#define VALID_POWER_INCREASE 0.1f // in mW; if the power is increased but the difference is under this value when increasing the current, current increasing it not taken 
#define MINIMUM_SOLAR_VOLTAGE_OVER_BATTERY_VOLTAGE 0.35f // in V, to charge, charger IC input voltage must be higher than battery_voltage + this constant
#define MIN_SOLAR_VOLTAGE (MIN_BATTERY_VOLTAGE + MINIMUM_SOLAR_VOLTAGE_OVER_BATTERY_VOLTAGE)

/**
 * Read charger charging state; Charger IC is MCP73831
*/
static chip_charging_state_t read_charger_chip_charging_state()
{
    // PIN_BATTERY_CHARGER_STAT is 3-state.
    pinMode(PIN_BATTERY_CHARGER_STAT, INPUT);
    delayMicroseconds(100);
    bool Z_state = digitalRead(PIN_BATTERY_CHARGER_STAT) == HIGH;
    pinMode(PIN_BATTERY_CHARGER_STAT, INPUT_PULLUP);
    delayMicroseconds(100);
    bool pullup_ed_state = digitalRead(PIN_BATTERY_CHARGER_STAT) == HIGH;
    pinMode(PIN_BATTERY_CHARGER_STAT, INPUT_PULLDOWN);
    delayMicroseconds(100);
    bool pulldown_ed_state = digitalRead(PIN_BATTERY_CHARGER_STAT) == HIGH;

    if(!pulldown_ed_state && pullup_ed_state)
    {
        // the pin reading follows pullup or pulldown;
        // the state is Z (high-impedance)
        return ccs_not_charging_nor_complete;
    }

    // else, the battery charging state should be indicated by 
    // Z_state
    if(Z_state)
    {
        // charge completed
        return ccs_complete;
    }
    else
    {
        // charging proceeding
        return ccs_charging;
    }
}


/**
 * simple lfsr
*/
static uint32_t lfsr(int bits)
{
    static uint32_t lfsr = 1;
    for(int i = 0; i < bits; ++i)
        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xd0000001u); /* taps 32 31 29 1 */
    return lfsr >> (32 - bits);
}


/**
 * returns random increase/decrease step
*/
static int random_step()
{
    uint32_t n;
    n = lfsr(CHARGING_CURRENT_PWM_WIDTH) + 1; // get non-zero random value
    if(n > (1<<CHARGING_CURRENT_PWM_WIDTH) - 1) n = (1<<CHARGING_CURRENT_PWM_WIDTH) - 1;
    return (n * n) >> CHARGING_CURRENT_PWM_WIDTH; // use n^2 curve to give more probability to smaller values
}

/**
 * coroutine stuff
*/
#define ___YIELD2(COUNTER) \
	do { \
	state = COUNTER; \
	return; \
	case COUNTER:; \
	} while(0)

/**
 * coroutine stuff
*/
#define YIELD ___YIELD2(__COUNTER__)

// wait execution at least (X) milliseconds.
// eg. DELAY(X) may wait at least 1 millisecond, but
// sometimes may wait 2 ms, or more.
#define DELAY(X) \
	do { \
		delay_until = millis() + (X) + 1; \
		do YIELD; while ((long) (millis() - delay_until) < 0); \
	} while(0)


/**
 * battery management coroutine-like container implemented in C++ class
*/
class battery_manager_t
{
    int state = -1; // current continuation state
	decltype(millis()) delay_until; // delay end count of millis()

public:
    not_charging_reason_t not_charging_reason = ncr_unknown;
    charging_state_t charging_state = cs_not_charging;

    int charging_current_index = 0;
    float battery_voltage = 0; // in V
    float solar_voltage = 0; // in V
    float battery_temp = 0; // in deg. C
    float charger_temp = 0; // in deg. C
    float charging_current = 0; // in mA
    float charging_power = 0; // charging power = in mW
    chip_charging_state_t chip_charging_state = ccs_not_charging_nor_complete;

private:
    /**
     * Set charging current index to specified value
    */
    void set_charging_current(int idx)
    {
        // Note that: charging current index will saturate at the "prog" current reaches its max before
        // reaching MAX_CHARGING_CURRENT_INDEX
        if(idx < 0) idx = 0;
        else if(idx > MAX_CHARGING_CURRENT_INDEX) idx = MAX_CHARGING_CURRENT_INDEX;
        analogWrite(PIN_CHARGE_CURRENT, idx);
        charging_current_index = idx;
    }

    int increase_steps, decrease_steps;
    float prev_sol_vol, prev_power;
    float prev_current;
    float decreased_current, increased_current;
    float decreased_power, increased_power;
    int prev_current_index;
    int decreased_current_index, increased_current_index;
    float decreased_solar_voltage, increased_solar_voltage;

public:
    /**
     * Initialize control module
    */
    void init()
    {
        pinMode(PIN_DEFAULT_CHARGE_INH, OUTPUT);
        digitalWrite(PIN_DEFAULT_CHARGE_INH, HIGH);
        pinMode(PIN_CHARGE_CURRENT, OUTPUT);
        pinMode(PIN_BATTERY_CHARGER_STAT, INPUT_PULLDOWN);

        analogWriteResolution(CHARGING_CURRENT_PWM_WIDTH);
        analogWriteFrequency(CHARGING_CURRENT_PWM_FREQUENCY);
    }


    /**
     * Check thermistor values and voltages, update the state.
    */
    void check()
    {
        while(true)
        {
            switch(state)
            {
            default:
                YIELD;

            start:
                set_charging_current(0);

            wait:
                DELAY(WAIT_TIME);

            temp:
                // temperature check
                charger_temp = thermistor_read_charger();
                battery_temp = thermistor_read_battery();
                if(battery_temp < BATTERY_CHARGEABLE_TEMP_LOW)
                {
                    not_charging_reason = ncr_battery_temp_too_low;
                    charging_state = cs_not_charging;
                    goto start;
                }

                if(battery_temp > BATTERY_CHARGEABLE_TEMP_HIGH)
                {
                    not_charging_reason = ncr_battery_temp_too_high;
                    charging_state = cs_not_charging;
                    goto start;
                }

                if(charger_temp > CHARGER_MAX_TEMP)
                {
                    not_charging_reason = ncr_charger_temp_too_high;
                    charging_state = cs_not_charging;
                    goto start;
                }


            track:
                // Do Maximum Power Point Tracking (MPPT)
                prev_sol_vol = adc_read_solar_voltage();
                prev_current_index = charging_current_index;
                prev_power = charging_power;

                // decide random walk steps
                decrease_steps = random_step();
                increase_steps = random_step();


                // measure charging power using decreased current
                set_charging_current(prev_current_index - decrease_steps); // random walk
                decreased_current_index = charging_current_index;
                DELAY(CHARGE_CURRENT_STABILIZE_TIME);
                decreased_current = adc_read_charger_current_indication();
                decreased_solar_voltage = adc_read_solar_voltage();
                decreased_power = decreased_current * decreased_solar_voltage;

                // measure charging power using increased current and measure battery voltage
                set_charging_current(prev_current_index + increase_steps); // random walk
                increased_current_index = charging_current_index;
                DELAY(CHARGE_CURRENT_STABILIZE_TIME);
                increased_current = adc_read_charger_current_indication();
                increased_solar_voltage = adc_read_solar_voltage();
                increased_power = increased_current * increased_solar_voltage;                    

                // read battery voltage
                // connect battery to the voltage divider
                adc_connect_battery_voltage(); // battery voltage sense circuit has a stabilization capacitor,
                    // so it takes some time to the measured voltage to be stabilized.
                DELAY(10);
                battery_voltage = adc_read_battery_voltage();
                // disconnect battery
                adc_disconnect_battery_voltage();

                if(increased_solar_voltage < battery_voltage + MINIMUM_SOLAR_VOLTAGE_OVER_BATTERY_VOLTAGE ||
                                increased_solar_voltage < MIN_SOLAR_VOLTAGE)
                {
                    increased_power = 0.0f; // too low input voltage
                }

                if(decreased_solar_voltage < battery_voltage + MINIMUM_SOLAR_VOLTAGE_OVER_BATTERY_VOLTAGE ||
                                decreased_solar_voltage < MIN_SOLAR_VOLTAGE)
                {
                    decreased_power = 0.0f; // too low input voltage
                }

                if(prev_sol_vol < battery_voltage + MINIMUM_SOLAR_VOLTAGE_OVER_BATTERY_VOLTAGE ||
                                prev_sol_vol < MIN_SOLAR_VOLTAGE)
                {
                    prev_power = 0.0f;
                }

                // check which is larger
                if(increased_power > prev_power)
                {
                    set_charging_current(increased_current_index);
                    charging_current = increased_current;
                    solar_voltage = increased_solar_voltage;
                    charging_power = increased_power;
                }
                else if(decreased_power > prev_power)
                {
                    set_charging_current(decreased_current_index);
                    charging_current = decreased_current;
                    solar_voltage = decreased_solar_voltage;
                    charging_power = decreased_power;
                }
                else
                {
                    set_charging_current(prev_current_index);
                    solar_voltage = prev_sol_vol;
                    charging_power = prev_power;
                }
                DELAY(CHARGE_CURRENT_STABILIZE_TIME);

                chip_charging_state = read_charger_chip_charging_state();

                if(solar_voltage < MIN_SOLAR_VOLTAGE)
                {
                    // solar voltage too low;
                    // charger chip is likely under UVLO
                    goto solar_too_low;
                }
                else
                {
                    if(chip_charging_state == ccs_complete || chip_charging_state == ccs_charging)
                    {
                        charging_state = cs_charging;
                        not_charging_reason = ncr_unknown;
                    }
                    else if(chip_charging_state == ccs_not_charging_nor_complete)
                    {
                        // check why the charging is not proceeding
                    solar_too_low:
                        charging_state = cs_not_charging;
                        if(battery_voltage < MIN_BATTERY_VOLTAGE)
                        {
                            not_charging_reason = ncr_battery_error;
                        }
                        else
                        {
                            if(solar_voltage < battery_voltage + MINIMUM_SOLAR_VOLTAGE_OVER_BATTERY_VOLTAGE ||
                                solar_voltage < MIN_SOLAR_VOLTAGE)
                                not_charging_reason = ncr_solar_voltage_too_low;
                            else
                                not_charging_reason = ncr_unknown; // unknown reason ...
                        }
                    } // if
                } // if

                dbg_print(to_string().c_str());
                goto wait;

            } // switch
        } // while(true)
    } // void check()

    String to_string()
    {
        const char * ncr = nullptr;
        bool temperature_error = false;
        switch(not_charging_reason)
        {
        case ncr_battery_temp_too_high:         ncr = "BAT_TEMP_HIGH";      temperature_error = true; break;
        case ncr_battery_temp_too_low:          ncr = "BAT_TEMP_LOW";       temperature_error = true; break;
        case ncr_charger_temp_too_high:         ncr = "CHIP_TEMP_HIGH";     temperature_error = true; break;
        case ncr_solar_voltage_too_low:         ncr = "SOLAR_TOO_LOW";      break;
        case ncr_battery_error:                 ncr = "BATTERY_ERROR";      break;
        case ncr_unknown:                       ncr = "NOT_SET";            break;
        }

        const char *cs = nullptr;
        switch(charging_state)
        {
        case cs_charging:                       cs = "CHARGING";            break;
        case cs_not_charging:                   cs = "NOT_CHARGING";        break;
        }

        String str(cs);
        str += " ";
        str += ncr;

        #define S_OUT(n, x) str += " " #n ":" + float_to_string(x)

        S_OUT(BT, battery_temp);
        S_OUT(CT, charger_temp);
        if(!temperature_error)
        {
            // on temperature error, these measurements will not be taken
            str += " CCI:" + String(charging_current_index);
            S_OUT(CC, charging_current);
            S_OUT(BV, battery_voltage);
            S_OUT(SV, solar_voltage);
            S_OUT(CP, charging_power);
        }
        str += "\n";

        return str;
    }
};

static battery_manager_t battery_manager;

void control_check()
{
    battery_manager.check();
}

void control_init()
{
    battery_manager.init();
}

String control_charging_status_to_string()
{
    return battery_manager.to_string();
}
