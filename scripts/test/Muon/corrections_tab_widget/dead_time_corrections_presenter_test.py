# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_model import DeadTimeCorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_presenter import DeadTimeCorrectionsPresenter
from Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_view import DeadTimeCorrectionsView


class DeadTimeCorrectionsPresenterTest(unittest.TestCase):

    def setUp(self):
        self.dead_time_workspace_name = "HIFI84447 dead time"
        self.dead_time_range = (1.001, 3.003)
        self.dead_time_average = 2.002

        self._setup_mock_view()
        self._setup_mock_model()
        self._setup_presenter()

    def tearDown(self):
        self.presenter = None
        self.model = None
        self.view = None

    def test_that_the_slots_are_set_in_the_view(self):
        self.assertEqual(self.view.set_slot_for_dead_time_from_selector_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_dead_time_workspace_selector_changed.call_count, 1)
        self.assertEqual(self.view.set_slot_for_dead_time_file_browse_clicked.call_count, 1)

    def test_that_handle_ads_clear_or_remove_workspace_event_will_set_dead_time_source_in_the_view_to_from_data_file(self):
        self.model.is_dead_time_source_from_data_file = mock.Mock(return_value=True)

        self.presenter.handle_ads_clear_or_remove_workspace_event()

        self.model.is_dead_time_source_from_data_file.assert_called_once_with()
        self.view.set_dead_time_from_data_file_selected.assert_called_once_with()

    def test_that_handle_ads_clear_or_remove_workspace_event_will_set_dead_time_source_in_the_view_to_from_workspace(self):
        self.model.is_dead_time_source_from_data_file = mock.Mock(return_value=False)
        self.model.is_dead_time_source_from_workspace = mock.Mock(return_value=True)

        self.presenter.handle_ads_clear_or_remove_workspace_event()

        self.model.is_dead_time_source_from_data_file.assert_called_once_with()
        self.model.is_dead_time_source_from_workspace.assert_called_once_with()
        self.view.set_dead_time_from_workspace_selected.assert_called_once_with()

    def test_that_handle_instrument_changed_will_reset_the_dead_time_source_and_attempt_a_recalculation(self):
        self.presenter.handle_instrument_changed()

        self.model.set_dead_time_source_to_from_file.assert_called_with()
        self.presenter._notify_perform_dead_time_corrections.assert_called_once_with()
        self.view.set_dead_time_from_data_file_selected.assert_called_once_with()

    def test_that_handle_run_selector_changed_will_update_the_run_string_in_the_model(self):
        self.presenter.update_dead_time_info_text_in_view = mock.Mock()

        self.presenter.handle_run_selector_changed()

        self.presenter.update_dead_time_info_text_in_view.assert_called_once_with()

    def test_that_handle_dead_time_from_selector_changed_will_hide_the_relevant_widgets_for_data_file_mode(self):
        self.view.is_dead_time_from_data_file_selected = mock.Mock(return_value=True)

        self.presenter.handle_dead_time_from_selector_changed()

        self.view.is_dead_time_from_data_file_selected.assert_called_once_with()
        self.presenter._handle_dead_time_from_data_file_selected.assert_called_once_with()
        self.view.set_dead_time_workspace_selector_visible.assert_called_once_with(False)
        self.view.set_dead_time_other_file_visible.assert_called_once_with(False)

    def test_that_handle_dead_time_from_selector_changed_will_hide_the_relevant_widgets_for_workspace_mode(self):
        self.view.is_dead_time_from_data_file_selected = mock.Mock(return_value=False)
        self.view.is_dead_time_from_workspace_selected = mock.Mock(return_value=True)

        self.presenter.handle_dead_time_from_selector_changed()

        self.view.is_dead_time_from_data_file_selected.assert_called_once_with()
        self.view.is_dead_time_from_workspace_selected.assert_called_once_with()
        self.presenter._handle_dead_time_from_workspace_selected.assert_called_once_with()
        self.view.set_dead_time_workspace_selector_visible.assert_called_once_with(True)
        self.view.set_dead_time_other_file_visible.assert_called_once_with(False)

    def test_that_handle_dead_time_from_selector_changed_will_hide_the_relevant_widgets_for_other_file_mode(self):
        self.view.is_dead_time_from_data_file_selected = mock.Mock(return_value=False)
        self.view.is_dead_time_from_workspace_selected = mock.Mock(return_value=False)
        self.view.is_dead_time_from_other_file_selected = mock.Mock(return_value=True)

        self.presenter.handle_dead_time_from_selector_changed()

        self.view.is_dead_time_from_data_file_selected.assert_called_once_with()
        self.view.is_dead_time_from_workspace_selected.assert_called_once_with()
        self.view.is_dead_time_from_other_file_selected.assert_called_once_with()
        self.presenter._handle_dead_time_from_none_selected.assert_called_once_with()
        self.view.set_dead_time_workspace_selector_visible.assert_called_once_with(False)
        self.view.set_dead_time_other_file_visible.assert_called_once_with(True)

    def test_that_handle_dead_time_from_selector_changed_will_hide_the_relevant_widgets_for_none_mode(self):
        self.view.is_dead_time_from_data_file_selected = mock.Mock(return_value=False)
        self.view.is_dead_time_from_workspace_selected = mock.Mock(return_value=False)
        self.view.is_dead_time_from_other_file_selected = mock.Mock(return_value=False)

        self.presenter.handle_dead_time_from_selector_changed()

        self.view.is_dead_time_from_data_file_selected.assert_called_once_with()
        self.view.is_dead_time_from_workspace_selected.assert_called_once_with()
        self.view.is_dead_time_from_other_file_selected.assert_called_once_with()
        self.presenter._handle_dead_time_from_none_selected.assert_called_once_with()
        self.view.set_dead_time_workspace_selector_visible.assert_called_once_with(False)
        self.view.set_dead_time_other_file_visible.assert_called_once_with(False)

    def test_that_handle_dead_time_workspace_selector_changed_will_set_none_selected_if_the_file_name_is_empty(self):
        self.view.selected_dead_time_workspace = mock.Mock(return_value="")

        self.presenter.handle_dead_time_workspace_selector_changed()

        self.view.selected_dead_time_workspace.assert_called_once_with()
        self.presenter._handle_dead_time_from_none_selected.assert_called_once_with()

    def test_that_handle_dead_time_workspace_selector_changed_will_set_none_selected_if_the_file_name_is_none(self):
        self.view.selected_dead_time_workspace = mock.Mock(return_value="None")

        self.presenter.handle_dead_time_workspace_selector_changed()

        self.view.selected_dead_time_workspace.assert_called_once_with()
        self.presenter._handle_dead_time_from_none_selected.assert_called_once_with()

    def test_that_handle_dead_time_workspace_selector_changed_will_show_an_error_if_the_table_provided_is_invalid(self):
        error_message = "Error message"
        self.view.selected_dead_time_workspace = mock.Mock(return_value=self.dead_time_workspace_name)
        self.model.validate_selected_dead_time_workspace = mock.Mock(return_value=error_message)

        self.presenter.handle_dead_time_workspace_selector_changed()

        self.view.selected_dead_time_workspace.assert_called_once_with()
        self.model.validate_selected_dead_time_workspace.assert_called_once_with(self.dead_time_workspace_name)
        self.presenter._handle_selected_table_is_invalid.assert_called_once_with()
        self.corrections_presenter.warning_popup.assert_called_once_with(error_message)

    def test_that_handle_dead_time_workspace_selector_changed_will_set_the_dead_time_source_if_the_table_is_valid(self):
        error_message = ""
        self.view.selected_dead_time_workspace = mock.Mock(return_value=self.dead_time_workspace_name)
        self.model.validate_selected_dead_time_workspace = mock.Mock(return_value=error_message)
        self.presenter.set_dead_time_source_to_from_ads = mock.Mock()

        self.presenter.handle_dead_time_workspace_selector_changed()

        self.view.selected_dead_time_workspace.assert_called_once_with()
        self.model.validate_selected_dead_time_workspace.assert_called_once_with(self.dead_time_workspace_name)
        self.presenter.set_dead_time_source_to_from_ads.assert_called_once_with()

    def test_that_handle_dead_time_browse_clicked_will_not_load_a_file_if_the_filename_is_empty(self):
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value=["", ""])
        self.presenter._load_file_containing_dead_time = mock.Mock(return_value=None)

        self.presenter.handle_dead_time_browse_clicked()

        self.view.show_file_browser_and_return_selection.assert_called_once_with(["nxs"], [""], multiple_files=False)
        self.assertEqual(self.presenter._load_file_containing_dead_time.call_count, 0)

    def test_that_handle_dead_time_browse_clicked_will_attempt_to_load_a_file_if_the_filename_is_not_empty(self):
        filename = "HIFI00084447.nxs"
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value=[filename, ""])
        self.presenter._load_file_containing_dead_time = mock.Mock(return_value=None)

        self.presenter.handle_dead_time_browse_clicked()

        self.view.show_file_browser_and_return_selection.assert_called_once_with(["nxs"], [""], multiple_files=False)
        self.presenter._load_file_containing_dead_time.assert_called_once_with(filename)
        self.assertEqual(self.view.populate_dead_time_workspace_selector.call_count, 0)
        self.assertEqual(self.view.switch_to_using_a_dead_time_table_workspace.call_count, 0)

    def test_that_handle_dead_time_browse_clicked_will_populate_the_workspace_selector_if_a_file_loads_successfully(self):
        filename = "HIFI00084447.nxs"
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value=[filename, ""])
        self.model.validate_selected_dead_time_workspace = mock.Mock(return_value="")
        self.presenter._load_file_containing_dead_time = mock.Mock(return_value=filename)

        self.presenter.handle_dead_time_browse_clicked()

        self.view.show_file_browser_and_return_selection.assert_called_once_with(["nxs"], [""], multiple_files=False)
        self.presenter._load_file_containing_dead_time.assert_called_once_with(filename)
        self.assertEqual(self.view.populate_dead_time_workspace_selector.call_count, 1)
        self.view.switch_to_using_a_dead_time_table_workspace.assert_called_once_with(filename)

    def test_that_handle_pre_process_and_counts_calculated_will_update_the_dead_time_label_in_the_view(self):
        self.presenter.update_dead_time_info_text_in_view = mock.Mock()

        self.presenter.handle_pre_process_and_counts_calculated()

        self.presenter.update_dead_time_info_text_in_view.assert_called_once_with()

    def test_that_update_dead_time_info_text_in_view_will_set_dead_time_averages_if_in_file_or_workspace_mode(self):
        self.model.is_dead_time_source_from_data_file = mock.Mock(return_value=False)
        self.model.is_dead_time_source_from_workspace = mock.Mock(return_value=True)

        self.presenter.update_dead_time_info_text_in_view()

        self.model.dead_times_average.assert_called_once_with()
        self.model.dead_times_range.assert_called_once_with()
        self.view.set_dead_time_average_and_range(self.dead_time_range, self.dead_time_average)

    def test_that_update_dead_time_info_text_in_view_will_set_dead_time_info_text_to_none_when_in_the_relevant_mode(self):
        self.model.is_dead_time_source_from_data_file = mock.Mock(return_value=False)
        self.model.is_dead_time_source_from_workspace = mock.Mock(return_value=False)

        self.presenter.update_dead_time_info_text_in_view()

        self.view.set_dead_time_info_text("No dead time correction")

    def test_that_set_dead_time_source_to_from_file_notifies_that_a_recalculation_is_needed(self):
        self.presenter.set_dead_time_source_to_from_file()

        self.model.set_dead_time_source_to_from_file.assert_called_with()
        self.presenter._notify_perform_dead_time_corrections.assert_called_once_with()

    def test_that_set_dead_time_source_to_from_ads_notifies_that_a_recalculation_is_needed(self):
        self.view.selected_dead_time_workspace = mock.Mock(return_value=self.dead_time_workspace_name)
        self.presenter.set_dead_time_source_to_from_ads()

        self.view.selected_dead_time_workspace.assert_called_once_with()
        self.model.set_dead_time_source_to_from_ads.assert_called_once_with(self.dead_time_workspace_name)
        self.presenter._notify_perform_dead_time_corrections.assert_called_once_with()

    def test_that_set_dead_time_source_to_none_notifies_that_a_recalculation_is_needed(self):
        self.presenter.set_dead_time_source_to_none()

        self.model.set_dead_time_source_to_none.assert_called_once_with()
        self.presenter._notify_perform_dead_time_corrections.assert_called_once_with()

    def _setup_mock_view(self):
        self.view = mock.Mock(spec=DeadTimeCorrectionsView)

        self.view.set_slot_for_dead_time_from_selector_changed = mock.Mock()
        self.view.set_slot_for_dead_time_workspace_selector_changed = mock.Mock()
        self.view.set_slot_for_dead_time_file_browse_clicked = mock.Mock()

        self.view.set_dead_time_from_data_file_selected = mock.Mock()
        self.view.set_dead_time_from_workspace_selected = mock.Mock()
        self.view.set_dead_time_workspace_selector_visible = mock.Mock()
        self.view.set_dead_time_other_file_visible = mock.Mock()
        self.view.set_dead_time_average_and_range = mock.Mock()
        self.view.set_dead_time_info_text = mock.Mock()

        self.view.populate_dead_time_workspace_selector = mock.Mock()
        self.view.switch_to_using_a_dead_time_table_workspace = mock.Mock()

    def _setup_mock_model(self):
        self.model = mock.Mock(spec=DeadTimeCorrectionsModel)

        self.model.set_dead_time_source_to_from_file = mock.Mock()
        self.model.dead_times_range = mock.Mock(return_value=self.dead_time_range)
        self.model.dead_times_average = mock.Mock(return_value=self.dead_time_average)

    def _setup_presenter(self):
        self.corrections_presenter = mock.Mock()
        self.corrections_presenter.warning_popup = mock.Mock()

        self.presenter = DeadTimeCorrectionsPresenter(self.view, self.model, self.corrections_presenter)

        self.presenter._handle_dead_time_from_data_file_selected = mock.Mock()
        self.presenter._handle_dead_time_from_workspace_selected = mock.Mock()
        self.presenter._handle_dead_time_from_none_selected = mock.Mock()

        self.presenter._handle_selected_table_is_invalid = mock.Mock()

        self.presenter._notify_perform_dead_time_corrections = mock.Mock()


if __name__ == '__main__':
    unittest.main()
