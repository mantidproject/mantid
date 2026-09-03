# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.dataobjects import GroupingWorkspace
from mantid.simpleapi import Load
from IndirectReductionCommon import (
    create_detector_grouping_string,
    create_grouping_string,
    create_grouping_workspace,
    create_range_string,
    group_spectra_by_theta,
    group_spectra_of,
    _excluded_detector_ids,
    _get_x_range_when_bins_vary,
)


class IndirectReductionCommonTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        workspace = Load(Filename="OSI10203.raw", StoreInADS=False)
        cls._workspace = workspace

    def test_create_range_string_returns_expected_string(self):
        self.assertEqual("3-5", create_range_string(3, 5))

    def test_create_grouping_string_returns_expected_strings(self):
        self.assertEqual("3-7,8-12,13-17,18-22,23-27", create_grouping_string(5, 5, 3))
        self.assertEqual("0-2,3-5,6-8,9-11", create_grouping_string(3, 4, 0))
        self.assertEqual("12-21,22-31", create_grouping_string(10, 2, 12))

    def test_create_detector_grouping_gives_assertion_error_when_spectra_min_is_greater_than_spectra_max(self):
        with self.assertRaisesRegex(AssertionError, "Spectra min cannot be larger than spectra max."):
            _ = create_detector_grouping_string(2, 3, 2)

    def test_create_detector_grouping_gives_assertion_error_when_number_of_groups_is_zero(self):
        with self.assertRaisesRegex(AssertionError, "Number of groups must be greater than zero."):
            _ = create_detector_grouping_string(0, 3, 6)

    def test_create_detector_grouping_gives_assertion_error_when_number_of_groups_is_greater_than_number_of_spectra(self):
        with self.assertRaisesRegex(AssertionError, "Number of groups must be less or equal to the number of spectra."):
            _ = create_detector_grouping_string(5, 3, 6)

    def test_create_detector_grouping_for_divisible_number_of_groups(self):
        self.assertEqual("3-6,7-10,11-14,15-18,19-22,23-26,27-30,31-34", create_detector_grouping_string(8, 3, 34))

    def test_create_detector_grouping_for_non_divisible_number_of_groups(self):
        # An extra group is created with the remaining detectors
        self.assertEqual("7-9,10-12,13-15,16-18,19-21,22-24,25-27,28-32", create_detector_grouping_string(7, 7, 32))

    def test_create_grouping_workspace_will_create_a_workspace_with_the_expected_size(self):
        grouping_workspace = create_grouping_workspace(self._workspace, "osiris_041_RES10.cal")

        self.assertTrue(isinstance(grouping_workspace, GroupingWorkspace))
        self.assertEquals(2562, grouping_workspace.getNumberHistograms())

    def test_excluded_detector_ids_returns_the_expected_detector_ids(self):
        grouping_workspace = create_grouping_workspace(self._workspace, "osiris_041_RES10.cal")

        excluded_ids = _excluded_detector_ids(grouping_workspace)

        self.assertEquals(2458, len(excluded_ids))
        self.assertEquals([i for i in range(16, 116)], excluded_ids[:100])

    def test_get_x_range_when_bins_vary_returns_the_expected_min_and_max_x(self):
        grouping_workspace = create_grouping_workspace(self._workspace, "osiris_041_RES10.cal")

        x_min, x_max = _get_x_range_when_bins_vary(self._workspace, grouping_workspace)

        self.assertEqual(50100.00, x_min)
        self.assertEqual(70100.00, x_max)


class GroupSpectraByThetaTest(unittest.TestCase):
    """Tests for group_spectra_by_theta using OSIRIS silicon analyser data.

    OSI100320_silicon_test.nxs is used with spectra_range=[1005, 2564] so that
    spectrum numbers (starting at 1005) differ from workspace indices (0-based).
    This is an explicit regression check for the bug where workspace indices
    were passed as spectrum numbers to GroupDetectors.SpectraList, causing
    "All list properties are empty, nothing to group".
    """

    _SPECTRA_MIN = 1005  # OSIRIS silicon analyser spectra-min (from IPF)
    _SPECTRA_MAX = 2564  # OSIRIS silicon analyser spectra-max (from IPF)

    @classmethod
    def setUpClass(cls) -> None:
        cls._workspace = Load(Filename="OSI100320_silicon_test.nxs", StoreInADS=False)

    def test_returns_requested_number_of_groups(self):
        result = group_spectra_by_theta(self._workspace, number_of_groups=3, spectra_range=[self._SPECTRA_MIN, self._SPECTRA_MAX])
        self.assertEqual(3, result.getNumberHistograms())

    def test_single_group_combines_all_spectra_into_one(self):
        result = group_spectra_by_theta(self._workspace, number_of_groups=1, spectra_range=[self._SPECTRA_MIN, self._SPECTRA_MAX])
        self.assertEqual(1, result.getNumberHistograms())

    def test_number_of_groups_does_not_exceed_requested_count(self):
        # Empty theta bins are dropped, so output histograms <= number_of_groups
        result = group_spectra_by_theta(self._workspace, number_of_groups=5, spectra_range=[self._SPECTRA_MIN, self._SPECTRA_MAX])
        self.assertLessEqual(result.getNumberHistograms(), 5)

    def test_raises_when_spectra_range_contains_no_valid_detectors(self):
        with self.assertRaisesRegex(RuntimeError, "No valid detectors found"):
            group_spectra_by_theta(self._workspace, number_of_groups=3, spectra_range=[99999, 99999])


class GroupSpectraDetectorsTest(unittest.TestCase):
    """Tests for the 'Detectors' grouping method in group_spectra_of."""

    @classmethod
    def setUpClass(cls) -> None:
        cls._si_workspace = Load(Filename="OSI100320_silicon_test.nxs", StoreInADS=False)
        # Graphite workspace has no Workflow.DetectorsGroupingFile in its IPF
        cls._pg_workspace = Load(Filename="OSI10203.raw", StoreInADS=False)

    def test_groups_spectra_using_detectors_grouping_file(self):
        # The SI workspace IPF defines Workflow.DetectorsGroupingFile pointing
        # to OSIRIS_Si_Detectors_Grouping.xml
        result = group_spectra_of(self._si_workspace, method="Detectors")
        self.assertGreater(result.getNumberHistograms(), 0)

    def test_raises_when_detectors_grouping_file_parameter_is_missing(self):
        with self.assertRaisesRegex(RuntimeError, "Workflow.DetectorsGroupingFile"):
            group_spectra_of(self._pg_workspace, method="Detectors")


if __name__ == "__main__":
    unittest.main()
