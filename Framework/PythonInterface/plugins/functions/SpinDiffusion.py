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
from scipy.integrate import quad
from scipy.special import i0e
from typing import Tuple


@cache
def cached_i0e(d_i: float, t: Tuple[float]) -> NDArray[float64]:
    """Calculate the zeroth order modified Bessel function, scaled by an exponential term."""
    return i0e(2 * d_i * np.array(t))


def autocorrelation_st(d_1: float, d_2: float, d_3: float, t: Tuple[float]) -> float:
    """Calculate S(t), the autocorrelation function represented by an anisotropic random walk."""
    return cached_i0e(d_1, t) * cached_i0e(d_2, t) * cached_i0e(d_3, t)


def spectral_density_approximation_1d(d_1: float, d_2: float, w: NDArray[float64]) -> NDArray[float64]:
    """Calculate J(w) approximation for a 1D system."""
    return (1 / np.sqrt(4 * d_1 * d_2)) * np.sqrt((1 + np.sqrt(1 + np.power(w / (2 * d_2), 2))) / (1 + np.power(w / (2 * d_2), 2)))


def integrand(t: Tuple[float], d_1: float, d_2: float, d_3: float, w: float) -> float:
    "Integrand used to calculate J(w)."
    return autocorrelation_st(d_1, d_2, d_3, t) * np.cos(w * t)


def spectral_density(d_1: float, d_2: float, d_3: float, w: float, n_points: int) -> float:
    """Calculate J(w) for 2D and 3D systems."""
    integral, _ = quad(
        integrand,
        0,
        n_points / 4,
        args=(
            d_1,
            d_2,
            d_3,
            w,
        ),
    )  # Change integration range so it tends towards infinity
    return 2.0 * integral


class SpinDiffusion(IFunction1D):

    def category(self):
        return "Muon\\MuonSpecific"

    def init(self):
        # Fitting parameters
        self.declareParameter("A", 1.0, "Amplitude, or Scaling factor")
        self.declareParameter("D1", 1e3, "Dipolar term 1 (MHz)")
        self.declareParameter("D2", 1e-2, "Dipolar term 2 (MHz)")
        self.declareParameter("D3", 1e-2, "Dipolar term 3 (MHz)")
        # Non-fitting parameters
        self.declareAttribute("NDimensions", 1, IntBoundedValidator(lower=1, upper=3))

    def function1D(self, xvals):
        A = self.getParameterValue("A")
        d_1 = self.getParameterValue("D1")
        d_2 = self.getParameterValue("D2")
        d_3 = self.getParameterValue("D3")

        n_dimensions = int(self.getAttributeValue("NDimensions"))
        n_points = len(xvals)
        match n_dimensions:
            case 1:
                d_3 = d_2
                spectral_density_results = spectral_density_approximation_1d(d_1 / d_1, d_2 / d_1, np.array(xvals))
            case 2:
                d_2 = d_1
                spectral_density_results = np.array([spectral_density(d_1 / d_1, d_2 / d_1, d_3 / d_1, w, n_points) for w in xvals])
            case 3:
                d_3 = d_2 = d_1
                spectral_density_results = np.array([spectral_density(d_1 / d_1, d_2 / d_1, d_3 / d_1, w, n_points) for w in xvals])

        return np.square(A) / 4 * spectral_density_results


FunctionFactory.subscribe(SpinDiffusion)
