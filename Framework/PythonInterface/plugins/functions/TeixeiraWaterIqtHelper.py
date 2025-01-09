# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init
import numpy as np
from scipy.special import spherical_jn


# def safe_exp_multiply(x, factor):
#     """
#     Safely compute exp(x) * factor with overflow protection
#     """
#     try:
#         # Use log space to handle large numbers
#         if isinstance(x, np.ndarray):
#             # Handle array inputs
#             max_x = np.max(x)
#             if max_x > 700:  # numpy exp overflow threshold
#                 scaled_x = x - max_x
#                 return np.exp(scaled_x) * (factor * np.exp(max_x))
#         return np.exp(x) * factor
#     except FloatingPointError:
#         # If still getting overflow, try even more conservative approach
#         log_result = x + np.log(factor)
#         return np.exp(log_result)


# def functionTeixeiraWaterIQT(amp, tau1, gamma, q_value, radius, times):
#     """
#     Safe implementation of the Teixeira water function
#     """
#     qr = q_value * radius
#     j0 = spherical_jn(0, qr)
#     j1 = spherical_jn(1, qr)
#     j2 = spherical_jn(2, qr)

#     # Compute terms separately to avoid overflow
#     term0 = safe_exp_multiply(-gamma * np.abs(times), amp * j0**2)
#     term1 = safe_exp_multiply(
#         -(1 / (3 * tau1) + gamma) * np.abs(times), amp * 3 * j1**2
#     )
#     term2 = safe_exp_multiply(-(1 / tau1 + gamma) * np.abs(times), amp * 5 * j2**2)

#     return term0 + term1 + term2


def functionTeixeiraWaterIQT(amp, tau1, gamma, q_value, radius, xvals):
    xvals = np.array(xvals)
    qr = np.array(q_value * radius)
    j0 = spherical_jn(0, qr)
    j1 = spherical_jn(1, qr)
    j2 = spherical_jn(2, qr)

    with np.errstate(divide="ignore"):
        rotational = np.square(j0) + 3 * np.square(j1) * np.exp(-xvals / (3 * tau1)) + 5 * np.square(j2) * np.exp(-xvals / tau1)
        translational = np.exp(-gamma * xvals)
        iqt = amp * rotational * translational

    return iqt
