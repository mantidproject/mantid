# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

# param P not used!
# N is number of frequency points provided
# 2**I2PWR is length of FFT (zero pad spectrum). Replace with I2
# npts is number of time bins (truncate result)
# PULSESHAPE_convol is complex (convolR + i*convolI)


def ZFT(f, PULSESHAPE_convol, DETECT_e, SAVETIME_i2):
    n = f.shape[0]
    npts = DETECT_e.shape[0]
    y = np.zeros([SAVETIME_i2], dtype=np.complex128)
    y[:n] = f * PULSESHAPE_convol
    y2 = np.fft.ifft(y) * SAVETIME_i2  # SN=+1 meaning inverse FFT without the 1/N scale factor
    return np.real(y2[:npts]) * DETECT_e, np.imag(y2[:npts]) * DETECT_e
