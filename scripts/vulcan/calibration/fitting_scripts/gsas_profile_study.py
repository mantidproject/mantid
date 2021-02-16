# High angle bank TOF fitting result
from matplotlib import pyplot as plt
import numpy as np


vec_d = np.array([0.603090, 0.630730, 0.686650, 0.728300, 0.818540, 0.891980, 1.075770])

# A(d) = alph0 + alpha1 x (1/d)
vec_a = np.array([0.243614, 0.222708, 0.197149, 0.179761, 0.161531, 0.141583, 0.119552])

# B(d) = beta0 + beta1 x (1/d**4)
vec_b = np.array([0.066596, 0.066054, 0.064946, 0.064118, 0.063240, 0.062179, 0.060781])

# S**2 = sig0 + sig1 x d**2 + sig2 x d**4
vec_s = np.array([5.314821, 5.745163, 6.574456, 7.202154, 8.572187, 9.773861, 12.390742])

# Fit Alpha
model_a = np.poly1d(np.polyfit(1./vec_d, vec_a, 1))
alpha0 = model_a.coefficients[1]
alpha1 = model_a.coefficients[0]
fitted_a_vec = model_a(1./vec_d)

print(f'alpha0 = {alpha0}, alpha1 = {alpha1}')

if False:
    plt.plot(1./vec_d, vec_a, linestyle='None', marker='o', color='black')
    plt.plot(1./vec_d, fitted_a_vec, linestyle=':', marker='d', color='red')
    plt.show()

# Fit Beta
model_b = np.poly1d(np.polyfit(1./vec_d**4, vec_b, 1))
beta0 = model_b.coefficients[1]
beta1 = model_b.coefficients[0]
fitted_b_vec = model_b(1./vec_d**4)

print(f'beta0 = {beta0}, beta1 = {beta1}')

if False:
    plt.plot(1./vec_d**4, vec_b, linestyle='None', marker='o', color='black')
    plt.plot(1./vec_d**4, fitted_b_vec, linestyle=':', marker='d', color='red')
    plt.show()

# Fit Sigma
model_s = np.poly1d(np.polyfit(vec_d**2, vec_s**2, 2))
sig0 = model_s.coefficients[2]
sig1 = model_s.coefficients[1]
sig2 = model_s.coefficients[0]
fitted_sq_s_vec = model_s(vec_d**2)

print(f'sig0 = {sig0}, sig1 = {sig1}, sig2 = {sig2}')

if True:
    plt.plot(vec_d**2, vec_s**2, linestyle='None', marker='o', color='black')
    plt.plot(vec_d**2, fitted_sq_s_vec, linestyle=':', marker='d', color='red')
    plt.show()

