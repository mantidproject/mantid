# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import math

"""
   a,b: arrays
   n : size (to use)
   returns x (array) (was parameter by reference, modified in place)
"""


def CHOSOL_old(a, b, n):
    assert n == 3
    assert a.shape == (3, 3)
    assert b.shape == (3,)
    L = np.zeros([n, n])
    L[0, 0] = math.sqrt(a[0, 0])
    for i in range(1, n):
        L[i, 0] = a[i, 0] / L[0, 0]
        for j in range(1, i + 1):
            z = a[i, j] - np.dot(L[i, :j], L[j, :j])
            if z < 0:
                raise UserWarning("trapped a negative square root in CHOSOL_old")
                z = 1.0e-10
            if j == i:
                L[i, j] = math.sqrt(z)
            else:
                L[i, j] = z / L[j, j]

    bl = np.zeros([n])
    bl[0] = b[0] / L[0, 0]
    for i in range(1, n):
        z = np.dot(L[i, :i], bl[:i])
        bl[i] = (b[i] - z) / L[i, i]
    x = np.zeros([n])
    x[n - 1] = bl[n - 1] / L[n - 1, n - 1]
    for i in range(n - 2, -1, -1):
        z = np.dot(L[i + 1 : n, i], x[i + 1 : n])
        x[i] = (bl[i] - z) / L[i, i]
    return x


# solves ax=b by Cholesky decomposition
# only uses lower half of array a
# equivalent to np.linalg.solve(a,b) IF a is actually symmetric (Hermitian)
# exact equivalent code follows:


def CHOSOL(a, b, mylog):
    n = a.shape[0]
    try:
        L = np.linalg.cholesky(a)
    except:
        mylog.warning("np.linalg.cholesky failed, trying backup")
        mylog.debug("array a is " + str(a))
        L = np.zeros([n, n])
        L[0, 0] = math.sqrt(a[0, 0])
        for i in range(1, n):
            L[i, 0] = a[i, 0] / L[0, 0]
            for j in range(1, i + 1):
                z = a[i, j] - np.dot(L[i, :j], L[j, :j])
                if z < 0:
                    mylog.warning("trapped a negative square root in CHOSOL backup code")
                    z = 1.0e-10
                if j == i:
                    L[i, j] = math.sqrt(z)
                else:
                    L[i, j] = z / L[j, j]

    bl = np.zeros([n])
    for i in range(n):
        bl[i] = (b[i] - np.dot(L[i, :], bl)) / L[i, i]
    x = np.zeros([n])
    for i in range(n - 1, -1, -1):
        x[i] = (bl[i] - np.dot(L[:, i], x)) / L[i, i]
    return x
