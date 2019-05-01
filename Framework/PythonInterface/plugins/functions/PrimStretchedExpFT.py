# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

"""
@author Jose Borreguero, NScD
@date June 01, 2017
"""

from __future__ import (absolute_import, division, print_function)
import numpy as np

from mantid.api import IFunction1D, FunctionFactory
from StretchedExpFTHelper import surrogate, function1Dcommon


class PrimStretchedExpFT(IFunction1D):
    # pylint: disable=super-on-old-class
    def __init__(self):
        super(self.__class__, self).__init__()
        self._parmList = list()

    def category(self):
        return 'QuasiElastic'

    @surrogate
    def init(self):
        """Declare parameters that participate in the fitting"""
        pass

    @surrogate
    def validateParams(self):
        """Check parameters are positive"""
        pass

    def function1D(self, xvals, **optparms):
        r""" Fourier transform of the symmetrized stretched exponential integrated
        within each energy bin.

        The Symmetrized Stretched Exponential:
                height * exp( - |t/tau|**beta )

        The Fourier Transform:
            F(E) \int_{-infty}^{infty} (dt/h) e^{-i2\pi Et/h} f(t)
            F(E) is normalized:
                \int_{-infty}^{infty} dE F(E) = 1

        Let P(E) be the primitive of F(E) from minus infinity to E, then for element i of
        xvals we compute:
            1. bin_boundaries[i] = (xvals[i]+xvals[i+1])/2
            3. P(bin_boundaries[i+1])- P(bin_boundaries[i])
        :param xvals: list of values where to evaluate the function
        :param optparms: alternate list of function parameters
        :return: P(bin_boundaries[i+1])- P(bin_boundaries[i]), the difference of the primitive
        """
        rf = 16
        parms, de, energies, fourier = function1Dcommon(self, xvals, rf=rf, **optparms)
        if parms is None:
            return fourier # return zeros if parameters not valid
        denergies = (energies[-1] - energies[0]) / (len(energies)-1)
        # Find bin boundaries
        boundaries = (xvals[1:]+xvals[:-1])/2  # internal bin boundaries
        boundaries = np.insert(boundaries, 0, 2*xvals[0]-boundaries[0])  # external lower boundary
        boundaries = np.append(boundaries, 2*xvals[-1]-boundaries[-1])  # external upper boundary
        primitive = np.cumsum(fourier) * (denergies/(rf*de))  # running Riemann sum
        transform = np.interp(boundaries[1:] - parms['Centre'], energies, primitive) - \
            np.interp(boundaries[:-1] - parms['Centre'], energies, primitive)
        return transform * parms['Height']

    @surrogate
    def fillJacobian(self, xvals, jacobian, partials):
        """Fill the jacobian object with the dictionary of partial derivatives
        :param xvals: domain where the derivatives are to be calculated
        :param jacobian: jacobian object
        :param partials: dictionary with partial derivates with respect to the
        fitting parameters
        """
        pass

    @surrogate
    def functionDeriv1D(self, xvals, jacobian):
        """Numerical derivative except for Height parameter"""
        # partial derivatives with respect to the fitting parameters
        pass


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(PrimStretchedExpFT)
