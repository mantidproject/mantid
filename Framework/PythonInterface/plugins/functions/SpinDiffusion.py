# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import numpy as np
from functools import cache
from numpy import complex64, float64
from numpy.typing import NDArray
from mantid.api import IFunction1D, FunctionFactory
from mantid.kernel import IntBoundedValidator
from scipy.constants import elementary_charge, physical_constants
from scipy.fft import fft, fftshift
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


def perform_fft(d_1: float, d_2: float, d_3: float, t: Tuple[float]) -> NDArray[complex64]:
    """Calculate the discrete Fast Fourier Transform of S(t)."""
    g = autocorrelation_st(d_1, d_2, d_3, t)
    return fftshift(fft(g))


def d_rates(n_dimensions: int, d_par: float, d_perp: float) -> Tuple[float]:
    """Tie dipolar rates based on the dimensionality of the system."""
    match n_dimensions:
        case 1:  # D1 = D||           D2, D3 = D_|_
            return d_par / d_par, d_perp / d_par, d_perp / d_par
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
        # A = self.getParameterValue("A")
        d_par = self.getParameterValue("DParallel")
        d_perp = self.getParameterValue("DPerpendicular")

        n_dimensions = self.getAttributeValue("NDimensions")

        d_i_terms = d_rates(n_dimensions, d_par, d_perp)
        # w_values = MAGNETOGYRIC_RATIO * np.array(xvals)
        spectral_density_complex = perform_fft(*d_i_terms, tuple(xvals))

        # Convert from an NDArray(complex64) to a NDArray(float64)
        spectral_density_real = np.array(np.real(spectral_density_complex))

        return spectral_density_real  # np.square(A) / 4 * spectral_density_results


FunctionFactory.subscribe(SpinDiffusion)
