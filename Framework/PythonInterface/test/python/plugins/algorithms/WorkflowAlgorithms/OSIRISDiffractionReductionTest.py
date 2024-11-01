# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods,invalid-name
import unittest
from mantid.api import MatrixWorkspace
from mantid.simpleapi import OSIRISDiffractionReduction


class OSIRISDiffractionReductionTest(unittest.TestCase):
    def _assert_valid_reduction_output(self, number_of_histograms: int, **kwargs):
        workspace = OSIRISDiffractionReduction(StoreInADS=False, **kwargs)

        self.assertTrue(isinstance(workspace, MatrixWorkspace), "Result workspace should be a matrix workspace.")
        self.assertEqual(workspace.getAxis(0).getUnit().unitID(), "dSpacing")
        self.assertEqual(workspace.getNumberHistograms(), number_of_histograms)

    def test_basic_reduction_completes(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """
        alg_kwargs = {
            "Sample": ["OSI89813.raw"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["osi89757.raw"],
            "SpectraMin": 3,
            "SpectraMax": 361,
        }
        self._assert_valid_reduction_output(1, **alg_kwargs)

    def test_reduction_with_can_subtraction(self):
        """
        Tests reduction after subtraction of an empty can.
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw"],
            "Container": ["OSI10241.raw"],
            "SpectraMin": 3,
            "SpectraMax": 361,
        }
        self._assert_valid_reduction_output(1, **alg_kwargs)

    def test_reduction_with_can_subtraction_and_scale(self):
        """
        Tests reduction after subtraction of an empty can.
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw"],
            "Container": ["OSI10241.raw"],
            "ContainerScaleFactor": 0.5,
            "SpectraMin": 3,
            "SpectraMax": 361,
        }
        self._assert_valid_reduction_output(1, **alg_kwargs)

    def test_reduction_with_multiple_runs(self):
        """
        Test reduction with multiple sample and vanadium runs
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw", "OSI10204.RAW"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw", "OSI10157.RAW"],
            "ContainerScaleFactor": 0.5,
            "SpectraMin": 3,
            "SpectraMax": 361,
        }
        self._assert_valid_reduction_output(1, **alg_kwargs)

    def test_reduction_with_multiple_sample_and_multiple_can(self):
        """
        Test reduction with multiple sample, vanadium and container runs
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw", "OSI10204.RAW"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw", "OSI10157.RAW"],
            "Container": ["OSI10241.raw", "OSI10242.RAW"],
            "ContainerScaleFactor": 0.5,
            "SpectraMin": 3,
            "SpectraMax": 361,
        }
        self._assert_valid_reduction_output(1, **alg_kwargs)

    def test_greater_sample_runs_no_container(self):
        """
        Test when the number of sample runs is greater than the number of vanadium runs
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw", "OSI10204.RAW"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw"],
            "SpectraMin": 3,
            "SpectraMax": 361,
        }
        self._assert_valid_reduction_output(1, **alg_kwargs)

    def test_greater_sample_runs_with_container(self):
        """
        Test when the number of sample runs is greater than the number of vanadium runs and a container is used
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw", "OSI10204.RAW"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw"],
            "Container": ["OSI10241.raw"],
            "SpectraMin": 3,
            "SpectraMax": 361,
        }
        self._assert_valid_reduction_output(1, **alg_kwargs)

    def test_greater_vanadium_runs_no_container(self):
        """
        Test when the number of vanadium runs is greater than the number of sample runs
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw", "OSI10157.RAW"],
            "SpectraMin": 3,
            "SpectraMax": 361,
        }
        self._assert_valid_reduction_output(1, **alg_kwargs)

    def test_greater_vanadium_runs_with_container(self):
        """
        Test when the number of vanadium runs is greater than the number of sample runs and a container is used
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw", "OSI10157.RAW"],
            "Container": ["OSI10241.raw"],
            "SpectraMin": 3,
            "SpectraMax": 361,
        }
        self._assert_valid_reduction_output(1, **alg_kwargs)

    def test_greater_container_runs(self):
        """
        Test when the number of container runs is greater than the number of sample or vanadium runs.
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw"],
            "Container": ["OSI10241.raw", "OSI10242.RAW"],
            "ContainerScaleFactor": 0.5,
            "SpectraMin": 3,
            "SpectraMax": 361,
        }
        self._assert_valid_reduction_output(1, **alg_kwargs)

    def test_reduction_with_n_groups_that_has_a_remainder(self):
        """
        Test when the GroupingMethod is Groups and there is a remaining number of groups.
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw"],
            "Container": ["OSI10241.raw"],
            "ContainerScaleFactor": 0.5,
            "SpectraMin": 3,
            "SpectraMax": 361,
            "GroupingMethod": "Groups",
            "NGroups": 5,
        }
        # There are 6 spectra because the last spectra contains the remainder
        self._assert_valid_reduction_output(6, **alg_kwargs)

    def test_reduction_with_n_groups_and_there_is_no_remainder(self):
        """
        Test when the GroupingMethod is Groups and there is no remainder.
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw"],
            "Container": ["OSI10241.raw"],
            "ContainerScaleFactor": 0.5,
            "SpectraMin": 12,
            "SpectraMax": 361,
            "GroupingMethod": "Groups",
            "NGroups": 10,
        }
        # The number of spectra is equally divisible into 10 so there is no extra group.
        self._assert_valid_reduction_output(10, **alg_kwargs)

    def test_reduction_with_a_custom_grouping_string(self):
        """
        Test when the GroupingMethod is Custom.
        """
        alg_kwargs = {
            "Sample": ["OSI10203.raw"],
            "CalFile": "osiris_041_RES10.cal",
            "Vanadium": ["OSI10156.raw"],
            "Container": ["OSI10241.raw"],
            "ContainerScaleFactor": 0.5,
            "SpectraMin": 12,
            "SpectraMax": 361,
            "GroupingMethod": "Custom",
            "GroupingString": "12-50,51-103,104+105,107:353",
        }
        self._assert_valid_reduction_output(250, **alg_kwargs)

    def test_mismatch_sample_container_d_ranges(self):
        """
        Test error handling when there is no overlap between the sample and container d-ranges
        """
        self.assertRaisesRegex(
            RuntimeError,
            "D-Ranges found in runs have no overlap",
            OSIRISDiffractionReduction,
            Sample=["OSI89813.raw"],
            CalFile="osiris_041_RES10.cal",
            Vanadium=["osi89757.raw"],
            Container=["OSI10241.raw"],
            SpectraMin=3,
            SpectraMax=361,
            OutputWorkspace="wks",
        )

    def test_spectra_with_bad_detectors(self):
        """
        Tests reduction with a spectra range that includes no selected Detectors from the Cal file
        """

        self.assertRaisesRegex(
            RuntimeError,
            "No selected Detectors found in .cal file for input range.",
            OSIRISDiffractionReduction,
            Sample=["OSI10203.raw"],
            CalFile="osiris_041_RES10.cal",
            Vanadium=["OSI10156.raw"],
            Container=["OSI10241.raw"],
            SpectraMin=16,
            SpectraMax=17,
            OutputWorkspace="wks",
        )

    def test_failure_no_vanadium(self):
        """
        Tests error handling when failed to obtain a vanadium run.
        """
        self.assertRaisesRegex(
            RuntimeError,
            "D-Ranges found in runs have no overlap",
            OSIRISDiffractionReduction,
            Sample=["OSI89813.raw"],
            CalFile="osiris_041_RES10.cal",
            Vanadium=["OSI10156.raw"],
            SpectraMin=3,
            SpectraMax=361,
            OutputWorkspace="wks",
        )


if __name__ == "__main__":
    unittest.main()
