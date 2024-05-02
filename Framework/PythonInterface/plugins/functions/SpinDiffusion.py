# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import numpy as np
from functools import cache
from numpy import float64
from numpy.typing import NDArray
from mantid.api import IFunction1D, FunctionFactory
from mantid.kernel import IntBoundedValidator
from mantid.kernel import PhysicalConstants
from scipy.integrate import quad
from scipy.special import i0e
from typing import Tuple

MUON_MAGNETOGYRIC_RATIO = 2 * np.pi * PhysicalConstants.MuonGyromagneticRatio  # MHz / G


@cache
def cached_i0e(d_i: float, t: Tuple[float]) -> NDArray[float64]:
    """Calculate the zeroth order modified Bessel function, scaled by an exponential term."""
    return i0e(2 * d_i * np.array(t))


def autocorrelation(t: Tuple[float], d_1: float, d_2: float, d_3: float) -> NDArray[float64]:
    """Calculate S(t), the autocorrelation function represented by an anisotropic random walk."""
    return cached_i0e(d_1, t) * cached_i0e(d_2, t) * cached_i0e(d_3, t)


def spectral_density(d_1, d_2, d_3, w):
    """Calculate J(w), the spectral density function."""
    integral, _ = quad(
        autocorrelation,
        0,
        np.inf,  # Integration upper bound
        args=(
            d_1,
            d_2,
            d_3,
        ),
        weight="cos",
        wvar=w,
    )
    return 2 * integral


def d_rates_from_dimensionality(n_dimensions: int, d_parallel: float, d_perpendicular: float) -> Tuple[float]:
    """Tie dipolar rates based on the dimensionality of the system."""
    match n_dimensions:
        case 1:  # D1 = D||           D2, D3 = D_|_
            return d_parallel, d_perpendicular, d_perpendicular
        case 2:  # D1, D2 = D||       D3 = D_|_
            return d_parallel, d_parallel, d_perpendicular
        case 3:  # D1, D2, D3 = D||
            return d_parallel, d_parallel, d_parallel
        case _:
            raise RuntimeError("The number of dimensions has an upper bound of 3.")


class SpinDiffusion(IFunction1D):

    def category(self):
        return "Muon\\MuonSpecific"

    def init(self):
        # Fitting parameters
        self.declareParameter("A", 1.0, "Amplitude, or Scaling factor")
        self.declareParameter("DParallel", 1e3, "Dipolar parallel, the fast rate dipolar term (MHz)")
        self.declareParameter("DPerpendicular", 1e-2, "Dipolar perpendicular, the slow rate dipolar term (MHz)")
        # Non-fitting parameters
        self.declareAttribute("NDimensions", 1, IntBoundedValidator(lower=1, upper=3))

    def function1D(self, xvals):
        A = self.getParameterValue("A")
        d_parallel = self.getParameterValue("DParallel")
        d_perpendicular = self.getParameterValue("DPerpendicular")

        n_dimensions = self.getAttributeValue("NDimensions")

        d_rates = d_rates_from_dimensionality(n_dimensions, d_parallel, d_perpendicular)

        # xvals is assumed to be B(LF), in units of Gauss. We need to convert to inverse MHz
        w_values = MUON_MAGNETOGYRIC_RATIO * np.array(xvals)

        jw = [spectral_density(*d_rates, w) for w in w_values]

        return np.square(A) / 4 * np.array(jw)


FunctionFactory.subscribe(SpinDiffusion)
