from __future__ import (absolute_import, division, print_function)
import unittest
import sys
from sans.test_helper.mock_objects import create_mock_diagnostics_tab
from sans.gui_logic.models.beam_centre_model import BeamCentreModel
from sans.common.enums import FindDirectionEnum,SANSInstrument, DetectorType
from sans.gui_logic.presenter.diagnostic_presenter import DiagnosticsPagePresenter
from sans.test_helper.mock_objects import (create_run_tab_presenter_mock)
from sans.common.enums import IntegralEnum
from sans.gui_logic.models.table_model import TableModel, TableIndexModel
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

class DiagnosticsPagePresenterTest(unittest.TestCase):
    def setUp(self):
        self.parent_presenter = create_run_tab_presenter_mock(use_fake_state = False)
        self.view = create_mock_diagnostics_tab()
        self.state_creator = mock.MagicMock()
        self.GuiStateDirector = mock.MagicMock(return_value=self.state_creator)
        self.WorkHandler = mock.MagicMock()
        self.run_integral = mock.MagicMock()
        self.presenter = DiagnosticsPagePresenter(self.parent_presenter, self.WorkHandler, self.run_integral, self.GuiStateDirector)
        self.presenter.set_view(self.view, SANSInstrument.LARMOR)

    def test_that_on_horizontal_clicked_calls_work_handler_with_correct_parameters(self):
        self.presenter.on_horizontal_clicked()

        self.assertEqual(self.presenter._work_handler.process.call_count, 1)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][1], self.run_integral)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][2], self.presenter._view.horizontal_range)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][3], self.presenter._view.horizontal_mask)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][4], IntegralEnum.Horizontal)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][5], DetectorType.LAB)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][6], self.state_creator.create_state(0))

    def test_that_on_vertical_clicked_calls_work_handler_with_correct_parameters(self):
        self.presenter.on_vertical_clicked()

        self.assertEqual(self.presenter._work_handler.process.call_count, 1)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][1], self.run_integral)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][2], self.presenter._view.vertical_range)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][3], self.presenter._view.vertical_mask)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][4], IntegralEnum.Vertical)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][5], DetectorType.LAB)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][6], self.state_creator.create_state(0))

    def test_that_on_time_clicked_calls_work_handler_with_correct_parameters(self):
        self.presenter.on_time_clicked()

        self.assertEqual(self.presenter._work_handler.process.call_count, 1)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][1], self.run_integral)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][2], self.presenter._view.time_range)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][3], self.presenter._view.time_mask)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][4], IntegralEnum.Time)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][5], DetectorType.LAB)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][6], self.state_creator.create_state(0))

    def test_that_on_user_file_load_sets_user_file_name_on_view(self):
        user_file_name = 'user_file_name'
        self.presenter.on_user_file_load(user_file_name)

        self.assertEqual(self.presenter._view.user_file_name, user_file_name)

    def test_that_set_instrument_settings_calls_set_detectors_on_view(self):
        self.presenter.set_instrument_settings(SANSInstrument.SANS2D)

        self.presenter._view.set_detectors.assert_called_with(['rear', 'front'])
        self.assertEqual(self.presenter._view.set_detectors.call_count, 2)

    def test_that_when_create_state_GuiStateDirector_is_called_with_correct_parameters(self):
        file = 'filename'
        period = 0
        table_row = TableIndexModel(0, file, period, '', '', '', '', '', '', '', '', '', '')
        table = TableModel()
        table.add_table_entry(0, table_row)
        state_model_with_view_update = self.presenter._parent_presenter._get_state_model_with_view_update()

        self.presenter.create_state(file, period)

        self.GuiStateDirector.assert_called_once_with(table, state_model_with_view_update,
                                                      self.presenter._parent_presenter._facility)


if __name__ == '__main__':
    unittest.main()