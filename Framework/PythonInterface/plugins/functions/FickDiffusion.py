# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
'''
@author Spencer Howells, ISIS
@date December 05, 2013
'''

from __future__ import (absolute_import, division, print_function)
from mantid.api import IFunction1D, FunctionFactory


class FickDiffusion(IFunction1D):

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("D", 1.0, 'Diffusion constant')

    def function1D(self, xvals):
        return self.getParameterValue("D")*xvals*xvals

    def functionDeriv1D(self, xvals, jacobian):
        i = 0
        for x in xvals:
            jacobian.set(i, 0, 2.0*x)
            i += 1


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(FickDiffusion)
