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
        self.assertEquals(1008, grouping_workspace.getNumberHistograms())

    def test_excluded_detector_ids_returns_the_expected_detector_ids(self):
        grouping_workspace = create_grouping_workspace(self._workspace, "osiris_041_RES10.cal")

        excluded_ids = _excluded_detector_ids(grouping_workspace)

        self.assertEquals(904, len(excluded_ids))
        self.assertEquals([i for i in range(16, 116)], excluded_ids[:100])

    def test_get_x_range_when_bins_vary_returns_the_expected_min_and_max_x(self):
        grouping_workspace = create_grouping_workspace(self._workspace, "osiris_041_RES10.cal")

        x_min, x_max = _get_x_range_when_bins_vary(self._workspace, grouping_workspace)

        self.assertEquals(50100.00, x_min)
        self.assertEquals(70100.00, x_max)


if __name__ == "__main__":
    unittest.main()
