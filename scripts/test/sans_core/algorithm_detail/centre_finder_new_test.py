# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from sans_core.algorithm_detail.centre_finder_new import centre_finder_new, centre_finder_mass
from sans_core.common.enums import SANSDataType, FindDirectionEnum, DetectorType
from sans_core.test_helper.test_director import TestDirector


class CentreFinderNewTest(unittest.TestCase):
    def setUp(self):
        state_builder = TestDirector()
        self.state = state_builder.construct()

    @mock.patch("sans_core.algorithm_detail.centre_finder_new.provide_loaded_data")
    @mock.patch("sans_core.algorithm_detail.centre_finder_new.create_managed_non_child_algorithm")
    def test_that_create_manage_non_child_algorithm_is_called_once_in_centre_finder_new(self, make_algorithm_mock, load_data_mock):
        r_min = 5
        r_max = 10
        position_1_start = 300
        position_2_start = -300
        tolerance = 0.001
        find_direction = FindDirectionEnum.ALL
        iterations = 10
        verbose = False

        load_data_mock.return_value = {SANSDataType.SAMPLE_SCATTER: [mock.MagicMock()]}, {SANSDataType.SAMPLE_SCATTER: [mock.MagicMock()]}

        beam_centre_finder = "SANSBeamCentreFinder"
        beam_centre_finder_options = {
            "Component": "LAB",
            "Iterations": iterations,
            "RMin": r_min / 1000,
            "RMax": r_max / 1000,
            "Position1Start": position_1_start,
            "Position2Start": position_2_start,
            "Tolerance": tolerance,
            "Direction": find_direction.value,
            "Verbose": verbose,
        }

        centre_finder_new(
            self.state,
            r_min=r_min,
            r_max=r_max,
            iterations=iterations,
            position_1_start=position_1_start,
            position_2_start=position_2_start,
            tolerance=tolerance,
            find_direction=find_direction,
            verbose=verbose,
            component=DetectorType.LAB,
        )

        make_algorithm_mock.assert_called_once_with(beam_centre_finder, **beam_centre_finder_options)

    @mock.patch("sans_core.algorithm_detail.centre_finder_new.provide_loaded_data")
    @mock.patch("sans_core.algorithm_detail.centre_finder_new.create_managed_non_child_algorithm")
    def test_that_create_manage_non_child_algorithm_is_called_once_in_centre_finder_mass(self, make_algorithm_mock, load_data_mock):
        r_min = 5
        position_1_start = 300
        position_2_start = -300
        tolerance = 0.001
        iterations = 10

        load_data_mock.return_value = {SANSDataType.SAMPLE_SCATTER: [mock.MagicMock()]}, {SANSDataType.SAMPLE_SCATTER: [mock.MagicMock()]}

        beam_centre_finder = "SANSBeamCentreFinderMassMethod"
        beam_centre_finder_options = {
            "RMin": r_min / 1000,
            "Centre1": position_1_start,
            "Centre2": position_2_start,
            "Component": "LAB",
            "Tolerance": tolerance,
        }

        centre_finder_mass(
            self.state,
            r_min=r_min,
            max_iter=iterations,
            position_1_start=position_1_start,
            position_2_start=position_2_start,
            tolerance=tolerance,
            component=DetectorType.LAB,
        )

        make_algorithm_mock.assert_called_once_with(beam_centre_finder, **beam_centre_finder_options)


if __name__ == "__main__":
    unittest.main()
