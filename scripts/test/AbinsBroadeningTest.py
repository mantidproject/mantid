from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np

from AbinsModules.Instruments import Broadening

class AbinsBroadeningTest(unittest.TestCase):
    """
    Test Abins broadening functions
    """

    def test_broadening_values(self):
        """Check broadening implementations give similar values"""

        # Use dense bins with a single peak for fair comparison
        npts = 1000
        bins = np.linspace(0, 100, npts + 1)
        freq_points = (bins[1:] + bins[:-1]) / 2
        sigma = freq_points * 0.1 + 1
        s_dft = np.zeros(npts)
        s_dft[npts // 2] = 2

        schemes = ['gaussian', 'gaussian_truncated',
                   'normal', 'normal_truncated',
                   'interpolate']

        results = {}
        for scheme in schemes:
            _, results[scheme] = Broadening.broaden_spectrum(frequencies=freq_points,
                                                             bins=bins,
                                                             s_dft=s_dft,
                                                             sigma=sigma,
                                                             scheme=scheme)

        for scheme in schemes:
            # Interpolate scheme is approximate so just check a couple of sig.fig.
            if scheme == 'interpolate':
                places = 3
            else:
                places = 6
            self.assertAlmostEqual(results[scheme][(npts // 2) + 20],
                                   0.01257,
                                   places=places)


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

        # Gaussian scheme is normalised by values; truncated form still has correct sum
        for scheme in ('none',
                       'gaussian', 'gaussian_truncated',
                       ):
            freq_points, spectrum = Broadening.broaden_spectrum(frequencies=frequencies,
                                                                bins=bins,
                                                                s_dft=s_dft,
                                                                sigma=sigma,
                                                                scheme=scheme)
            self.assertAlmostEqual(sum(spectrum),
                                   pre_broadening_total,)

        # Normal scheme reproduces area as well as total;
        freq_points, full_spectrum = Broadening.broaden_spectrum(frequencies=frequencies,
                                                                 bins=bins,
                                                                 s_dft=s_dft,
                                                                 sigma=sigma,
                                                                 scheme='normal')
        self.assertAlmostEqual(np.trapz(spectrum, x=freq_points),
                               pre_broadening_total * (bins[1] - bins[0]),)
        self.assertAlmostEqual(sum(spectrum), pre_broadening_total)

        # truncated form will be a little off but shouldn't be _too_ off
        freq_points, trunc_spectrum = Broadening.broaden_spectrum(frequencies=frequencies,
                                                                 bins=bins,
                                                                 s_dft=s_dft,
                                                                 sigma=sigma,
                                                                 scheme='normal_truncated')
        self.assertLess(abs(sum(full_spectrum) - sum(trunc_spectrum)) / sum(full_spectrum),
                            0.03)

        # Interpolated methods need histogram input and smooth sigma
        hist_spec, _ = np.histogram(frequencies, bins, weights=s_dft)
        hist_sigma = sigma_func(freq_points)
        freq_points, interp_spectrum = Broadening.broaden_spectrum(frequencies=freq_points,
                                                                   bins=bins,
                                                                   s_dft=hist_spec,
                                                                   sigma=hist_sigma,
                                                                   scheme='interpolate')
        self.assertLess(abs(sum(interp_spectrum) - pre_broadening_total) / pre_broadening_total,
                            0.05)


if __name__ == '__main__':
    unittest.main()
