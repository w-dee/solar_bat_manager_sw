#include <Arduino.h>
#include "control.h"
#include "adc.h"
#include "thermistor.h"
#include "pins.h"


#define BATTERY_CHARGEABLE_TEMP_LOW 0
#define BATTERY_CHARGEABLE_TEMP_HIGH 45
#define CHARGER_MAX_TEMP 80

#define WAIT_TIME 100 // control loop wait time in ms
#define CHARGE_CURRENT_STABILIZE_TIME 10 // in ms

#define CHARGING_CURRENT_INDEX_STEP 10 // step count for increase/decrease charging current index
#define CHARGING_CURRENT_PWM_WIDTH 10 // pwm width in bits
#define MAX_CHARGING_CURRENT_INDEX ((1<<CHARGING_CURRENT_PWM_WIDTH)-1)

#define MINIMUM_SOLAR_VOLTAGE_OVER_BATTERY_VOLTAGE 0.3 // in V, to charge, charger IC input voltage must be higher than battery_voltage + this constant
/**
 * Read charger charging state; Charger IC is MCP73831
*/
static chip_charging_state_t read_charger_chip_charging_state()
{
    // PIN_BATTERY_CHARGER_STAT is 3-state.
    pinMode(PIN_BATTERY_CHARGER_STAT, INPUT);
    bool Z_state = digitalRead(PIN_BATTERY_CHARGER_STAT) == HIGH;
    pinMode(PIN_BATTERY_CHARGER_STAT, INPUT_PULLUP);
    bool pullup_ed_state = digitalRead(PIN_BATTERY_CHARGER_STAT) == HIGH;
    pinMode(PIN_BATTERY_CHARGER_STAT, INPUT_PULLDOWN);
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
    float prog_voltage = 0; // in V
    float battery_temp = 0; // in deg. C
    float charger_temp = 0; // in deg. C
    float charging_current = 0; // in mA
    float charging_power = 0; // charging power = in mW
    chip_charging_state_t charger_charging_state = ccs_not_charging_nor_complete;

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


        analogWriteFrequency(62500);
        analogWriteResolution(10);
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
                battery_temp = thermistor_read_battery();
                charger_temp = thermistor_read_charger();
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
                {
                    chip_charging_state_t ccs;
                    float prev_sol_vol, prev_power;
                    float prev_current;
                    float decreased_current, increased_current;
                    float decreased_power, increased_power;
                    int prev_current_index;
                    int decreased_current_index, increased_current_index;
                    float decreased_solar_voltage, increased_solar_voltage;
                    prev_sol_vol = adc_read_solar_voltage();
                    prev_current_index = charging_current_index;
                    prev_power = charging_power;

                    // measure charging power using decreased current
                    set_charging_current(prev_current_index - CHARGING_CURRENT_INDEX_STEP);
                    decreased_current_index = charging_current_index;
                    DELAY(CHARGE_CURRENT_STABILIZE_TIME);
                    decreased_current = adc_read_charger_current_indication();
                    decreased_solar_voltage = adc_read_solar_voltage();
                    decreased_power = decreased_current * decreased_solar_voltage;

                    // measure charging power using increased current
                    set_charging_current(prev_current_index + CHARGING_CURRENT_INDEX_STEP);
                    increased_current_index = charging_current_index;
                    DELAY(CHARGE_CURRENT_STABILIZE_TIME);
                    increased_current = adc_read_charger_current_indication();
                    increased_solar_voltage = adc_read_solar_voltage();
                    increased_power = increased_current * increased_solar_voltage;                    

                    // check which is larger
                    if(decreased_power >= prev_power) // give bias to "decreasing" current
                    {
                        set_charging_current(decreased_current_index);
                        charging_current = decreased_current;
                        solar_voltage = decreased_solar_voltage;
                        charging_power = decreased_power;
                    }
                    else /*if(increased_current > prev_power)*/
                    {
                        set_charging_current(increased_current_index);
                        charging_current = increased_current;
                        solar_voltage = increased_solar_voltage;
                        charging_power = increased_power;
                    }

                    adc_connect_battery_voltage();
                    DELAY(1);
                    battery_voltage = adc_read_battery_voltage();
                    adc_disconnect_battery_voltage();

                    ccs = read_charger_chip_charging_state();

                    switch(ccs)
                    {
                    case ccs_complete:
                    case ccs_charging:
                        charging_state = cs_charging;
                        not_charging_reason = ncr_unknown;
                        break;
                
                    case ccs_not_charging_nor_complete:
                        charging_state = cs_not_charging;
                            
                        // check why the charging is not proceeding
                        
                        if(solar_voltage < battery_voltage + MINIMUM_SOLAR_VOLTAGE_OVER_BATTERY_VOLTAGE)
                            not_charging_reason = ncr_solar_voltage_too_low;
                        else
                            not_charging_reason = ncr_unknown; // unknown reason ...
                        
                    } // switch(ccs)


                    goto wait;
                }
            } // switch
        } // while(true)
    } // void check()
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
