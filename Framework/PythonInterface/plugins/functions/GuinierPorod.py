# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
"""
@author Mathieu Doucet, ORNL
@date Oct 13, 2014
"""

import math
import numpy as np
from mantid.api import IFunction1D, FunctionFactory


class GuinierPorod(IFunction1D):
    """
    Provide a Guinier-Porod model for SANS.

    See Hammouda, J. Appl. Cryst. (2010) 43, 716-719
    """

    def category(self):
        return "SANS"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Scale", 1.0, "Scale")
        self.declareParameter("Dimension", 1.0, "Dimension")
        self.declareParameter("Rg", 50.0, "Radius of gyration")
        self.declareParameter("M", 3.0, "M")
        self.declareParameter("Background", 0.0, "Background")

    def _boundary_conditions(self, dummy_qval):
        """
        Check boundary constraints and return True if we
        are out of bounds.
        @param dummy_qval: q-value to evaluate at
        """
        s = self.getParameterValue("Dimension")
        Rg = self.getParameterValue("Rg")
        m = self.getParameterValue("M")
        if Rg <= 0:
            return True
        if m < s:
            return True
        if s > 3.0:
            return True
        return False

    def _guinier_porod_core(self, qval):
        """
        Compute the main function for the model
        @param qval: q-value to evaluate at
        """
        s = self.getParameterValue("Dimension")
        Rg = self.getParameterValue("Rg")
        m = self.getParameterValue("M")
        n = 3.0 - s
        if self._boundary_conditions(qval):
            return 0.0
        q1 = math.sqrt((m - s) * n / 2.0) / Rg
        if qval < q1:
            return math.pow(qval, -s) * math.exp((-qval * qval * Rg * Rg) / n)
        else:
            return math.pow(qval, -m) * math.pow(Rg, s - m) * math.exp((s - m) / 2.0) * math.pow((m - s) * n / 2.0, (m - s) / 2.0)

    def _first_derivative_dim(self, qval):
        """
        Compute the first derivative dI/d(Dimension)
         @param qval: q-value to evaluate at
        """
        s = self.getParameterValue("Dimension")
        Rg = self.getParameterValue("Rg")
        m = self.getParameterValue("M")
        n = 3.0 - s
        if self._boundary_conditions(qval):
            return 1.0
        q1 = math.sqrt((m - s) * n / 2.0) / Rg
        qrg = qval * qval * Rg * Rg
        if qval < q1:
            return -math.exp(-qrg / n) * math.pow(qval, -s) * math.log(qval) - math.exp(-qrg / n) * math.pow(qval, -s) * qrg / n / n
        else:
            result = (2.0 * s - m - 3.0) / (2.0 * (3.0 - s)) - 0.5 * (math.log(m - s) + math.log(3 - s))
            result += math.log(Rg) + math.log(2.0) + 1.0
            return result * math.pow(qval, -m) * math.pow(Rg, s - m) * math.exp((s - m) / 2.0) * math.pow((m - s) * n / 2.0, (m - s) / 2.0)

    def _first_derivative_m(self, qval):
        """
        Compute the first derivative dI/dM
        @param qval: q-value to evaluate at

        Derivatives can be obtained here:
        http://www.derivative-calculator.net/#expr=%28%28m-s%29%283-s%29%2F2%29%5E%28%28m%20-%20s%29%2F2%29q%5E-mr%5E%28s-m%29exp%28%28s-m%29%2F2%29&diffvar=m
        """
        s = self.getParameterValue("Dimension")
        Rg = self.getParameterValue("Rg")
        m = self.getParameterValue("M")
        n = 3.0 - s
        if self._boundary_conditions(qval):
            return 1.0
        q1 = math.sqrt((m - s) * n / 2.0) / Rg
        if qval < q1:
            return 0.0
        else:
            result = -math.log(qval) - math.log(Rg) - math.log(2.0) - 1.0
            result += (math.log(m - s) + math.log(3 - s)) / 2.0 + 0.5
            return result * math.pow(qval, -m) * math.pow(Rg, s - m) * math.exp((s - m) / 2.0) * math.pow((m - s) * n / 2.0, (m - s) / 2.0)

    def _first_derivative_rg(self, qval):
        """
        Compute the first derivative dI/d(Rg)
        @param qval: q-value to evaluate at
        """
        s = self.getParameterValue("Dimension")
        Rg = self.getParameterValue("Rg")
        m = self.getParameterValue("M")
        n = 3.0 - s
        if self._boundary_conditions(qval):
            return 1.0
        q1 = math.sqrt((m - s) * n / 2.0) / Rg
        qrg = qval * qval * Rg * Rg
        if qval < q1:
            return -2.0 * Rg * math.pow(qval, -s) * math.exp(-qrg / n) * qval * qval / n
        else:
            return (
                math.pow(qval, -m)
                * math.exp((s - m) / 2.0)
                * math.pow(((m - s) * n / 2.0), ((m - s) / 2.0))
                * (s - m)
                * math.pow(Rg, (s - m - 1))
            )

    def function1D(self, xvals):
        """
        Evaluate the model
        @param xvals: numpy array of q-values
        """
        # parameters
        scale = self.getParameterValue("Scale")
        bgd = self.getParameterValue("Background")

        output = np.zeros(len(xvals), dtype=float)
        for i, x in enumerate(xvals):
            output[i] = scale * self._guinier_porod_core(x) + bgd
        return output

    def functionDeriv1D(self, xvals, jacobian):
        """
        Evaluate the first derivatives
        @param xvals: numpy array of q-values
        @param jacobian: Jacobian object
        """
        i = 0
        for x in xvals:
            jacobian.set(i, 0, self._guinier_porod_core(x))
            jacobian.set(i, 1, self._first_derivative_dim(x))
            jacobian.set(i, 2, self._first_derivative_rg(x))
            jacobian.set(i, 3, self._first_derivative_m(x))
            jacobian.set(i, 4, 1.0)
            i += 1


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(GuinierPorod)
