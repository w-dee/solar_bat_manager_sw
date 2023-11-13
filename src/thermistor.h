#pragma once


#define THERMISTOR_T0 25.0 // T0 value in deg. C
#define THERMISTOR_R0 10000.0f // thermistor R0 resistance at 25 deg. C in Ohm
#define THERMISTOR_RP 10000.0f // thermistor pull-up resistor value in Ohm
#define THERMISTOR_BETA 3960.0f // beta value of WUXI XUYANG ELECTRONICS CO.,LTD, DKF103*1



/**
 * Thermister calculation class. The connection must be:
 * ^  to VREF
 * |
 * >
 * <   RP
 * >
 * |
 * *--------> to ADC
 * |
 * <
 * >  Thermistor
 * <
 * |
 * -  GND
 * */
class thermistor_converter_t
{
private:
    float T0; // temp T0 (in deg C)
    float B; // coeff B
    float R0; // value R0
    float RP; // pull up resistor value

public:
    thermistor_converter_t(float t0, float b, float r0, float rp) :
        T0(t0), B(b), R0(r0), RP(rp) {;}
    
    static constexpr float K0 = 273.15; // Kelvin value for 0 deg C


    float calc(float adc_val_normalized) const;

    static float convert(const float T0, const float B, const float R0, const float RP,
        float adc_val_normalized);
};


float thermistor_read_charger();
float thermistor_read_battery();