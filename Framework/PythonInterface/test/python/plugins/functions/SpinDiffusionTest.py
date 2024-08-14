# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import FunctionWrapper

from numpy import corrcoef, linspace, log
from scipy.stats import linregress
import unittest


class SpinDiffusionTest(unittest.TestCase):

    def test_spin_diffusion_when_n_dimensions_one(self):
        x = linspace(0.01, 100.0, 1000)
        f = FunctionWrapper("SpinDiffusion", A=1.0, DParallel=1e3, DPerpendicular=1e-2, NDimensions=1)
        y = f(x)

        # We expect the NDimensions=1 case to follow a x^-(1/2) relationship
        slope, *_ = linregress(log(x), log(y))
        self.assertAlmostEqual(slope, -0.5, delta=0.01)

    def test_spin_diffusion_when_n_dimensions_two(self):
        x = linspace(0.01, 100.0, 1000)
        f = FunctionWrapper("SpinDiffusion", A=1.0, DParallel=1e3, DPerpendicular=1e-2, NDimensions=2)
        y = f(x)

        # We expect the NDimensions=2 case to follow a x^(-1) relationship
        coefficent_matrix = corrcoef(log(y), log(1 / x))
        # A correlation coefficient close to 1 indicates a strong linear relationship
        self.assertAlmostEqual(coefficent_matrix[0, 1], 1.0, delta=0.05)

    def test_spin_diffusion_when_n_dimensions_three(self):
        x = linspace(2.0, 100.0, 1000)
        f = FunctionWrapper("SpinDiffusion", A=1.0, DParallel=1e3, DPerpendicular=1e-2, NDimensions=3)
        y = f(x)

        # We expect the NDimensions=3 case to follow a x^(-3/2) relationship
        coefficent_matrix = corrcoef(log(y), log(1 / x ** (3 / 2)))
        # A correlation coefficient close to 1 indicates a strong linear relationship
        self.assertAlmostEqual(coefficent_matrix[0, 1], 1.0, delta=0.05)


if __name__ == "__main__":
    unittest.main()
