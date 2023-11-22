"""
Thermistor Resistance and Voltage Calculator

This script calculates the optimal resistance values for a voltage divider network
that includes a thermistor, used for temperature sensing within a specified range.
The circuit configuration consists of a fixed resistor (Rd) connected to ground, 
followed by the thermistor, and another fixed resistor (Ru) connected to the power supply (Vdd).
The junction between the thermistor and Ru is connected to an ADC input.

The script finds the best values for Rd and Ru such that the voltage at the ADC input
stays within a specified range (ADC_min to ADC_max) over the entire temperature range of interest.
This ensures accurate and reliable temperature readings from the thermistor through the ADC.

Parameters:
- T0 (float): Reference temperature (in Celsius) at which the thermistor resistance is known (R0).
- B (float): Beta value of the thermistor.
- R0 (float): Resistance of the thermistor at the reference temperature T0.
- T_min, T_max (float): Minimum and maximum temperatures (in Celsius) in the range of interest.
- Vdd (float): Power supply voltage.
- ADC_min, ADC_max (float): Minimum and maximum input voltage range of the ADC (in volts).

The script outputs the optimal values for Rd and Ru, as well as the thermistor resistance
at the minimum and maximum temperatures, and the corresponding minimum and maximum voltages at the ADC input.

Example:
    To run the script, just execute it in a Python environment with numpy installed.
    Modify the parameters T0, B, R0, T_min, T_max, Vdd, ADC_min, and ADC_max as needed.

"""

import numpy as np

def calculate_Rt(T, T0, B, R0):
    """Calculate the resistance of the thermistor at a given temperature."""
    T_K = T + 273.15  # Convert to Kelvin
    T0_K = T0 + 273.15
    return R0 * np.exp(B * (1/T_K - 1/T0_K))

def calculate_Vout(Rt, Rd, Ru, Vdd):
    """Calculate the voltage at the ADC input."""
    return Vdd * ((Rt + Rd) / (Rt + Ru + Rd))

def find_best_resistors(T0, B, R0, T_min, T_max, Vdd, ADC_min, ADC_max):
    Rt_max = calculate_Rt(T_min, T0, B, R0)  # Resistance at T_min (Higher Temperature, Lower Resistance)
    Rt_min = calculate_Rt(T_max, T0, B, R0)  # Resistance at T_max (Lower Temperature, Higher Resistance)
    print(f"Rt_max {Rt_max}, Rt_min {Rt_min}")

    rd_values = np.linspace(1,   100000, 5000)  # Example range
    ru_values = np.linspace(100, 600000, 5000)  # Example range
    best_Rd = 0
    best_Ru = 0
    best_diff = 0
    counter = 0
    diff = 0  # Initialize diff

    for Rd in rd_values:
        for Ru in ru_values:
            Vout_min = calculate_Vout(Rt_min, Rd, Ru, Vdd)
            Vout_max = calculate_Vout(Rt_max, Rd, Ru, Vdd)


            if ADC_min <= Vout_min <= ADC_max and ADC_min <= Vout_max <= ADC_max:
                diff = Vout_max - Vout_min
                if diff > best_diff:
                    best_diff = diff
                    best_Rd = Rd
                    best_Ru = Ru

            # Display the process every 1000 iterations
            if counter % 99931 == 0:
                print(f"Iteration {counter}: Rd = {Rd:.2f}, Ru = {Ru:.2f}, Vout_min = {Vout_min:.3f}, Vout_max = {Vout_max:.3f}, Diff = {diff:.3f}")
                print(f"ADC_min:{ADC_min}, ADC_max:{ADC_max}, Vout_min:{Vout_min}, Vout_max:{Vout_max}")
            counter += 1

    return best_Rd, best_Ru

# Example parameters
T0 = 25.0
B = 3960
R0 = 10000
T_min = -20
T_max = 120
Vdd = 3.3
ADC_min = 0.1  # 100 mV
ADC_max = 0.95  # 950 mV

best_Rd, best_Ru = find_best_resistors(T0, B, R0, T_min, T_max, Vdd, ADC_min, ADC_max)
# Calculate Rt_min and Rt_max for the final output
Rt_max = calculate_Rt(T_min, T0, B, R0)  # Resistance at T_min
Rt_min = calculate_Rt(T_max, T0, B, R0)  # Resistance at T_max

# Calculate final Vout_min and Vout_max using the best Rd and Ru
final_Vout_min = calculate_Vout(Rt_min, best_Rd, best_Ru, Vdd)
final_Vout_max = calculate_Vout(Rt_max, best_Rd, best_Ru, Vdd)

print(f"\nBest Rd: {best_Rd:.2f} Ohms, Best Ru: {best_Ru:.2f} Ohms")
print(f"Rt_max: {Rt_max:.2f} Ohms, Rt_min: {Rt_min:.2f} Ohms")
print(f"Final Vout_min: {final_Vout_min:.3f} V, Final Vout_max: {final_Vout_max:.3f} V")
