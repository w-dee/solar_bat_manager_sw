#include <Arduino.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "thermistor.h"
#include "adc.h"
#include "pins.h"

/**
 * Calculate thermistor temperature from normalized (0.0 to 1.0) ADC value.
 * The returned value will be in deg C.
 * */
float thermistor_converter_t::calc(float adc_val_normalized) const
{
    return convert(T0, B, R0, RP, adc_val_normalized);
}


/*
    The standard logf requires approx 6kB of flash ... too large.
    I (re)implemented smaller code for logf, requires only ~2kB. 
*/

/*
    constant value required for exp2f
*/
static const float d_exp2f_p[] = {
    1.535336188319500e-4f,
    1.339887440266574e-3f,
    9.618437357674640e-3f,
    5.550332471162809e-2f,
    2.402264791363012e-1f,
    6.931472028550421e-1f,
    1.000000000000000f
};
#define D_FLOAT_LOG2OFE  1.4426950408889634074f

#define D_FLOAT_BIAS 127

#if __cplusplus >= 202002L
    #include <bit>
#endif

/**
 * returns 2 ** num;
 * Using union for manipulating float as an integer representation, 
 * is no more standard conforming AFAIK.
 * "You cannot use a union for type punning because you are not allowed to
 * first write to one member of the union, and then read from a different one. [cppreference:union]"
 * So I rewrote using memcpy.
 * (after C++20, std::bit_cast<float> can be used instead)
*/
static float d_pow2i(int num)
{
    uint32_t i_repr;
    i_repr = (((int) num) + D_FLOAT_BIAS) << 23;
#if __cplusplus >= 202002L
    return std::bit_cast<float> (i_repr);
#else
    float result;
    memcpy(&result, &i_repr, sizeof(i_repr));
    return result;
#endif
}

/**
 * exp2f taken from faster math
 * https://github.com/akohlmey/fastermath/tree/master
*/
static float d_exp2f(float x)
{
    float ipart, fpart;
    float epart;

    ipart = floorf(x + 0.5f);
    fpart = x - ipart;
    epart = d_pow2i((int)ipart);

    x =           d_exp2f_p[0];
    x = x*fpart + d_exp2f_p[1];
    x = x*fpart + d_exp2f_p[2];
    x = x*fpart + d_exp2f_p[3];
    x = x*fpart + d_exp2f_p[4];
    x = x*fpart + d_exp2f_p[5];
    x = x*fpart + d_exp2f_p[6];

    return epart*x;
}

static float d_expf(float x)
{
    return d_exp2f(D_FLOAT_LOG2OFE*x);
}


/**
 * Approximation of logf.  
*/
float approximate_logf(float x, float epsilon = 0.0001)
{
    if (x < 0) {
        // domain error
        return NAN;
    } else if (x == 0) {
        // returns negative infinity
        return -INFINITY;
    }
    
    int e;
    float m = frexp(x, &e);
    float yn = e * 0.69314718056f;  // using e * ln(2) as initial-value
    float yn1 = yn;
    const int MAX_ITERATIONS = 5;

    int iter = 0;
    do
    {
        iter++;
        yn = yn1;
        float expf_yn = d_expf(yn);
        yn1 = yn + 2 * (x - expf_yn) / (x + expf_yn);
    } while (iter < MAX_ITERATIONS && fabs(yn - yn1) > epsilon);

    // for large number about > 0.04, and e = 0.0001, iteration count is usually 3 or less.

//    printf("%d times iterated\n", iter);
    return yn1;
}

#if 0
#include <stdio.h>
static void test(float test_value)
{
    float approx = approximate_logf(test_value);
    float standard = logf(test_value);
    printf("approx   logf(%f) = %f\n", test_value, approx);
    printf("standard logf(%f) = %f\n", test_value, standard);
}

int main(void)
{
    for(float test_value = 0; test_value < 1; test_value += 0.01)
    {
        test(test_value);
    }
}
#endif

/**
 * one time convert method for a thermistor
 * */
float thermistor_converter_t::convert(const float T0, const float B, const float R0, const float RP,
        float adc_val_normalized)
{
    /*
        float Rt = (adc_val_normalized / (1.0 - adc_val_normalized)) * RP;
        float T0_K = T0 + K0;

        float T_K = 1.0 / (1.0 / T0_K + 1.0 / B * log(Rt / R0));

        return T_K - K0;  // ℃単位で返す
    */

   if(adc_val_normalized == 1.0)
        return (float)NAN; // possible divide by zero

    float T0_K = T0 + K0;
	float Rt = - (RP * adc_val_normalized) / (adc_val_normalized - 1.0);

	float T_K = (T0_K * B) / (T0_K * approximate_logf(Rt / R0) + B);
	return T_K - K0;

}


static thermistor_converter_t thermistor(THERMISTOR_T0, THERMISTOR_BETA, THERMISTOR_R0, THERMISTOR_RP);

/**
 * Read thermistor temperature at the charger IC, and retuns the temperature in deg C.
*/
float thermistor_read_charger()
{
    return thermistor.calc(adc_read_normalized(PIN_THERM_CHG));
}

/**
 * Read thermistor temperature at the li-ion battery, and retuns the temperature in deg C.
*/
float thermistor_read_battery()
{
    return thermistor.calc(adc_read_normalized(PIN_THERM_CHG));
}
