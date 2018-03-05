from __future__ import (absolute_import, division, print_function)
import unittest
import sys
from sans.test_helper.mock_objects import create_mock_beam_centre_tab
from sans.common.enums import SANSInstrument
from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter
from sans.test_helper.mock_objects import (create_run_tab_presenter_mock)
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class BeamCentrePresenterTest(unittest.TestCase):

    def setUp(self):
        self.parent_presenter = create_run_tab_presenter_mock(use_fake_state = False)
        self.parent_presenter._file_information = mock.MagicMock()
        self.parent_presenter._file_information.get_instrument = mock.MagicMock(return_value = SANSInstrument.LARMOR)
        self.view = create_mock_beam_centre_tab()
        self.WorkHandler = mock.MagicMock()
        self.BeamCentreModel = mock.MagicMock()
        self.SANSCentreFinder = mock.MagicMock()
        self.presenter = BeamCentrePresenter(self.parent_presenter, self.WorkHandler, self.BeamCentreModel,
                                             self.SANSCentreFinder)

    def test_that_on_run_clicked_calls_find_beam_centre(self):
        self.presenter.set_view(self.view)
        self.presenter.on_run_clicked()

        self.assertEqual(self.presenter._work_handler.process.call_count, 1)
        self.assertTrue(self.presenter._work_handler.process.call_args[0][1] == self.presenter._beam_centre_model.find_beam_centre)

    def test_that_on_run_clicked_updates_model_from_view(self):
        self.view.left_right = False
        self.view.lab_pos_1 = 100
        self.view.lab_pos_2 = -100
        self.presenter.set_view(self.view)
        self.presenter._beam_centre_model.scale_1 = 1000
        self.presenter._beam_centre_model.scale_2 = 1000

        self.presenter.on_run_clicked()

        self.assertFalse(self.presenter._beam_centre_model.left_right)
        self.assertEqual(self.presenter._beam_centre_model.lab_pos_1, 0.1)
        self.assertEqual(self.presenter._beam_centre_model.lab_pos_2, -0.1)

    def test_that_set_options_is_called_on_update_rows(self):
        self.presenter.set_view(self.view)

        self.view.set_options.assert_called_once_with(self.presenter._beam_centre_model)

        self.presenter.on_update_rows()
        self.assertTrue(self.view.set_options.call_count == 2)

    def test_that_set_scaling_is_called_on_update_instrument(self):
        self.presenter.set_view(self.view)

        self.presenter.on_update_instrument(SANSInstrument.LARMOR)
        self.presenter._beam_centre_model.set_scaling.assert_called_once_with(SANSInstrument.LARMOR)

    def test_that_on_processing_finished_updates_view_and_model(self):
        self.presenter.set_view(self.view)
        result = {'pos1': 0.1, 'pos2': -0.1}
        self.presenter._beam_centre_model.scale_1 = 1000
        self.presenter._beam_centre_model.scale_2 = 1000

        self.presenter.on_processing_finished_centre_finder(result)
        self.assertEqual(result['pos1'], self.presenter._beam_centre_model.lab_pos_1)
        self.assertEqual(result['pos2'], self.presenter._beam_centre_model.lab_pos_2)
        self.assertEqual(self.view.lab_pos_1, self.presenter._beam_centre_model.scale_1*result['pos1'])
        self.assertEqual(self.view.lab_pos_2, self.presenter._beam_centre_model.scale_2*result['pos2'])
        self.view.set_run_button_to_normal.assert_called_once_with()


if __name__ == '__main__':
    unittest.main()
