import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

def load_cor_file(filepath):
    """
    Reads a .cor CV file and returns a pandas DataFrame with columns:
    Potential (V), Current density (A/cm^2), Time (s)
    """
    with open(filepath, "r", errors="ignore") as f:
        lines = f.readlines()

    # Find where the data starts (after the "E(V)" header line)
    for i, line in enumerate(lines):
        if line.strip().startswith("E(V)"):
            start_idx = i + 2
            break

    # Load data into DataFrame
    data = pd.read_csv(filepath, sep="\t", skiprows=start_idx,
                       names=["E(V)", "i(A/cm^2)", "T(s)"],
                       engine="python")

    # Drop any empty rows
    data = data.dropna()

    return data

def analyze_cv(data, scan_rate, sample_name="CS-ZnO nanocomposite"):
    """
    Performs basic CV analysis: peak currents, symmetry, hysteresis.
    """
    # Extract potential and current
    E = data["E(V)"].astype(float).values
    I = data["i(A/cm^2)"].astype(float).values

    # Find anodic (oxidation) and cathodic (reduction) peaks
    anodic_peak_idx = np.argmax(I)
    cathodic_peak_idx = np.argmin(I)

    anodic_peak = (E[anodic_peak_idx], I[anodic_peak_idx])
    cathodic_peak = (E[cathodic_peak_idx], I[cathodic_peak_idx])

    deltaEp = anodic_peak[0] - cathodic_peak[0]  # peak-to-peak separation

    summary = {
        "Sample": sample_name,
        "Scan rate (mV/s)": scan_rate,
        "Anodic peak (E, i)": anodic_peak,
        "Cathodic peak (E, i)": cathodic_peak,
        "Peak-to-peak separation (V)": deltaEp
    }

    return summary

def plot_cv(data, scan_rate, ax=None):
    """
    Plots CV curve: Current density vs Potential.
    """
    if ax is None:
        fig, ax = plt.subplots(figsize=(6, 5))

    ax.plot(data["E(V)"], data["i(A/cm^2)"], label=f"{scan_rate} mV/s")
    ax.set_xlabel("Potential (V vs RE)")
    ax.set_ylabel("Current density (A/cm²)")
    ax.set_title("Cyclic Voltammetry (CS-ZnO nanocomposite)")
    ax.legend()
    ax.grid(True)

    return ax

# ---------------- MAIN ----------------
if __name__ == "__main__":
    files = {
        "5": "5 mVs-1 (New).cor",
        "50": "50 mVs-1 (New).cor",
        "100": "100 mVs-1 (New).cor"
    }

    results = []

    fig, ax = plt.subplots(figsize=(7,6))

    for rate, filepath in files.items():
        if os.path.exists(filepath):
            data = load_cor_file(filepath)
            summary = analyze_cv(data, scan_rate=int(rate))
            results.append(summary)
            plot_cv(data, scan_rate=rate, ax=ax)

    # Save the figure before showing
    plt.tight_layout()
    plt.savefig("CV_comparison.png", dpi=300)   # saves as high-resolution PNG
    # plt.savefig("CV_comparison.pdf")          # optional: vector format

    plt.show()

    # Print analysis summary
    for r in results:
        print("\n--- CV Analysis Summary ---")
        for k, v in r.items():
            print(f"{k}: {v}")
