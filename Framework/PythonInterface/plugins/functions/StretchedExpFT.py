# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

"""
@author Jose Borreguero, NScD
@date October 06, 2013
"""

import numpy as np

from mantid.api import IFunction1D, FunctionFactory
from StretchedExpFTHelper import surrogate, function1Dcommon


class StretchedExpFT(IFunction1D):
    # pylint: disable=super-on-old-class
    def __init__(self):
        super(self.__class__, self).__init__()
        self._parmList = list()

    def category(self):
        return "QuasiElastic"

    @surrogate
    def init(self):
        """Declare parameters that participate in the fitting"""
        pass

    @surrogate
    def validateParams(self):
        """Check parameters are positive"""
        pass

    def function1D(self, xvals, **optparms):
        r"""Fourier transform of the Symmetrized Stretched Exponential

        The Symmetrized Stretched Exponential:
                height * exp( - |t/tau|**beta )

        The Fourier Transform:
            F(E) \int_{-infty}^{infty} (dt/h) e^{-i2\pi Et/h} f(t)
            F(E) is normalized:
                \int_{-infty}^{infty} dE F(E) = 1
        """
        parms, de, energies, fourier = function1Dcommon(self, xvals, **optparms)
        if parms is None:
            return fourier  # return zeros if parameters not valid
        transform = parms["Height"] * np.interp(xvals - parms["Centre"], energies, fourier)
        return transform

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
FunctionFactory.subscribe(StretchedExpFT)
