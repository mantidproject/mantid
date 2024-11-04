# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np


def OPUS(x, SAVETIME_i2, PULSESHAPE_convol, DETECT_a, DETECT_b, DETECT_e):
    npts = DETECT_e.shape[0]
    n = x.shape[0]
    y = np.zeros([SAVETIME_i2], dtype=np.complex128)
    y[:n] = x * PULSESHAPE_convol
    y2 = np.fft.ifft(y) * SAVETIME_i2  # SN=+1, inverse FFT without the 1/N
    ox = (np.outer(np.real(y2[:npts]), DETECT_a) + np.outer(np.imag(y2[:npts]), DETECT_b)) * DETECT_e[:, np.newaxis]
    return ox
