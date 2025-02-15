# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np
from numpy.testing import assert_almost_equal


class AbinsSpectraTest(unittest.TestCase):
    """Test operations on euphonic-derived Spectrum classes"""

    def test_autoconvolution(self):
        from abins.sdata import AbinsSpectrum1DCollection, add_autoconvolution_spectra
        from euphonic import ureg

        # Check a trivial case: starting with a single peak,
        # expect evenly-spaced sequence of same intensity
        #
        # _|____ .... -> _|_|_|_|_ ...
        #
        y_data = np.zeros((1, 50)) * ureg("1 / meV")
        y_data[0, 2] = 1.0 * ureg("1 / meV")
        expected_y_data = np.zeros((10, 50)) * ureg("1 / meV")
        for i in range(10):
            expected_y_data[i][2 * (i + 1)] = 1.0 * ureg("1 / meV")

        spectra = AbinsSpectrum1DCollection(
            x_data=(np.linspace(0, 10, 50) * ureg("meV")),
            y_data=y_data,
            metadata={"test_key": "test_value", "line_data": [{"atom_index": 0, "quantum_order": 1}]},
        )

        spectra = add_autoconvolution_spectra(spectra)

        assert_almost_equal(spectra.y_data.magnitude, expected_y_data.magnitude)
        for i, row in enumerate(spectra):
            self.assertEqual(row.metadata, {"atom_index": 0, "quantum_order": i + 1, "test_key": "test_value"})

        # Check range restriction works, and beginning with more orders
        #
        # O1 _|____ ... + O2 __|___ ... -> O3 ___|___ ... + O4 ____|__ ...

        y_data = np.zeros((2, 50)) * ureg("1 / meV")
        y_data[0, 2] = 1.0 * ureg("1 / meV")
        y_data[1, 3] = 1.0 * ureg("1 / meV")
        spectra = AbinsSpectrum1DCollection(
            x_data=(np.linspace(0, 10, 50) * ureg("meV")),
            y_data=y_data,
            metadata={
                "test_key": "test_value",
                "line_data": [{"atom_index": 0, "quantum_order": 1}, {"atom_index": 0, "quantum_order": 2}],
            },
        )
        output_spectra = add_autoconvolution_spectra(spectra, max_order=4)

        # Check only the approriate orders were included
        assert set(spectrum.metadata["quantum_order"] for spectrum in output_spectra) == set(range(1, 5))
        for order in 1, 2:
            selection = output_spectra.select(quantum_order=order)
            self.assertEqual(len(selection), 1)
            assert_almost_equal(spectra.select(quantum_order=order).y_data.magnitude, selection.y_data.magnitude)

        for order in 3, 4:
            expected = np.zeros((1, 50))
            expected[0, (order - 2) * 2 + 3] = 1.0
            assert_almost_equal(output_spectra.select(quantum_order=order).y_data.magnitude, expected)
