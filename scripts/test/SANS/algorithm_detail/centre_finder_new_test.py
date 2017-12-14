from __future__ import (absolute_import, division, print_function)

import unittest
import sys
from sans.algorith_detail.centre_finder_new import centre_finder_new, centre_finder_mass
from sans.common.enums import (SANSDataType, FindDirectionEnum)

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class CentreFinderNewTest(unittest.TestCase):
    def setUp(self):
        self.state = mock.MagicMock()

    @mock.patch('sans.algorithm_detail.centre_finder_new.create_managed_non_child_algorithm')
    def test_that_create_manage_non_child_algorithm_is_called_once_in_centre_finder_new(self, make_algorithm_mock):
        r_min = 5
        r_max = 10
        position_1_start = 300
        position_2_start = -300
        tolerance = 0.001
        find_direction = FindDirectionEnum.All
        iterations = 10
        verbose = False

        beam_centre_finder = "SANSBeamCentreFinder"
        beam_centre_finder_options = {"Iterations": iterations, "RMin": r_min / 1000, "RMax": r_max / 1000,
                                      "Position1Start": position_1_start, "Position2Start": position_2_start,
                                      "Tolerance": tolerance, "Direction": FindDirectionEnum.to_string(find_direction),
                                      "Verbose": verbose}

        centre_finder_new(self.state, r_min=r_min, r_max=r_max, iterations=iterations, position_1_start=position_1_start
                          ,position_2_start=position_2_start, tolerance=tolerance, find_direction=find_direction
                          ,verbose=verbose)

        make_algorithm_mock.assert_called_once_with(beam_centre_finder, **beam_centre_finder_options)

    @mock.patch('sans.algorithm_detail.centre_finder_new.create_managed_non_child_algorithm')
    def test_that_create_manage_non_child_algorithm_is_called_once_in_centre_finder_mass(self, make_algorithm_mock):
        r_min = 5
        position_1_start = 300
        position_2_start = -300
        tolerance = 0.001
        iterations = 10

        beam_centre_finder = "SANSBeamCentreFinderMassMethod"
        beam_centre_finder_options = {"Iterations": iterations, "RMin": r_min / 1000,
                                      "Position1Start": position_1_start, "Position2Start": position_2_start,
                                      "Tolerance": tolerance}

        centre_finder_mass(self.state, r_min=r_min, iterations=iterations, position_1_start=position_1_start
                           , position_2_start=position_2_start, tolerance=tolerance)

        make_algorithm_mock.assert_called_once_with(beam_centre_finder, **beam_centre_finder_options)

if __name__ == '__main__':
    unittest.main()