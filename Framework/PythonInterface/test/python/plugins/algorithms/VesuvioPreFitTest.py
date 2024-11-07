# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Unit test for Vesuvio pre-fitting steps

Assumes that mantid can be imported and the data paths
are configured to find the Vesuvio data
"""

from mantid.api import AlgorithmManager

import numpy as np
import unittest

import vesuvio.testing as testing


class VesuvioPreFitTest(unittest.TestCase):
    _test_ws = None

    def setUp(self):
        if self._test_ws is not None:
            return
        # Cache a TOF workspace
        self.__class__._test_ws = testing.create_test_ws()

    # -------------- Success cases ------------------

    def test_smooth_uses_requested_number_of_points(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws, Smoothing="Neighbour", SmoothingOptions="NPoints=3", BadDataError=-1)
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        self.assertEqual(2, output_ws.getNumberHistograms())
        self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

        # Expected values
        expected_peak_height_spec1 = 0.4555026
        expected_peak_height_spec2 = 0.50082883
        expected_bin_index_spec1 = 139
        expected_bin_index_spec2 = 139

        # Peak height and bin index
        peak_height_spec1, bin_index_spec1 = self._get_peak_height_and_bin_index(output_ws.readY(0))
        peak_height_spec2, bin_index_spec2 = self._get_peak_height_and_bin_index(output_ws.readY(1))

        # Check first spectra matches expected
        self.assertTrue(self._equal_within_tolerance(expected_peak_height_spec1, peak_height_spec1))
        self.assertTrue(self._equal_within_tolerance(expected_bin_index_spec1, bin_index_spec1))

        # Check second spectra matches expected
        self.assertTrue(self._equal_within_tolerance(expected_peak_height_spec2, peak_height_spec2))
        self.assertTrue(self._equal_within_tolerance(expected_bin_index_spec2, bin_index_spec2))

    def test_mask_only_masks_over_threshold(self):
        err_start = self._test_ws.readE(1)[-1]
        self._test_ws.dataE(1)[-1] = 1.5e6

        alg = self._create_algorithm(InputWorkspace=self._test_ws, Smoothing="None", BadDataError=1.0e6)
        alg.execute()
        self._test_ws.dataE(1)[-1] = err_start
        output_ws = alg.getProperty("OutputWorkspace").value

        self.assertEqual(2, output_ws.getNumberHistograms())
        self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

        # Expected values
        expected_peak_height_spec1 = 0.4663805
        expected_peak_height_spec2 = 0.523506
        expected_bin_index_spec1 = 59
        expected_bin_index_spec2 = 139

        # Peak height and bin index
        peak_height_spec1, bin_index_spec1 = self._get_peak_height_and_bin_index(output_ws.readY(0))
        peak_height_spec2, bin_index_spec2 = self._get_peak_height_and_bin_index(output_ws.readY(1))

        # Check first spectra matches expected
        self.assertTrue(self._equal_within_tolerance(expected_peak_height_spec1, peak_height_spec1))
        self.assertTrue(self._equal_within_tolerance(expected_bin_index_spec1, bin_index_spec1))

        # Check second spectra matches expected
        self.assertTrue(self._equal_within_tolerance(expected_peak_height_spec2, peak_height_spec2))
        self.assertTrue(self._equal_within_tolerance(expected_bin_index_spec2, bin_index_spec2))

        # Check masked data
        self.assertAlmostEqual(0.0, output_ws.readY(1)[-1])

    # -------------- Failure cases ------------------

    def test_invalid_smooth_opt_raises_error_on_validate(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws, Smoothing="Neighbour", SmoothingOptions="npts=3")
        self.assertRaisesRegex(RuntimeError, "Invalid value for smoothing option. It must begin the format NPoints=3", alg.execute)

    # -------------- Helpers --------------------

    def _create_algorithm(self, **kwargs):
        alg = AlgorithmManager.createUnmanaged("VesuvioPreFit")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("OutputWorkspace", "__unused")
        for key, value in kwargs.items():
            alg.setProperty(key, value)
        return alg

    def _get_peak_height_and_bin_index(self, y_data):
        peak_height = np.amax(y_data)
        peak_bin = np.argmax(y_data)
        return peak_height, peak_bin

    def _equal_within_tolerance(self, expected, actual, tolerance=0.01):
        """
        Ensures the expected value matches the actual value within a tolerance (default 0.01)
        """
        return abs(expected - actual) < tolerance


if __name__ == "__main__":
    unittest.main()
