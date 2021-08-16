# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.corrections_presenter import CorrectionsPresenter
from Muon.GUI.Common.corrections_tab_widget.corrections_view import CorrectionsView
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.Common.thread_model import ThreadModel


class CorrectionsPresenterTest(unittest.TestCase):

    def setUp(self):
        self.current_run_string = "84447"
        self.run_strings = ["84447", "84448", "84449"]
        self.groups = ["fwd", "bwd", "bottom", "top"]

        self._setup_mock_view()
        self._setup_mock_model()
        self._setup_presenter()

    def tearDown(self):
        self.presenter = None
        self.model = None
        self.view = None

    def test_that_the_slots_are_set_in_the_view(self):
        self.assertEqual(self.view.set_slot_for_run_selector_changed.call_count, 1)

    def test_that_handle_ads_clear_or_remove_workspace_event_will_set_dead_time_source_in_the_view_to_from_data_file(self):
        self.presenter.handle_runs_loaded = mock.Mock()

        self.presenter.handle_ads_clear_or_remove_workspace_event()

        self.presenter.dead_time_presenter.handle_ads_clear_or_remove_workspace_event.assert_called_once_with()
        self.presenter.handle_runs_loaded.assert_called_once_with()

    def test_that_handle_ads_clear_or_remove_workspace_event_will_set_dead_time_source_in_the_view_to_from_workspace(self):
        self.presenter.handle_runs_loaded = mock.Mock()

        self.presenter.handle_ads_clear_or_remove_workspace_event()

        self.presenter.dead_time_presenter.handle_ads_clear_or_remove_workspace_event.assert_called_once_with()
        self.presenter.handle_runs_loaded.assert_called_once_with()

    def test_that_handle_instrument_changed_will_reset_the_dead_time_source_and_attempt_a_recalculation(self):
        self.presenter.handle_instrument_changed()
        self.presenter.dead_time_presenter.handle_instrument_changed.assert_called_once_with()
        self.presenter.background_presenter.handle_instrument_changed.assert_called_once_with()

    def test_that_handle_runs_loaded_will_update_the_runs_in_the_model_and_view(self):
        self.model.run_number_strings = mock.Mock(return_value=self.run_strings)
        self.view.current_run_string = mock.Mock(return_value=self.current_run_string)

        self.presenter.handle_runs_loaded()

        self.model.run_number_strings.assert_called_once_with()
        self.view.update_run_selector_combo_box.assert_called_once_with(self.run_strings)
        self.view.current_run_string.assert_called_once_with()
        self.model.set_current_run_string.assert_called_once_with(self.current_run_string)
        self.mock_model_number_of_run_strings.assert_called_once_with()

    def test_that_handle_runs_loaded_will_disable_the_view_if_there_are_no_runs_loaded(self):
        run_strings = []
        self.model.run_number_strings = mock.Mock(return_value=run_strings)
        self.view.current_run_string = mock.Mock(return_value="")

        self.mock_model_number_of_run_strings = mock.PropertyMock(return_value=len(run_strings))
        type(self.model).number_of_run_strings = self.mock_model_number_of_run_strings

        self.presenter.handle_runs_loaded()

        self.mock_model_number_of_run_strings.assert_called_once_with()
        self.view.disable_view.assert_called_once_with()

    def test_that_handle_runs_loaded_will_enable_the_view_if_there_are_runs_loaded(self):
        self.model.run_number_strings = mock.Mock(return_value=self.run_strings)
        self.view.current_run_string = mock.Mock(return_value=self.current_run_string)

        self.presenter.handle_runs_loaded()

        self.mock_model_number_of_run_strings.assert_called_once_with()
        self.view.enable_view.assert_called_once_with()

    def test_that_handle_run_selector_changed_will_update_the_run_string_in_the_model(self):
        self.view.current_run_string = mock.Mock(return_value=self.current_run_string)

        self.presenter.handle_run_selector_changed()

        self.view.current_run_string.assert_called_once_with()
        self.model.set_current_run_string.assert_called_once_with(self.current_run_string)
        self.presenter.dead_time_presenter.handle_run_selector_changed.assert_called_once_with()
        self.presenter.background_presenter.handle_run_selector_changed.assert_called_once_with()

    def test_that_handle_pre_process_and_counts_calculated_will_update_the_dead_time_label_in_the_view(self):
        self.presenter.handle_pre_process_and_counts_calculated()
        self.presenter.dead_time_presenter.handle_pre_process_and_counts_calculated.assert_called_once_with()
        self.presenter.background_presenter.handle_pre_process_and_counts_calculated.assert_called_once_with()

    def test_that_handle_groups_changed_will_notify_the_background_corrections_presenter(self):
        self.presenter.handle_groups_changed()
        self.presenter.background_presenter.handle_groups_changed.assert_called_once_with()

    def test_that_handle_thread_calculation_started_notifies_to_disable_editing(self):
        self.presenter.handle_thread_calculation_started()
        self.presenter.disable_editing_notifier.notify_subscribers.assert_called_once_with()

    def test_that_handle_background_corrections_for_all_finished_calls_the_expected_methods(self):
        self.presenter.handle_background_corrections_for_all_finished()

        self.presenter.enable_editing_notifier.notify_subscribers.assert_called_once_with()
        self.presenter.background_presenter.handle_background_corrections_for_all_finished.assert_called_once_with()
        self.mock_presenter_correction_results.assert_called_once_with()
        self.presenter._perform_asymmetry_pairs_and_diffs_calculation.assert_called_once_with(self.run_strings,
                                                                                              self.groups)

    def test_that_handle_asymmetry_pairs_and_diffs_calc_finished_calls_the_expected_notifiers(self):
        self.presenter.handle_asymmetry_pairs_and_diffs_calc_finished()

        self.presenter.enable_editing_notifier.notify_subscribers.assert_called_once_with()
        self.presenter.asymmetry_pair_and_diff_calculations_finished_notifier.notify_subscribers.assert_called_once_with()

    def test_that_handle_thread_error_will_attempt_to_show_a_warning_popup(self):
        error = "This is an error message."
        self.presenter.handle_thread_error(error)

        self.presenter.disable_editing_notifier.notify_subscribers.assert_called_once_with()
        self.view.warning_popup.assert_called_once_with(error)

    def _setup_mock_view(self):
        self.view = mock.Mock(spec=CorrectionsView)

        self.view.set_slot_for_run_selector_changed = mock.Mock()

        self.view.update_run_selector_combo_box = mock.Mock()
        self.view.warning_popup = mock.Mock()
        self.view.disable_view = mock.Mock()
        self.view.enable_view = mock.Mock()

    def _setup_mock_model(self):
        self.model = mock.Mock(spec=CorrectionsModel)

        self.model.set_current_run_string = mock.Mock()

        # Mock the properties of the model
        self.mock_model_number_of_run_strings = mock.PropertyMock(return_value=len(self.run_strings))
        type(self.model).number_of_run_strings = self.mock_model_number_of_run_strings

    def _setup_presenter(self):
        self.context = setup_context()
        self.presenter = CorrectionsPresenter(self.view, self.model, self.context)
        self.presenter.disable_editing_notifier = mock.Mock()
        self.presenter.enable_editing_notifier = mock.Mock()
        self.presenter.asymmetry_pair_and_diff_calculations_finished_notifier = mock.Mock()

        self.presenter.dead_time_presenter.initialize_model_options = mock.Mock()
        self.presenter.dead_time_presenter.handle_ads_clear_or_remove_workspace_event = mock.Mock()
        self.presenter.dead_time_presenter.handle_instrument_changed = mock.Mock()
        self.presenter.dead_time_presenter.handle_run_selector_changed = mock.Mock()
        self.presenter.dead_time_presenter.handle_pre_process_and_counts_calculated = mock.Mock()

        self.presenter.background_presenter.initialize_model_options = mock.Mock()
        self.presenter.background_presenter.handle_instrument_changed = mock.Mock()
        self.presenter.background_presenter.handle_runs_loaded = mock.Mock()
        self.presenter.background_presenter.handle_run_selector_changed = mock.Mock()
        self.presenter.background_presenter.handle_groups_changed = mock.Mock()
        self.presenter.background_presenter.handle_pre_process_and_counts_calculated = mock.Mock()
        self.presenter.background_presenter.handle_background_corrections_for_all_finished = mock.Mock()

        self.presenter._handle_selected_table_is_invalid = mock.Mock()
        self.presenter._notify_perform_dead_time_corrections = mock.Mock()
        self.presenter._perform_asymmetry_pairs_and_diffs_calculation = mock.Mock()

        # Mock the correction result property
        self.presenter.thread_model_wrapper = mock.Mock(spec=ThreadModel)
        self.mock_presenter_correction_results = mock.PropertyMock(return_value=(self.run_strings, self.groups))
        type(self.presenter.thread_model_wrapper).result = self.mock_presenter_correction_results


if __name__ == '__main__':
    unittest.main()
