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
from scipy.special import i0e
from typing import Tuple


@cache
def cached_i0e(d_i: float, t: Tuple[float]) -> NDArray[float64]:
    return i0e(2 * d_i * np.array(t))


def autocorrelation_st(d_1: float, d_2: float, d_3: float, t: Tuple[float]) -> float:
    return cached_i0e(d_1, t) * cached_i0e(d_2, t) * cached_i0e(d_3, t)


def spectral_density_approximation_1d(d_1: float, d_2: float, w: NDArray[float64]) -> NDArray[float64]:
    return (1 / np.sqrt(4 * d_1 * d_2)) * np.sqrt((1 + np.sqrt(1 + np.power(w / (2 * d_2), 2))) / (1 + np.power(w / (2 * d_2), 2)))


def spectral_density(d_1: float, d_2: float, d_3: float, w: float) -> float:
    raise NotImplementedError("This is not implemented yet")


class SpinDiffusion(IFunction1D):

    # For integration over all directions of vector Q
    n_theta = 128
    d_theta = (np.pi / 2.0) / n_theta
    theta = d_theta * np.arange(1, n_theta)
    sin_theta = np.sin(theta)
    cos_theta = np.cos(theta)

    def category(self):
        return "Muon\\MuonSpecific"

    def init(self):
        # Active fitting parameters
        self.declareParameter("A", 1.0, "Amplitude, or Scaling factor")
        self.declareParameter("D1", 1e3, "Dipolar term 1 (MHz)")
        self.declareParameter("D2", 1e-2, "Dipolar term 2 (MHz)")
        self.declareParameter("D3", 1e-2, "Dipolar term 3 (MHz)")
        # Non-fitting parameters
        self.declareAttribute("NDimensions", 1)

    def function1D(self, xvals):
        r"""Calculate the intensities

        Parameters
        ----------
        xvals : sequence of floats
          The domain where to evaluate the function
        jacobian: 2D array
          partial derivative of the function with respect to the fitting
          parameters evaluated at the domain.

        Returns
        -------
        numpy.ndarray
            Function values
        """
        A = self.getParameterValue("A")
        d_1 = self.getParameterValue("D1")
        d_2 = self.getParameterValue("D2")
        d_3 = self.getParameterValue("D3")

        n_dimensions = int(self.getAttributeValue("NDimensions"))
        if n_dimensions == 1:
            spectral_density_results = spectral_density_approximation_1d(d_1 / d_1, d_2 / d_1, np.array(xvals))
        else:
            spectral_density_results = spectral_density(d_1 / d_1, d_2 / d_1, d_3 / d_1, "Not implemented")

        return np.square(A) / 4 * spectral_density_results


FunctionFactory.subscribe(SpinDiffusion)
