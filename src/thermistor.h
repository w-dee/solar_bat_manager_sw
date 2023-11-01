#pragma once




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