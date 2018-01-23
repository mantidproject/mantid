from __future__ import (absolute_import, division, print_function)

import unittest
import sys
from sans.test_helper.mock_objects import create_mock_beam_centre_tab
from sans.gui_logic.models.beam_centre_model import BeamCentreModel
from sans.common.enums import FindDirectionEnum, SANSInstrument
from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter, find_beam_centre
from sans.test_helper.mock_objects import (create_run_tab_presenter_mock)
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class BeamCentreModelTest(unittest.TestCase):

    def test_that_model_initialises_with_correct_values(self):
        beam_centre_model = BeamCentreModel()

        self.assertEqual(beam_centre_model.max_iterations, 10)
        self.assertEqual(beam_centre_model.r_min, 60)
        self.assertEqual(beam_centre_model.r_max, 280)
        self.assertEqual(beam_centre_model.left_right, True)
        self.assertEqual(beam_centre_model.up_down, True)
        self.assertEqual(beam_centre_model.tolerance, 0.000125)
        self.assertEqual(beam_centre_model.lab_pos_1, '')
        self.assertEqual(beam_centre_model.lab_pos_2, '')
        self.assertEqual(beam_centre_model.hab_pos_2, '')
        self.assertEqual(beam_centre_model.hab_pos_1, '')
        self.assertEqual(beam_centre_model.scale_1, 1000)
        self.assertEqual(beam_centre_model.scale_2, 1000)


    def test_that_can_update_model_values(self):
        beam_centre_model = BeamCentreModel()
        beam_centre_model.scale_2 = 1.0

        self.assertEqual(beam_centre_model.scale_2, 1.0)

    def test_that_correct_values_are_set_for_LARMOR(self):
        beam_centre_model = BeamCentreModel()
        beam_centre_model.reset_to_defaults_for_instrument(SANSInstrument.LARMOR)

        self.assertEqual(beam_centre_model.scale_1, 1.0)

    def test_that_correct_values_are_set_for_LOQ(self):
        beam_centre_model = BeamCentreModel()
        beam_centre_model.reset_to_defaults_for_instrument(SANSInstrument.LOQ)

        self.assertEqual(beam_centre_model.r_max, 200)


if __name__ == '__main__':
    unittest.main()