import numpy as np

def calculate_Rt(T, T0, B, R0):
    """Calculate the resistance of the thermistor at a given temperature."""
    return R0 * np.exp(B * (1/T - 1/T0))

def calculate_Vt(Rt, Rp, Vref=1.0):
    """Calculate the voltage across the thermistor for a given pull-up resistance."""
    return Vref * (Rt / (Rt + Rp))

def find_optimal_Rp(T0, B, R0, T_min, T_max):
    """Find the optimal pull-up resistor value for a given temperature range."""
    
    T0 = T0 + 273.15  # Convert to Kelvin
    T_min = T_min + 273.15
    T_max = T_max + 273.15

    # Calculate resistance range for the given temperature range
    Rt_min = calculate_Rt(T_max, T0, B, R0)  # Swap because resistance decreases with increasing temperature
    Rt_max = calculate_Rt(T_min, T0, B, R0)

    print(f"Rt_min (for T={T_max-273.15}C): {Rt_min:.2f} Ohms")
    print(f"Rt_max (for T={T_min-273.15}C): {Rt_max:.2f} Ohms\n")

    # Define a range of possible Rp values
    Rp_values = np.linspace(0.1 * R0, 10 * R0, 10000)  # From 0.1*R0 to 10*R0

    # Calculate ADC values for the whole Rp range
    Vt_min_values = calculate_Vt(Rt_min, Rp_values)
    Vt_max_values = calculate_Vt(Rt_max, Rp_values)

    # Print the ADC values for each Rp value
    for i, Rp in enumerate(Rp_values):
        if i % 1000 == 0:  # Print every 1000th value to not overwhelm the output
            print(f"Rp = {Rp:.2f} Ohms: Vt_min = {Vt_min_values[i]:.4f} V, Vt_max = {Vt_max_values[i]:.4f} V, Difference = {Vt_max_values[i] - Vt_min_values[i]:.4f} V")

    # Find the Rp value that maximizes the difference between ADC values
    diff = Vt_max_values - Vt_min_values
    optimal_Rp = Rp_values[np.argmax(diff)]

    return optimal_Rp

# Example usage:
T0 = 25.0
B = 3960
R0 = 10000  # 10kΩ
T_min = -20
T_max = 120

optimal_Rp = find_optimal_Rp(T0, B, R0, T_min, T_max)
print(f"\nThe optimal pull-up resistance for the given range is: {optimal_Rp:.2f} Ohms")
