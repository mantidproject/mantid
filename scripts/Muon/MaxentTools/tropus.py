# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
import numpy as np


def TROPUS(ox, SAVETIME_i2, PULSESHAPE_convol, DETECT_a, DETECT_b, DETECT_e):
    npts, ngroups = ox.shape
    n = PULSESHAPE_convol.shape[0]
    y = np.zeros([SAVETIME_i2], dtype=np.complex_)
    y[:npts] = np.dot(
        ox,
        DETECT_a) * DETECT_e + 1.j * np.dot(ox,
                                            DETECT_b) * DETECT_e
    y2 = np.fft.fft(y)  # SN=-1 meaning forward fft, scale is OK
    x = np.real(y2)[:n] * np.real(PULSESHAPE_convol) + \
        np.imag(y2)[:n] * np.imag(PULSESHAPE_convol)

    return x
