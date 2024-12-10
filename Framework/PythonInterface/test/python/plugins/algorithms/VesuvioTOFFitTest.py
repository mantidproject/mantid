# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Unit test for Vesuvio reduction

Assumes that mantid can be imported and the data paths
are configured to find the Vesuvio data
"""

import unittest
import numpy as np
import sys
from mantid.api import AlgorithmManager
import vesuvio.testing as testing


class VesuvioTOFFitTest(unittest.TestCase):
    _test_ws = None

    def setUp(self):
        if self._test_ws is not None:
            return
        # Cache a TOF workspace
        self.__class__._test_ws = testing.create_test_ws()

    # -------------- Success cases ------------------

    def test_single_run_produces_correct_output_workspace_index0_kfixed_no_background(self):
        profiles = (
            "function=GramCharlier,width=[2, 5, 7],hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1;"
            "function=Gaussian,width=10;function=Gaussian,width=13;function=Gaussian,width=30;"
        )

        alg = self._create_algorithm(
            InputWorkspace=self._test_ws,
            WorkspaceIndex=0,
            Masses=[1.0079, 16, 27, 133],
            MassProfiles=profiles,
            IntensityConstraints="[0,1,0,-4]",
        )
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        self.assertEqual(7, output_ws.getNumberHistograms())
        self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

        # Expected values
        expected_peak_height_spec1 = 0.4663805
        expected_peak_height_spec2 = 0.14474478
        expected_bin_index_spec1 = 59
        expected_bin_index_spec2 = 159

        # Peak height and bin index
        peak_height_spec1, bin_index_spec1 = self._get_peak_height_and_bin_index(output_ws.readY(0))
        peak_height_spec2, bin_index_spec2 = self._get_peak_height_and_bin_index(output_ws.readY(1))

        # Check first spectra matches expected
        self.assertTrue(self._equal_within_tolerance(expected_peak_height_spec1, peak_height_spec1))
        self.assertTrue(self._equal_within_tolerance(expected_bin_index_spec1, bin_index_spec1))

        # Check second spectra matches expected
        self.assertTrue(self._equal_within_tolerance(expected_peak_height_spec2, peak_height_spec2))
        self.assertTrue(self._equal_within_tolerance(expected_bin_index_spec2, bin_index_spec2))

    def test_single_run_index0_kfixed_no_background_with_ties(self):
        profiles = (
            "function=GramCharlier,width=[2, 5, 7],hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1;"
            "function=Gaussian,width=10;function=Gaussian,width=13;function=Gaussian,width=30;"
        )
        ties = "f2.Intensity=f3.Intensity"

        alg = self._create_algorithm(
            InputWorkspace=self._test_ws,
            WorkspaceIndex=0,
            Masses=[1.0079, 16, 27, 133],
            MassProfiles=profiles,
            Ties=ties,
            IntensityConstraints="[0,1,0,-4]",
        )
        alg.execute()
        fit_params = alg.getProperty("FitParameters").value
        f2_intensity = fit_params.row(9)
        f3_intensity = fit_params.row(12)

        # Ensure the value of the f2.Intensity and f3.Intensity fit parameters are tied
        self.assertAlmostEqual(f2_intensity["Value"], f3_intensity["Value"])

    def test_single_run_produces_correct_output_workspace_index1_kfixed_no_background(self):
        profiles = (
            "function=GramCharlier,width=[2, 5, 7],hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1;"
            "function=Gaussian,width=10;function=Gaussian,width=13;function=Gaussian,width=30;"
        )

        alg = self._create_algorithm(
            InputWorkspace=self._test_ws,
            WorkspaceIndex=1,
            Masses=[1.0079, 16, 27, 133],
            MassProfiles=profiles,
            IntensityConstraints="[0,1,0,-4]",
        )
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        self.assertEqual(7, output_ws.getNumberHistograms())
        self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

        # Expected values
        expected_peak_height_spec1 = 0.523506
        expected_peak_height_spec2 = 0.147802
        expected_bin_index_spec1 = 139
        expected_bin_index_spec2 = 158

        # Peak height and bin index
        peak_height_spec1, bin_index_spec1 = self._get_peak_height_and_bin_index(output_ws.readY(0))
        peak_height_spec2, bin_index_spec2 = self._get_peak_height_and_bin_index(output_ws.readY(1))

        # Check first spectra matches expected
        self.assertTrue(self._equal_within_tolerance(expected_peak_height_spec1, peak_height_spec1))
        self.assertTrue(self._equal_within_tolerance(expected_bin_index_spec1, bin_index_spec1))

        # Check second spectra matches expected
        self.assertTrue(self._equal_within_tolerance(expected_peak_height_spec2, peak_height_spec2))
        self.assertTrue(self._equal_within_tolerance(expected_bin_index_spec2, bin_index_spec2))

    def test_single_run_produces_correct_output_workspace_index0_kfixed_including_background(self):
        profiles = (
            "function=GramCharlier,width=[2, 5, 7],hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1;"
            "function=Gaussian,width=10;function=Gaussian,width=13;function=Gaussian,width=30;"
        )
        background = "function=Polynomial,order=3"

        alg = self._create_algorithm(
            InputWorkspace=self._test_ws,
            WorkspaceIndex=0,
            Masses=[1.0079, 16.0, 27.0, 133.0],
            MassProfiles=profiles,
            Background=background,
            IntensityConstraints="[0,1,0,-4]",
        )

        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        self.assertEqual(8, output_ws.getNumberHistograms())
        self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

        # Expected values
        expected_peak_height_spec1 = 0.4663805
        expected_bin_index_spec1 = 59

        # Peak height and bin index
        peak_height_spec1, bin_index_spec1 = self._get_peak_height_and_bin_index(output_ws.readY(0))

        # Check first spectra matches expected
        self.assertTrue(self._equal_within_tolerance(expected_peak_height_spec1, peak_height_spec1))
        self.assertTrue(self._equal_within_tolerance(expected_bin_index_spec1, bin_index_spec1))

        # This is not producing good results on OSX
        if sys.platform != "darwin":
            expected_peak_height_spec2 = 0.1295382
            expected_bin_index_spec2 = 159
            peak_height_spec2, bin_index_spec2 = self._get_peak_height_and_bin_index(output_ws.readY(1))

            # Check second spectra matches expected
            self.assertTrue(self._equal_within_tolerance(expected_peak_height_spec2, peak_height_spec2))
            self.assertTrue(self._equal_within_tolerance(expected_bin_index_spec2, bin_index_spec2))

    # -------------- Failure cases ------------------

    def test_empty_masses_raises_error(self):
        alg = self._create_algorithm()

        self.assertRaises(ValueError, alg.setProperty, "Masses", [])

    def test_empty_functions_raises_error(self):
        alg = self._create_algorithm()

        self.assertRaises(ValueError, alg.setProperty, "MassProfiles", "")

    def test_number_functions_in_list_not_matching_length_masses_throws_error(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws, Masses=[1.0079, 33], MassProfiles="function=Gaussian,width=5;")

        self.assertRaisesRegex(RuntimeError, "Found 2 masses but 1 function definition", alg.execute)

    # -------------- Helpers --------------------

    def _create_algorithm(self, **kwargs):
        alg = AlgorithmManager.createUnmanaged("VesuvioTOFFit")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("OutputWorkspace", "__unused")
        alg.setProperty("FitParameters", "__unused")
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
