import numpy as np
import matplotlib.pyplot as plt
from scipy import stats

# --- Configuration ---
# Replace 'my_data.txt' with the actual path to your data file
filename = 'CSZnO1.txt'
# Set the expected approximate band gap (e.g., for ZnO ~3.3 eV) to help find the linear region.
expected_eg_approx = 3.4  # in eV
# ---------------------

def load_uvvis_data(filename):
    """
    Loads UV-Vis data from a text file.
    Expected format: "Wavelength nm.","Abs." followed by rows of data.
    """
    wavelengths = []
    absorbance = []
    
    with open(filename, 'r') as file:
        lines = file.readlines()
        # Skip the header line (e.g., "CSZnO1 - RawData", "Wavelength nm.,Abs.")
        # Start reading from the third line (index 2) to be safe.
        for line in lines[2:]:
            if line.strip():  # Skip empty lines
                parts = line.split(',')
                try:
                    wl = float(parts[0])
                    abs_val = float(parts[1])
                    wavelengths.append(wl)
                    absorbance.append(abs_val)
                except ValueError:
                    # Skip lines that can't be converted to floats
                    continue
    return np.array(wavelengths), np.array(absorbance)

def calculate_tauc(wavelengths, absorbance):
    """
    Calculates the transformed data for a Tauc plot (for direct band gap).
    Returns: photon_energy (eV), (alpha * hv)^2 (approximated by (A * hv)^2)
    """
    # Convert wavelength to photon energy (eV)
    # Avoid division by zero: filter out wavelengths <= 0
    valid_indices = wavelengths > 0
    wavelengths = wavelengths[valid_indices]
    absorbance = absorbance[valid_indices]
    
    photon_energy = 1240 / wavelengths  # Energy in eV
    
    # Calculate (A * hν)^2 as an approximation for (α * hν)^2
    ahv_squared = (absorbance * photon_energy) ** 2
    
    return photon_energy, ahv_squared

def find_bandgap(photon_energy, tauc_y, expected_eg, energy_range=0.5):
    """
    Finds the band gap by performing a linear fit on the linear region of the Tauc plot.
    The linear region is found around the expected band gap energy.
    
    Returns:
        eg (float): Estimated band gap energy (eV)
        fit_line (array): Y-values of the linear fit for plotting
        x_fit (array): X-values (photon energy) used for the fit
    """
    # 1. Find the data points in the region of interest around the expected Eg
    energy_mask = (photon_energy > expected_eg - energy_range) & (photon_energy < expected_eg + energy_range)
    x_data = photon_energy[energy_mask]
    y_data = tauc_y[energy_mask]
    
    if len(x_data) < 2:
        raise ValueError("Not enough data points near the expected band gap to perform a linear fit. Adjust `expected_eg_approx` or `energy_range`.")
    
    # 2. Perform linear regression (y = mx + c)
    slope, intercept, r_value, p_value, std_err = stats.linregress(x_data, y_data)
    
    # 3. The band gap Eg is the x-intercept (where y = 0)
    eg = -intercept / slope
    
    # 4. Create a fitted line for plotting, extending a bit past the Eg
    x_fit = np.linspace(eg - 0.2, eg + 0.2, 50)
    fit_line = slope * x_fit + intercept
    
    print(f"Linear Fit Results:")
    print(f"  Slope: {slope:.4f}")
    print(f"  Intercept: {intercept:.4f}")
    print(f"  R-squared: {r_value**2:.6f}")
    print(f"  Estimated Band Gap (Eg): {eg:.4f} eV")
    
    return eg, fit_line, x_fit

# --- MAIN ANALYSIS ---
print(f"Loading data from {filename}...")
wavelengths, absorbance = load_uvvis_data(filename)
print(f"Loaded {len(wavelengths)} data points.")

# 1. Plot the Raw UV-Vis Spectrum
plt.figure(figsize=(10, 6))
plt.plot(wavelengths, absorbance, 'b-', linewidth=1.5)
plt.title('UV-Vis Absorption Spectrum of CS-ZnO Nanocomposite', fontsize=14, fontweight='bold')
plt.xlabel('Wavelength (nm)', fontsize=12)
plt.ylabel('Absorbance (a.u.)', fontsize=12)
plt.grid(True, alpha=0.3)
plt.xlim(200, 800) # Adjust if your data range is different
plt.savefig('uv_vis_plot.png', dpi=300, bbox_inches='tight')
print("Raw UV-Vis plot saved as 'uv_vis_plot.png'")
plt.show()

# 2. Calculate Tauc Plot Data
print("\nCalculating Tauc plot data...")
photon_energy, tauc_y = calculate_tauc(wavelengths, absorbance)

# 3. Find the Band Gap and perform the linear fit
try:
    eg, fit_line, x_fit = find_bandgap(photon_energy, tauc_y, expected_eg_approx)
except ValueError as e:
    print(e)
    eg = None

# 4. Plot the Tauc Plot
plt.figure(figsize=(10, 6))
# Plot the transformed data
plt.plot(photon_energy, tauc_y, 'ro', markersize=4, label='Transformed Data')
if eg is not None:
    # Plot the linear fit and the extrapolated band gap
    plt.plot(x_fit, fit_line, 'k--', linewidth=2, label=f'Linear Fit (R² = {stats.linregress(x_fit, fit_line).rvalue**2:.4f})')
    plt.axvline(x=eg, color='green', linestyle=':', linewidth=2, label=f'Estimated E$_g$ = {eg:.3f} eV')

plt.title('Tauc Plot for Direct Band Gap Estimation', fontsize=14, fontweight='bold')
plt.xlabel('Photon Energy (eV)', fontsize=12)
plt.ylabel('(A·hν)$^2$ (a.u.)', fontsize=12)
plt.legend()
plt.grid(True, alpha=0.3)
# Focus on the relevant part of the plot
plt.xlim(photon_energy.min(), photon_energy.max())
plt.ylim(0, tauc_y.max() * 1.1)
plt.savefig('tauc_plot.png', dpi=300, bbox_inches='tight')
print("Tauc plot saved as 'tauc_plot.png'")
plt.show()
