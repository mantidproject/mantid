# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.Common.corrections_tab_widget.background_corrections_model import BackgroundCorrectionsModel
from mantidqtinterfaces.Muon.GUI.Common.corrections_tab_widget.background_corrections_presenter import BackgroundCorrectionsPresenter
from mantidqtinterfaces.Muon.GUI.Common.corrections_tab_widget.background_corrections_view import BackgroundCorrectionsView


class BackgroundCorrectionsPresenterTest(unittest.TestCase):

    def setUp(self):
        self.workspace_name = "HIFI84447; Group; fwd; Counts; MA"
        self.runs = ["84447", "84447", "84447", "84447"]
        self.groups = ["fwd", "bwd", "top", "bottom"]
        self.selected_run = "84447"
        self.selected_group = "fwd"
        self.start_x = 5.0
        self.end_x = 15.0
        self.background = 2.0

        self._setup_mock_view()
        self._setup_mock_model()
        self._setup_presenter()

    def tearDown(self):
        self.presenter = None
        self.model = None
        self.view = None

    def test_that_the_slots_are_set_in_the_view(self):
        self.assertEqual(self.view.set_slot_for_mode_combo_box_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_select_function_combo_box_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_group_combo_box_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_show_all_runs.call_count, 1)
        self.assertEqual(self.view.set_slot_for_start_x_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_end_x_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_background_changed.call_count, 1)

    def test_that_handle_instrument_changed_will_reset_the_background_correction_mode(self):
        self.presenter.handle_instrument_changed()

        self.model.set_background_correction_mode.assert_called_once_with("None")
        self.model.set_selected_function.assert_called_once_with("Flat Background + Exp Decay")
        self.mock_view_background_correction_mode.assert_called_once_with("None")
        self.mock_view_selected_function.assert_called_once_with("Flat Background + Exp Decay")

    def test_that_handle_pre_process_and_counts_calculated_will_populate_the_background_corrections_data(self):
        self.presenter.handle_pre_process_and_counts_calculated()

        self.model.populate_background_corrections_data.assert_called_once_with()
        self.presenter._run_background_corrections_for_all.assert_called_once_with()

    def test_that_handle_groups_changed_will_populate_the_group_selector(self):
        self.presenter.handle_groups_changed()

        self.model.group_names.assert_called_once_with()
        self.view.populate_group_selector.assert_called_once_with(self.groups)

    def test_that_handle_run_selector_changed_will_attempt_to_update_the_displayed_corrections_data(self):
        self.presenter.handle_run_selector_changed()
        self.presenter._update_displayed_corrections_data.assert_called_once_with()

    def test_that_handle_mode_combo_box_changed_will_update_the_model_and_view_when_in_none_mode(self):
        self.assertEqual(self.view.background_correction_mode, "None")
        self.assertTrue(self.model.is_background_mode_none())

        self.presenter.handle_mode_combo_box_changed()

        self.mock_view_background_correction_mode.assert_called_with()
        self.model.set_background_correction_mode.assert_called_once_with("None")
        self.model.is_background_mode_none.assert_called_with()
        self.view.set_none_background_correction_options_visible.assert_called_once_with()

    def test_that_handle_mode_combo_box_changed_will_update_the_model_and_view_when_in_auto_mode(self):
        self.mock_view_background_correction_mode = mock.PropertyMock(return_value="Auto")
        type(self.view).background_correction_mode = self.mock_view_background_correction_mode
        self.model.is_background_mode_none = mock.Mock(return_value=False)
        self.model.is_background_mode_auto = mock.Mock(return_value=True)

        self.assertEqual(self.view.background_correction_mode, "Auto")
        self.assertTrue(self.model.is_background_mode_auto())

        self.presenter.handle_mode_combo_box_changed()

        self.mock_view_background_correction_mode.assert_called_with()
        self.model.set_background_correction_mode.assert_called_once_with("Auto")
        self.model.is_background_mode_none.assert_called_with()
        self.model.is_background_mode_auto.assert_called_with()
        self.view.set_auto_background_correction_options_visible.assert_called_once_with()

    def test_that_handle_mode_combo_box_changed_will_update_the_model_and_view_when_in_manual_mode(self):
        self.mock_view_background_correction_mode = mock.PropertyMock(return_value="Manual")
        type(self.view).background_correction_mode = self.mock_view_background_correction_mode
        self.model.is_background_mode_none = mock.Mock(return_value=False)
        self.model.is_background_mode_manual = mock.Mock(return_value=True)

        self.assertEqual(self.view.background_correction_mode, "Manual")
        self.assertTrue(self.model.is_background_mode_manual())

        self.presenter.handle_mode_combo_box_changed()

        self.mock_view_background_correction_mode.assert_called_with()
        self.model.set_background_correction_mode.assert_called_once_with("Manual")
        self.model.is_background_mode_none.assert_called_with()
        self.model.is_background_mode_auto.assert_called_with()
        self.model.is_background_mode_manual.assert_called_with()
        self.view.set_manual_background_correction_options_visible.assert_called_once_with()

    def test_that_handle_select_function_combo_box_changed_will_update_the_model(self):
        self.assertEqual(self.view.selected_function, "Flat Background")

        self.presenter.handle_select_function_combo_box_changed()

        self.mock_view_selected_function.assert_called_with()
        self.model.set_selected_function.assert_called_once_with("Flat Background")

    def test_that_handle_selected_group_changed_will_update_the_model(self):
        self.presenter.handle_selected_group_changed()

        self.mock_view_selected_group.assert_called_once_with()
        self.model.set_selected_group.assert_called_once_with("All")

        self.presenter._update_displayed_corrections_data.assert_called_once_with()

    def test_that_handle_show_all_runs_ticked_will_update_the_model(self):
        self.presenter.handle_show_all_runs_ticked()

        self.mock_view_show_all_runs.assert_called_once_with()
        self.model.set_show_all_runs.assert_called_once_with(False)

        self.presenter._update_displayed_corrections_data.assert_called_once_with()

    def test_that_handle_start_x_changed_will_validate_the_start_x_before_updating_the_xs_in_the_model(self):
        self.presenter.handle_start_x_changed()
        self.presenter._handle_start_or_end_x_changed.assert_called_once_with(
            self.presenter._get_new_x_range_when_start_x_changed)

    def test_that_handle_end_x_changed_will_validate_the_end_x_before_updating_the_xs_in_the_model(self):
        self.presenter.handle_end_x_changed()
        self.presenter._handle_start_or_end_x_changed.assert_called_once_with(
            self.presenter._get_new_x_range_when_end_x_changed)

    def test_that_handle_background_changed_will_update_the_background_in_the_view_and_model(self):
        runs, groups = ["62260", "62260"], ["fwd", "bwd"]

        self.presenter._selected_runs_and_groups = mock.Mock(return_value=(runs, groups))
        self.presenter._perform_background_corrections_for = mock.Mock()

        self.presenter.handle_background_changed()

        self.view.selected_background.assert_called_once_with()

        self.presenter._update_background_in_view_and_model.assert_has_calls(
            [mock.call("62260", "fwd", self.background), mock.call("62260", "bwd", self.background)])

        self.presenter._perform_background_corrections_for.assert_called_with(runs, groups)

    def _setup_mock_view(self):
        self.view = mock.Mock(spec=BackgroundCorrectionsView)

        self.view.set_slot_for_mode_combo_box_changed = mock.Mock()
        self.view.set_slot_for_select_function_combo_box_changed = mock.Mock()
        self.view.set_slot_for_group_combo_box_changed = mock.Mock()
        self.view.set_slot_for_show_all_runs = mock.Mock()
        self.view.set_slot_for_start_x_changed = mock.Mock()
        self.view.set_slot_for_end_x_changed = mock.Mock()
        self.view.set_slot_for_background_changed = mock.Mock()

        self.view.set_background_correction_options_visible = mock.Mock()
        self.view.populate_group_selector = mock.Mock()
        self.view.set_start_x = mock.Mock()
        self.view.set_end_x = mock.Mock()
        self.view.populate_corrections_table = mock.Mock()

        self.view.selected_run_and_group = mock.Mock(return_value=tuple([self.selected_run, self.selected_group]))
        self.view.selected_background = mock.Mock(return_value=self.background)
        self.view.start_x = mock.Mock(return_value=self.start_x)
        self.view.end_x = mock.Mock(return_value=self.end_x)

        # Mock the properties of the view
        self.mock_view_background_correction_mode = mock.PropertyMock(return_value="None")
        type(self.view).background_correction_mode = self.mock_view_background_correction_mode

        self.mock_view_selected_function = mock.PropertyMock(return_value="Flat Background")
        type(self.view).selected_function = self.mock_view_selected_function

        self.mock_view_selected_group = mock.PropertyMock(return_value="All")
        type(self.view).selected_group = self.mock_view_selected_group

        self.mock_view_show_all_runs = mock.PropertyMock(return_value=False)
        type(self.view).show_all_runs = self.mock_view_show_all_runs

    def _setup_mock_model(self):
        self.model = mock.Mock(spec=BackgroundCorrectionsModel)

        self.model.set_background_correction_mode = mock.Mock()
        self.model.set_selected_function = mock.Mock()
        self.model.set_selected_group = mock.Mock()
        self.model.set_show_all_runs = mock.Mock()
        self.model.set_start_x = mock.Mock()
        self.model.set_end_x = mock.Mock()
        self.model.group_names = mock.Mock(return_value=self.groups)
        self.model.is_background_mode_none = mock.Mock(return_value=True)
        self.model.is_background_mode_auto = mock.Mock(return_value=False)
        self.model.is_background_mode_manual = mock.Mock(return_value=False)
        self.model.get_counts_workspace_name = mock.Mock(return_value=self.workspace_name)

        self.model.populate_background_corrections_data = mock.Mock()

    def _setup_presenter(self):
        self.corrections_presenter = mock.Mock()
        self.corrections_presenter.warning_popup = mock.Mock()

        self.presenter = BackgroundCorrectionsPresenter(self.view, self.model, self.corrections_presenter)

        self.presenter._update_displayed_corrections_data = mock.Mock()
        self.presenter._update_background_in_view_and_model = mock.Mock()
        self.presenter._handle_start_or_end_x_changed = mock.Mock()
        self.presenter._run_background_corrections_for_all = mock.Mock()


if __name__ == '__main__':
    unittest.main()
