# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init
import numpy as np
from scipy.special import spherical_jn


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
