# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from sans.common.enums import FindDirectionEnum, SANSInstrument, DetectorType
from sans.gui_logic.models.beam_centre_model import BeamCentreModel


class BeamCentreModelTest(unittest.TestCase):
    def setUp(self):
        self.result = {'pos1':300, 'pos2':-300}
        self.centre_finder_instance = mock.MagicMock(return_value = self.result)
        self.SANSCentreFinder = mock.MagicMock(return_value = self.centre_finder_instance)
        self.beam_centre_model = BeamCentreModel(self.SANSCentreFinder)

    def test_that_model_initialises_with_correct_values(self):
        self.assertEqual(self.beam_centre_model.max_iterations, 10)
        self.assertEqual(self.beam_centre_model.r_min, 0)
        self.assertEqual(self.beam_centre_model.r_max, 0)
        self.assertEqual(self.beam_centre_model.left_right, True)
        self.assertEqual(self.beam_centre_model.up_down, True)
        self.assertEqual(self.beam_centre_model.tolerance, 0.0001251)
        self.assertEqual(self.beam_centre_model.lab_pos_1, '')
        self.assertEqual(self.beam_centre_model.lab_pos_2, '')
        self.assertEqual(self.beam_centre_model.hab_pos_2, '')
        self.assertEqual(self.beam_centre_model.hab_pos_1, '')
        self.assertEqual(self.beam_centre_model.scale_1, 1000)
        self.assertEqual(self.beam_centre_model.scale_2, 1000)
        self.assertEqual(self.beam_centre_model.COM, False)
        self.assertEqual(self.beam_centre_model.verbose, False)
        self.assertEqual(self.beam_centre_model.q_min, 0.01)
        self.assertEqual(self.beam_centre_model.q_max, 0.1)
        self.assertEqual(self.beam_centre_model.component, DetectorType.LAB)
        self.assertTrue(self.beam_centre_model.update_lab)
        self.assertTrue(self.beam_centre_model.update_hab)

    def test_all_other_hardcoded_inst_values_taken(self):
        for inst in {SANSInstrument.NO_INSTRUMENT, SANSInstrument.SANS2D, SANSInstrument.ZOOM}:
            self.beam_centre_model.reset_inst_defaults(instrument=inst, row_entry=mock.Mock())
            self.assertEqual(60, self.beam_centre_model.r_min)
            self.assertEqual(280, self.beam_centre_model.r_max)
            self.assertEqual(1000, self.beam_centre_model.scale_1)

    def test_that_can_update_model_values(self):
        self.beam_centre_model.scale_2 = 1.0

        self.assertEqual(self.beam_centre_model.scale_2, 1.0)

    def test_that_correct_values_are_set_for_LARMOR(self):
        row_entry_mock = mock.Mock()

        self.beam_centre_model.reset_inst_defaults(instrument=SANSInstrument.LARMOR,
                                                   row_entry=row_entry_mock)

        self.assertEqual(self.beam_centre_model.scale_1, 1.0)

    @mock.patch("sans.gui_logic.models.beam_centre_model.get_instrument_paths_for_sans_file")
    @mock.patch("sans.gui_logic.models.beam_centre_model.get_named_elements_from_ipf_file")
    def test_that_correct_values_are_set_for_LOQ(self, mock_get_named_elements, mock_get_paths):
        expected_values = {"beam_centre_radius_min": 1,
                           "beam_centre_radius_max": 2}
        mock_get_named_elements.return_value = expected_values
        mock_get_paths.return_value = [None, None]

        self.beam_centre_model.reset_inst_defaults(instrument=SANSInstrument.LOQ,
                                                   row_entry=mock.Mock())

        self.assertEqual(1, self.beam_centre_model.r_min)
        self.assertEqual(2, self.beam_centre_model.r_max)
        self.assertEqual(1000, self.beam_centre_model.scale_1)

    def test_that_find_beam_centre_calls_centre_finder_once_when_COM_is_False(self):
        state = mock.MagicMock()

        self.beam_centre_model.find_beam_centre(state)

        self.SANSCentreFinder.return_value.assert_called_once_with(state, r_min=self.beam_centre_model.r_min,
                                                                   r_max=self.beam_centre_model.r_max,
                                                                   max_iter= self.beam_centre_model.max_iterations,
                                                                   x_start=self.beam_centre_model.lab_pos_1,
                                                                   y_start=self.beam_centre_model.lab_pos_2,
                                                                   tolerance=self.beam_centre_model.tolerance,
                                                                   find_direction=FindDirectionEnum.ALL,
                                                                   reduction_method=True,
                                                                   verbose=False, component=DetectorType.LAB)

    def test_that_find_beam_centre_calls_centre_finder_twice_when_COM_is_TRUE(self):
        state = mock.MagicMock()
        self.beam_centre_model.COM = True

        self.beam_centre_model.find_beam_centre(state)

        self.assertEqual(self.SANSCentreFinder.return_value.call_count, 2)

        self.SANSCentreFinder.return_value.assert_called_with(state, r_min=self.beam_centre_model.r_min,
                                                              r_max=self.beam_centre_model.r_max,
                                                              max_iter= self.beam_centre_model.max_iterations,
                                                              x_start=self.result['pos1'],
                                                              y_start=self.result['pos2'],
                                                              tolerance=self.beam_centre_model.tolerance,
                                                              find_direction=FindDirectionEnum.ALL,
                                                              reduction_method=True,
                                                              verbose=False, component=DetectorType.LAB)

        self.SANSCentreFinder.return_value.assert_any_call(state, r_min=self.beam_centre_model.r_min,
                                                           r_max=self.beam_centre_model.r_max,
                                                           max_iter=self.beam_centre_model.max_iterations,
                                                           x_start=self.beam_centre_model.lab_pos_1,
                                                           y_start=self.beam_centre_model.lab_pos_2,
                                                           tolerance=self.beam_centre_model.tolerance,
                                                           find_direction=FindDirectionEnum.ALL,
                                                           reduction_method=False, component=DetectorType.LAB)


if __name__ == '__main__':
    unittest.main()
