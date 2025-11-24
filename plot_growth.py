import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit

def model(n, mu, A, gamma):
    return A * (mu ** n) * (n ** gamma)

def log_model(n, log_mu, log_A, gamma):
    return log_A + n * log_mu + gamma * np.log(n)

try:
    df = pd.read_csv('knot_counts.csv')
    df = df[df['Count'] > 0] # Filter out zeros
    
    if len(df) < 3:
        print("Not enough data points to fit model yet.")
    else:
        x = df['Length'].values
        y = df['Count'].values
        log_y = np.log(y)
        
        # Fit log model: ln(y) = ln(A) + n * ln(mu) + gamma * ln(n)
        # We can use curve_fit on the log model
        popt, pcov = curve_fit(log_model, x, log_y, p0=[np.log(4.6), 0, 0])
        
        log_mu_fit, log_A_fit, gamma_fit = popt
        mu_fit = np.exp(log_mu_fit)
        A_fit = np.exp(log_A_fit)
        
        print(f"Fitted Parameters:")
        print(f"Growth Constant (mu): {mu_fit:.4f}")
        print(f"Amplitude (A): {A_fit:.4f}")
        print(f"Exponent (gamma): {gamma_fit:.4f}")
        
        plt.figure(figsize=(10, 6))
        plt.scatter(x, y, label='Data')
        
        x_smooth = np.linspace(min(x), max(x), 100)
        y_smooth = model(x_smooth, mu_fit, A_fit, gamma_fit)
        
        plt.plot(x_smooth, y_smooth, 'r-', label=f'Fit: $\mu={mu_fit:.2f}, \gamma={gamma_fit:.2f}$')
        plt.yscale('log')
        plt.xlabel('Length (N)')
        plt.ylabel('Number of Knots (SAP)')
        plt.title('Growth of Self-Avoiding Polygons')
        plt.legend()
        plt.grid(True, which="both", ls="-")
        plt.savefig('growth_plot.png')
        print("Plot saved to growth_plot.png")

except Exception as e:
    print(f"Error: {e}")
