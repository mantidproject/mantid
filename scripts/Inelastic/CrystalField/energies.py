#pylint: disable=no-name-in-module
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import CrystalFieldEnergies
import numpy as np
import warnings


def _unpack_complex_matrix(packed, n_rows, n_cols):
    unpacked = np.ndarray(n_rows * n_cols, dtype=complex).reshape((n_rows, n_cols))
    for i in range(n_rows):
        for j in range(n_cols):
            k = 2 * (i * n_cols + j)
            value = complex(packed[k], packed[k + 1])
            unpacked[i, j] = value
    return unpacked


def energies(nre, **kwargs):
    """
    Calculate the crystal field energies and wavefunctions.

    Args:
        nre: a number denoting a rare earth ion
            |1=Ce|2=Pr|3=Nd|4=Pm|5=Sm|6=Eu|7=Gd|8=Tb|9=Dy|10=Ho|11=Er|12=Tm|13=Yb|

        kwargs: the keyword arguments for crystal field parameters.
            They can be:
            B20, B22, B40, B42, B44, ... : real parts of the crystal field parameters.
            IB20, IB22, IB40, IB42, IB44, ... : imaginary parts of the crystal field parameters.
            BmolX, BmolY, BmolZ: 3 molecular field parameters
            BextX, BextY, BextZ: 3 external field parameters

    Return:
        a tuple of energies (1D numpy array), wavefunctions (2D numpy array)
        and the hamiltonian (2D numpy array).
    """
    warnings.warn('This function is under development and can be changed/removed in the future',
                  FutureWarning)
    # Do the calculations
    res = CrystalFieldEnergies(nre, **kwargs)

    # Unpack the results
    eigenvalues = res[0]
    dim = len(eigenvalues)
    eigenvectors = _unpack_complex_matrix(res[1], dim, dim)
    hamiltonian = _unpack_complex_matrix(res[2], dim, dim)

    return eigenvalues, eigenvectors, hamiltonian
