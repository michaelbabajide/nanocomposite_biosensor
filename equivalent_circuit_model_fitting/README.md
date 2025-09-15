# Electrochemical Impedance Spectroscopy (EIS) Model Fitting

This repository contains Python code to fit impedance spectroscopy data (`.csv` files) using several equivalent circuit models:
- Randles circuit with Constant Phase Element (Randles-CPE)
- Randles circuit with capacitor (Randles-C)
- Cole-Cole model

The code automatically:
- Loads `.csv` files (with flexible column recognition).
- Fits models using nonlinear least-squares optimization.
- Selects the best model based on Akaike Information Criterion (AIC).
- Saves fitted parameters to `all_eis_fit_params.csv`.
- Generates comparative Nyquist and Bode plots.

---

## Requirements

Install dependencies with:

```bash
pip install -r requirements.txt
