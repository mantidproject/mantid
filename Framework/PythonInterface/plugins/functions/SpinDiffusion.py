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
from scipy.constants import elementary_charge, physical_constants
from scipy.integrate import quad
from scipy.special import i0e
from typing import Tuple

MAGNETOGYRIC_RATIO = physical_constants["muon g factor"][0] * -elementary_charge / (2 * physical_constants["muon mass"][0] * 1e6)  # MHz / T


@cache
def cached_i0e(d_i: float, t: Tuple[float]) -> NDArray[float64]:
    """Calculate the zeroth order modified Bessel function, scaled by an exponential term."""
    return i0e(2 * d_i * np.array(t))


def autocorrelation_st(d_1: float, d_2: float, d_3: float, t: Tuple[float]) -> float:
    """Calculate S(t), the autocorrelation function represented by an anisotropic random walk."""
    return cached_i0e(d_1, t) * cached_i0e(d_2, t) * cached_i0e(d_3, t)


def spectral_density_approximation_1d(d_parallel: float, d_perpendicular: float, w: NDArray[float64]) -> NDArray[float64]:
    """Calculate J(w) approximation for a 1D system."""
    return (1 / np.sqrt(4 * d_parallel * d_perpendicular)) * np.sqrt(
        (1 + np.sqrt(1 + np.power(w / (2 * d_perpendicular), 2))) / (1 + np.power(w / (2 * d_perpendicular), 2))
    )


def integrand(t: Tuple[float], d_1: float, d_2: float, d_3: float, w: float) -> float:
    "Integrand used to calculate J(w)."
    return autocorrelation_st(d_1, d_2, d_3, t) * np.cos(w * t)


def spectral_density(d_1: float, d_2: float, d_3: float, w: float, integration_upper: float) -> float:
    """Calculate J(w) for 2D and 3D systems."""
    integral, _ = quad(
        integrand,
        0,
        integration_upper,
        args=(
            d_1,
            d_2,
            d_3,
            w,
        ),
    )  # Note the integration range in the paper tends towards infinity
    return 2.0 * integral


def d_rates(n_dimensions: int, d_par: float, d_perp: float) -> Tuple[float]:
    """Tie dipolar rates based on the dimensionality of the system."""
    match n_dimensions:
        case 1:  # D1 = D||           D2, D3 = D_|_
            return d_par / d_par, d_perp / d_par
        case 2:  # D1, D2 = D||       D3 = D_|_
            return d_par / d_par, d_par / d_par, d_perp / d_par
        case 3:  # D1, D2, D3 = D||
            return d_par / d_par, d_par / d_par, d_par / d_par
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
        d_par = self.getParameterValue("DParallel")
        d_perp = self.getParameterValue("DPerpendicular")

        n_dimensions = self.getAttributeValue("NDimensions")

        d_i_terms = d_rates(n_dimensions, d_par, d_perp)
        w_values = MAGNETOGYRIC_RATIO * np.array(xvals)
        if n_dimensions == 1:
            spectral_density_results = spectral_density_approximation_1d(*d_i_terms, w_values)
        else:
            spectral_density_results = np.array([spectral_density(*d_i_terms, w, len(w_values) / 4) for w in w_values])

        return np.square(A) / 4 * spectral_density_results


FunctionFactory.subscribe(SpinDiffusion)
