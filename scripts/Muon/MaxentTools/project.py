
from __future__ import (absolute_import, division, print_function)
import numpy as np


def PROJECT(k, n, xi):  # xi modified in place
    a = np.sum(xi[:n, k])
    a = a / n
    xi[:n, k] -= a
