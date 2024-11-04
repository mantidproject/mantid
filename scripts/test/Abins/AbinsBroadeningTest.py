# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import numpy as np
from numpy.testing import assert_array_almost_equal
from scipy.stats import norm as spnorm

from abins.instruments import broadening


class BroadeningTest(unittest.TestCase):
    """
    Test Abins broadening functions
    """

    def test_gaussian(self):
        """Benchmark Gaussian against (slower) Scipy norm.pdf"""
        x = np.linspace(-10, 10, 101)
        diff = np.abs(spnorm.pdf(x) - broadening.gaussian(sigma=1, points=x, center=0))

        self.assertLess(max(diff), 1e-8)

        sigma, offset = 1.5, 4
        diff = np.abs(spnorm.pdf((x - offset) / sigma) / (sigma) - broadening.gaussian(sigma=sigma, points=x, center=offset))
        self.assertLess(max(diff), 1e-8)

    def test_mesh_gaussian_value(self):
        """Check reference values and empty cases for mesh_gaussian"""
        # Numerical values were not checked against an external reference
        # so they are only useful for detecting if the results have _changed_.

        self.assertEqual(broadening.mesh_gaussian(sigma=5, points=[], center=1).shape, (0,))

        zero_result = broadening.mesh_gaussian(
            sigma=np.array([[5]]),
            points=np.array(
                [
                    0,
                ]
            ),
            center=np.array([[3]]),
        )
        self.assertEqual(zero_result.shape, (1, 1))
        self.assertFalse(zero_result.any())

        assert_array_almost_equal(broadening.mesh_gaussian(sigma=2, points=np.array([0, 1]), center=0), np.array([0.199471, 0.176033]))
        assert_array_almost_equal(
            broadening.mesh_gaussian(sigma=np.array([[2], [2]]), points=np.array([0, 1, 2]), center=np.array([[0], [1]])),
            np.array([[0.199471, 0.176033, 0.120985], [0.176033, 0.199471, 0.176033]]),
        )

    def test_mesh_gaussian_sum(self):
        """Check sum of mesh_gaussian is correctly adapted to bin width"""
        # Note that larger bin widths will not sum to 1 with this theoretical normalisation factor; this is a
        # consequence of directly evaluating the Gaussian function. For coarse bins, consider using the "normal" kernel
        # which does not have this error.
        for bin_width in 0.1, 0.35:
            points = np.arange(-20, 20, bin_width)
            curve = broadening.mesh_gaussian(sigma=0.4, points=points)

            self.assertAlmostEqual(sum(curve), 1)

    def test_normal_sum(self):
        """Check that normally-distributed kernel sums to unity"""
        # Note that unlike Gaussian kernel, this totals intensity 1 even with absurdly large bins

        for bin_width in 0.1, 0.35, 3.1, 5:
            bins = np.arange(-20, 20, bin_width)
            curve = broadening.normal(sigma=0.4, bins=bins)

            self.assertAlmostEqual(sum(curve), 1)

    def test_broaden_spectrum_values(self):
        """Check broadening implementations give similar values"""

        # Use dense bins with a single peak for fair comparison
        npts = 1000
        bins = np.linspace(0, 100, npts + 1)
        freq_points = (bins[1:] + bins[:-1]) / 2
        sigma = freq_points * 0.1 + 1
        s_dft = np.zeros(npts)
        s_dft[npts // 2] = 2

        schemes = ["gaussian", "gaussian_truncated", "normal", "normal_truncated", "interpolate"]

        results = {}
        for scheme in schemes:
            _, results[scheme] = broadening.broaden_spectrum(freq_points, bins, s_dft, sigma, scheme)

        for scheme in schemes:
            # Interpolate scheme is approximate so just check a couple of sig.fig.
            if scheme == "interpolate":
                places = 3
            else:
                places = 6
            self.assertAlmostEqual(results[scheme][(npts // 2) + 20], 0.01257, places=places)

    def test_out_of_bounds(self):
        """Check schemes allowing arbitrary placement can handle data beyond bins"""
        frequencies = np.array([2000.0])
        bins = np.linspace(0, 100, 100)
        s_dft = np.array([1.0])
        sigma = np.array([3.0])

        schemes = ["none", "gaussian", "gaussian_truncated", "normal", "normal_truncated"]

        for scheme in schemes:
            broadening.broaden_spectrum(frequencies, bins, s_dft, sigma, scheme=scheme)

    def test_broadening_normalisation(self):
        """Check broadening implementations do not change overall intensity"""
        np.random.seed(0)

        # Use a strange bin width to catch bin-width-dependent behaviour
        bins = np.linspace(0, 5000, 2000)

        def sigma_func(frequencies):
            return 2 + frequencies * 1e-2

        n_peaks = 10
        frequencies = np.random.random(n_peaks) * 4000
        sigma = sigma_func(frequencies)
        s_dft = np.random.random(n_peaks) * 10

        pre_broadening_total = sum(s_dft)

        # Full Gaussian should reproduce null total
        for scheme in ("none", "gaussian"):
            freq_points, spectrum = broadening.broaden_spectrum(frequencies, bins, s_dft, sigma, scheme=scheme)
            self.assertAlmostEqual(
                sum(spectrum),
                pre_broadening_total,
            )

        # Normal scheme reproduces area as well as total;
        freq_points, full_spectrum = broadening.broaden_spectrum(frequencies, bins, s_dft, sigma, scheme="normal")
        self.assertAlmostEqual(
            np.trapezoid(spectrum, x=freq_points),
            pre_broadening_total * (bins[1] - bins[0]),
        )
        self.assertAlmostEqual(sum(spectrum), pre_broadening_total)

        # truncated forms will be a little off but shouldn't be _too_ off
        for scheme in ("gaussian_truncated", "normal_truncated"):
            freq_points, trunc_spectrum = broadening.broaden_spectrum(frequencies, bins, s_dft, sigma, scheme)
            self.assertLess(abs(sum(full_spectrum) - sum(trunc_spectrum)) / sum(full_spectrum), 0.03)

        # Interpolated methods need histogram input and smooth sigma
        hist_spec, _ = np.histogram(frequencies, bins, weights=s_dft)
        hist_sigma = sigma_func(freq_points)
        freq_points, interp_spectrum = broadening.broaden_spectrum(freq_points, bins, hist_spec, hist_sigma, scheme="interpolate")
        self.assertLess(abs(sum(interp_spectrum) - pre_broadening_total) / pre_broadening_total, 0.05)


if __name__ == "__main__":
    unittest.main()
