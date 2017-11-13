from __future__ import (absolute_import, division, print_function)

import unittest
import sys
from sans.test_helper.mock_objects import create_mock_beam_centre_tab
from sans.gui_logic.models.beam_centre_model import BeamCentreModel
from sans.common.enums import FindDirectionEnum
from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter, find_beam_centre
from sans.test_helper.mock_objects import (create_run_tab_presenter_mock)
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class BeamCentrePresenterTest(unittest.TestCase):

    @mock.patch('sans.gui_logic.presenter.beam_centre_presenter.WorkHandler')
    def test_that_on_run_clicked_calls_find_beam_centre(self, work_handler_mock):
        parent_presenter = create_run_tab_presenter_mock(use_fake_state = False)
        presenter = BeamCentrePresenter(parent_presenter)
        view = create_mock_beam_centre_tab()
        presenter.set_view(view)

        presenter.on_run_clicked()

        self.assertTrue(work_handler_mock.return_value.process.call_count == 1)
        self.assertTrue(work_handler_mock.return_value.process.call_args[0][1] == find_beam_centre)
        self.assertTrue(work_handler_mock.return_value.process.call_args[0][3] == presenter._beam_centre_model)

    def test_that_on_run_clicked_updates_model_from_view(self):
        parent_presenter = create_run_tab_presenter_mock(use_fake_state=False)
        presenter = BeamCentrePresenter(parent_presenter)
        view = create_mock_beam_centre_tab()
        view.left_right = False
        view.lab_pos_1 = 100
        view.lab_pos_2 = -100
        presenter.set_view(view)
        presenter.on_run_clicked()
        self.assertFalse(presenter._beam_centre_model.left_right)
        self.assertEqual(presenter._beam_centre_model.lab_pos_1, 0.1)
        self.assertEqual(presenter._beam_centre_model.lab_pos_2, -0.1)

    def test_that_set_options_is_called_on_update_rows(self):
        parent_presenter = create_run_tab_presenter_mock(use_fake_state=False)
        parent_presenter._file_information = None
        presenter = BeamCentrePresenter(parent_presenter)
        view = create_mock_beam_centre_tab()

        presenter.set_view(view)
        view.set_options.assert_called_once_with(presenter._beam_centre_model)

        presenter.on_update_rows()
        self.assertTrue(view.set_options.call_count == 2)

    def test_that_on_processing_finished_updates_view(self):
        parent_presenter = create_run_tab_presenter_mock(use_fake_state=False)
        presenter = BeamCentrePresenter(parent_presenter)
        view = create_mock_beam_centre_tab()
        presenter.set_view(view)
        result = {'pos1': 0.1, 'pos2': -0.1}

        presenter.on_processing_finished_centre_finder(result)

        self.assertEqual(view.lab_pos_1, presenter._beam_centre_model.scale_1*result['pos1'])
        self.assertEqual(view.lab_pos_2, presenter._beam_centre_model.scale_2*result['pos2'])

    @mock.patch('sans.gui_logic.presenter.beam_centre_presenter.SANSCentreFinder')
    def test_that_find_beam_centre_initiates_centre_finder_with_correct_parameters(self, centre_finder_mock):
        state = mock.MagicMock()
        beam_centre_model = BeamCentreModel()
        beam_centre_model.lab_pos_1 = 0.1
        beam_centre_model.lab_pos_2 = -0.1
        find_direction = FindDirectionEnum.All

        find_beam_centre(state, beam_centre_model)

        centre_finder_mock.return_value.called_once_with(state, r_min=beam_centre_model.r_min,
                                                         r_max=beam_centre_model.r_max,
                                                         max_iter=beam_centre_model.max_iterations,
                                                         x_start=beam_centre_model.lab_pos_1,
                                                         y_start=beam_centre_model.lab_pos_2,
                                                         tolerance=beam_centre_model.tolerance,
                                                         find_direction=find_direction, reduction_method=True)


if __name__ == '__main__':
    unittest.main()
