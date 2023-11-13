#pragma once

void init_adc();
float adc_read_voltage(uint32_t pin);
float adc_read_normalized(uint32_t pin);