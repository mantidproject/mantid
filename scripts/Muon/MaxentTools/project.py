# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
import numpy as np


def PROJECT(k, n, xi):  # xi modified in place
    a = np.sum(xi[:n, k])
    a = a / n
    xi[:n, k] -= a
