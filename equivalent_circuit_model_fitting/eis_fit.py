import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from numpy import pi
from scipy.optimize import least_squares
import os
import glob

# -------- Find all relevant data files --------
path_pattern = "./CuIDE*.csv" 			#change to file path pattern
file_list = glob.glob(path_pattern)
if not file_list:
    raise FileNotFoundError(f"No files matching the pattern '{path_pattern}' were found in the current folder.")

print(f"Found {len(file_list)} files to process: {file_list}")

# -------- Helper functions --------
def pick(colnames, *keys):
    keys = [k.lower() for k in keys]
    for k in keys:
        for c in colnames:
            if k in c:
                return c
    return None

def Z_randles_cpe(params, w):
    Rs, Rct, Q, n = params
    Rs = abs(Rs); Rct = abs(Rct); Q = abs(Q); n = max(0.0, min(1.0, n))
    Y_cpe = Q*(1j*w)**n
    Y_branch = (1.0/Rct) + Y_cpe
    Z_branch = 1.0 / Y_branch
    return Rs + Z_branch

def Z_randles_c(params, w):
    Rs, Rct, C = params
    Rs = abs(Rs); Rct = abs(Rct); C = abs(C)
    Y_c = 1j*w*C
    Y_branch = (1.0/Rct) + Y_c
    Z_branch = 1.0 / Y_branch
    return Rs + Z_branch

def Z_cole(params, w):
    """Cole-Cole model: Rs + Rct / (1 + (jwT)^phi)"""
    Rs, Rct, T, phi = params
    Rs = abs(Rs); Rct = abs(Rct); T = abs(T); phi = max(0.0, min(1.0, phi))
    Z_branch = Rct / (1.0 + (1j*w*T)**phi)
    return Rs + Z_branch

def residuals_complex(params, model_func, z_meas, w, weights=None):
    z_model = model_func(params, w)
    if weights is None:
        weights = np.ones_like(z_meas.real)
    res = np.concatenate([weights*(z_model.real - z_meas.real),
                          weights*(z_model.imag - z_meas.imag)])
    return res

# -------- Loop through files, fit models, and store results --------
all_results = []
plot_data = {}

for path in sorted(file_list): # Sort to ensure consistent plot ordering
    print(f"\n--- Processing: {path} ---")
    
    # -------- Load and process data for the current file --------
    raw = pd.read_csv(path)
    df = raw.copy()
    df.columns = [str(c).strip().lower() for c in df.columns]

    col_f = pick(df.columns, "freq", "frequency", "f(Hz)", "hz")
    col_zreal = pick(df.columns, "zreal", "z'", "re(z)", "real", "z_re", "real(z)")
    col_zimag = pick(df.columns, "zimag", "z''", "im(z)", "imag", "imag(z)", "z_im")
    col_mag = pick(df.columns, "|z|", "mag", "mod", "abs", "magnitude")
    col_phase = pick(df.columns, "phase", "phi", "angle", "deg")

    if col_zreal is not None and col_zimag is not None:
        f = df[col_f].astype(float).values if col_f else None
        if f is None:
            raise ValueError(f"Frequency column not found in {path}.")
        zre = df[col_zreal].astype(float).values
        zim = df[col_zimag].astype(float).values
    elif col_mag is not None and col_phase is not None:
        f = df[col_f].astype(float).values if col_f else None
        if f is None:
            raise ValueError(f"Frequency column not found in {path}.")
        mag = df[col_mag].astype(float).values
        ph = df[col_phase].astype(float).values
        if np.nanmax(np.abs(ph)) > 3.2:
            ph = np.deg2rad(ph)
        zre = mag*np.cos(ph)
        zim = mag*np.sin(ph)
    else:
        raise ValueError(f"Could not infer impedance columns in {path}.")

    f = np.asarray(f).astype(float).ravel()
    zre = np.asarray(zre).astype(float).ravel()
    zim = np.asarray(zim).astype(float).ravel()
    if np.nanmean(zim) > 0:
        zim = -zim

    order = np.argsort(f) # Plot from low to high frequency
    f = f[order]; zre = zre[order]; zim = zim[order]

    Z = zre + 1j*zim
    w = 2*pi*f
    
    # -------- Fit models --------
    Rs0 = zre[np.argmin(np.abs(zim))]
    Rinf = np.min(zre)
    R0 = np.max(zre)
    Rct0 = max(R0 - Rs0, 1e-3)
    w_at_max_zim = w[np.argmax(-zim)]
    C0 = 1 / (w_at_max_zim * Rct0) if (w_at_max_zim * Rct0) > 0 else 1e-6
    Q0 = C0; n0 = 0.9
    T0 = 1 / w_at_max_zim; phi0 = 0.9

    z_meas = Z
    wts = 1/np.maximum(np.abs(z_meas), 1.0)
    
    # Fit Randles-CPE
    res_a = least_squares(residuals_complex, np.array([Rs0, Rct0, Q0, n0]),
                          args=(Z_randles_cpe, z_meas, w, wts), method='trf')
    za = Z_randles_cpe(res_a.x, w)
    rss_a = np.sum(residuals_complex(res_a.x, Z_randles_cpe, z_meas, w, wts)**2)
    k_a = len(res_a.x); nobs = len(f)*2
    aic_a = nobs*np.log(rss_a/nobs) + 2*k_a

    # Fit Randles-C
    res_b = least_squares(residuals_complex, np.array([Rs0, Rct0, C0]),
                          args=(Z_randles_c, z_meas, w, wts), method='trf')
    zb = Z_randles_c(res_b.x, w)
    rss_b = np.sum(residuals_complex(res_b.x, Z_randles_c, z_meas, w, wts)**2)
    k_b = len(res_b.x)
    aic_b = nobs*np.log(rss_b/nobs) + 2*k_b

    # Fit Cole
    res_c = least_squares(residuals_complex, np.array([Rs0, Rct0, T0, phi0]),
                          args=(Z_cole, z_meas, w, wts), method='trf')
    zc = Z_cole(res_c.x, w)
    rss_c = np.sum(residuals_complex(res_c.x, Z_cole, z_meas, w, wts)**2)
    k_c = len(res_c.x)
    aic_c = nobs*np.log(rss_c/nobs) + 2*k_c

    # Determine best model based on AIC
    best_aic = min(aic_a, aic_b, aic_c)
    if best_aic == aic_a:
        best_model = "Randles-CPE"; best_params = res_a.x; best_fit = za
        names = ["Rs", "Rct", "Q", "n"]
    elif best_aic == aic_b:
        best_model = "Randles-C"; best_params = res_b.x; best_fit = zb
        names = ["Rs", "Rct", "C"]
    else: # best_aic == aic_c
        best_model = "Cole"; best_params = res_c.x; best_fit = zc
        names = ["Rs", "Rct", "T", "phi"]
    
    print(f"Best model: {best_model}")
    
    # Store results
    result_dict = {"filename": os.path.basename(path), "model": best_model}
    for name, param in zip(names, best_params):
        result_dict[name] = param
        print(f"  {name}: {param:.4g}")
    all_results.append(result_dict)
    
    plot_data[path] = {'f': f, 'Z_measured': Z, 'Z_fitted': best_fit, 'model': best_model}

# -------- Consolidate and save parameters --------
param_df = pd.DataFrame(all_results)
param_df.to_csv("all_eis_fit_params.csv", index=False)
print("\n--- Consolidated Fit Parameters ---")
print(param_df)

# -------- Generate Comparative Plots --------
num_files = len(file_list)
colors = plt.cm.viridis(np.linspace(0, 1, num_files))

# Nyquist Plot
plt.figure(figsize=(7, 6))
for i, path in enumerate(sorted(plot_data.keys())):
    data = plot_data[path]
    label = os.path.basename(path)
    plt.plot(data['Z_measured'].real, -data['Z_measured'].imag, marker='o', linestyle='None', color=colors[i], label=f'Measured {label}')
    plt.plot(data['Z_fitted'].real, -data['Z_fitted'].imag, linestyle='-', color=colors[i], label=f'Fit ({data["model"]})')
plt.xlabel("Z' (Ohm)"); plt.ylabel("-Z'' (Ohm)"); plt.title("Nyquist Plot Comparison with Best Fit"); plt.legend()
plt.axis('equal'); plt.grid(True)
plt.tight_layout(); plt.savefig("nyquist_comparison.png", dpi=300)

# Bode Magnitude Plot
plt.figure(figsize=(7, 5))
for i, path in enumerate(sorted(plot_data.keys())):
    data = plot_data[path]
    label = os.path.basename(path)
    plt.loglog(data['f'], np.abs(data['Z_measured']), marker='o', linestyle='None', color=colors[i], label=f'Measured {label}')
    plt.loglog(data['f'], np.abs(data['Z_fitted']), linestyle='-', color=colors[i], label=f'Fit ({data["model"]})')
plt.xlabel("Frequency (Hz)"); plt.ylabel("|Z| (Ohm)"); plt.title("Bode Magnitude Comparison with Best Fit"); plt.legend()
plt.grid(True, which="both", ls="-")
plt.tight_layout(); plt.savefig("bode_magnitude_comparison.png", dpi=300)

# Bode Phase Plot
plt.figure(figsize=(7, 5))
for i, path in enumerate(sorted(plot_data.keys())):
    data = plot_data[path]
    label = os.path.basename(path)
    plt.semilogx(data['f'], np.rad2deg(np.angle(data['Z_measured'])), marker='o', linestyle='None', color=colors[i], label=f'Measured {label}')
    plt.semilogx(data['f'], np.rad2deg(np.angle(data['Z_fitted'])), linestyle='-', color=colors[i], label=f'Fit ({data["model"]})')
plt.xlabel("Frequency (Hz)"); plt.ylabel("Phase (deg)"); plt.title("Bode Phase Comparison with Best Fit"); plt.legend()
plt.grid(True, which="both", ls="-")
plt.tight_layout(); plt.savefig("bode_phase_comparison.png", dpi=300)

plt.show()
print("\nGenerated comparative plots: nyquist_comparison.png, bode_magnitude_comparison.png, bode_phase_comparison.png")
print("Saved all fit parameters to: all_eis_fit_params.csv")
