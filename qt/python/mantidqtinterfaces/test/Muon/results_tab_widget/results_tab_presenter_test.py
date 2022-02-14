
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqtinterfaces.Muon.GUI.Common.results_tab_widget.results_tab_presenter import ResultsTabPresenter
from mantidqt.utils.observer_pattern import GenericObservable

RESULTS_TAB_MODEL_CLS = 'mantidqtinterfaces.Muon.GUI.Common.results_tab_widget.results_tab_model.ResultsTabModel'
RESULTS_TAB_VIEW_CLS = 'mantidqtinterfaces.Muon.GUI.Common.results_tab_widget.results_tab_widget.ResultsTabView'


class mock_fit_info(object):
    def __init__(self, name):
        self.name = name

    def output_workspace_names(self):
        return [self.name]


class ResultsTabPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model_patcher = mock.patch(RESULTS_TAB_MODEL_CLS, autospec=True)
        self.view_patcher = mock.patch(RESULTS_TAB_VIEW_CLS, autospec=True)

        self.mock_model = self.model_patcher.start()
        self.mock_view = self.view_patcher.start()
        self.mock_view.function_selection_changed.connect = mock.MagicMock()
        self.mock_view.results_name_edited.connect = mock.MagicMock()
        self.mock_view.output_results_requested.connect = mock.MagicMock()
        self.mock_context = mock.MagicMock()
        self.mock_model._fit_context = self.mock_context

    def tearDown(self):
        self.view_patcher.stop()
        self.model_patcher.stop()

    def test_presenter_sets_up_view_correctly(self):
        self.mock_model.results_table_name.return_value = 'default_table'

        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        self.mock_view.set_results_table_name.assert_called_once_with(
            'default_table')
        self.mock_view.function_selection_changed.connect.assert_called_once_with(
            presenter.on_function_selection_changed)
        self.mock_view.results_name_edited.connect.assert_called_once_with(
            presenter.on_results_table_name_edited)
        self.mock_view.output_results_requested.connect.assert_called_once_with(
            presenter.on_output_results_request)
        self.mock_view.set_output_results_button_enabled.assert_called_once_with(
            False)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.results_tab_widget.results_tab_presenter.check_if_workspace_exist')
    def test_editing_results_name_updates_model_value(self, mock_check_ADS):
        mock_check_ADS.return_value = False
        new_name = 'edited_name'
        self.mock_view.results_table_name.return_value = new_name
        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        self.assertEqual(self.mock_view.set_output_results_button_no_warning.call_count, 1)
        presenter.on_results_table_name_edited()

        self.mock_view.results_table_name.assert_called_once_with()
        self.mock_model.set_results_table_name.assert_called_once_with(
            new_name)
        self.assertEqual(self.mock_view.set_output_results_button_no_warning.call_count, 2)
        self.mock_view.set_output_results_button_warning.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.results_tab_widget.results_tab_presenter.check_if_workspace_exist')
    def test_editing_results_name_updates_model_value_already_used_name(self, mock_check_ADS):
        mock_check_ADS.return_value = True
        new_name = 'edited_name'
        self.mock_view.results_table_name.return_value = new_name
        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        self.assertEqual(self.mock_view.set_output_results_button_no_warning.call_count, 1)
        presenter.on_results_table_name_edited()

        self.mock_view.results_table_name.assert_called_once_with()
        self.mock_model.set_results_table_name.assert_called_once_with(
            new_name)
        self.assertEqual(self.mock_view.set_output_results_button_no_warning.call_count, 1)
        self.mock_view.set_output_results_button_warning.assert_called_once_with()

    def test_changing_function_selection(self):
        new_name = 'func 2'
        self.mock_view.selected_fit_function.return_value = new_name
        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        presenter._get_workspace_list = mock.MagicMock(return_value=(['ws1', 'ws3'], "func 2"))
        presenter.on_function_selection_changed()

        self.mock_view.selected_fit_function.assert_called_once_with()
        self.mock_model.set_selected_fit_function.assert_called_once_with(new_name)

    def test_adding_new_fit_to_existing_fits_preserves_current_selections(
            self):
        ws1 = "ws1; parameters"
        ws2 = "ws2; parameters"
        fit_ws = "ws3; workspaces"
        ws3 = "ws3; parameters"

        # list is [row number, ticked, enabled] -> first selected, second is not
        orig_ws_list_state = {ws1: [0, True, True], ws2: [1, False, True]}
        final_ws_list_state = [ws1, ws2, ws3]
        test_functions = ['func1', 'func2']
        self.mock_model.fit_functions.return_value = test_functions
        self.mock_model.fit_selection.return_value = final_ws_list_state
        self.mock_view.fit_result_workspaces.return_value = orig_ws_list_state

        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        presenter._get_workspace_list = mock.MagicMock(return_value=([ws1, ws2, ws3], "func1"))
        # previous test verifies this is correct on construction
        self.mock_view.set_output_results_button_enabled.reset_mock()
        # add a new fit_ws
        fit_info = mock_fit_info(fit_ws)

        presenter.on_new_fit_performed(fit_info)

        self.mock_model.fit_functions.assert_called_once_with()
        # only the first ws and the new one are selected
        self.mock_model.fit_selection.assert_called_once_with([ws1, ws3])
        self.mock_view.set_fit_function_names.assert_called_once_with(
            test_functions)
        self.mock_view.set_fit_result_workspaces.assert_called_once_with(
            final_ws_list_state)
        self.mock_view.set_output_results_button_enabled.assert_called_once_with(
            True)

    def test_redoing_fit_to_updates_selections(
            self):
        ws1 = "ws1; parameters"
        ws2 = "ws2; parameters"
        fit_ws = "ws1; workspaces"

        # list is [row number, ticked, enabled] -> nothing selected
        orig_ws_list_state = {ws1: [0, False, True], ws2: [1, False, True]}
        final_ws_list_state = [ws1, ws2]
        test_functions = ['func1', 'func2']
        self.mock_model.fit_functions.return_value = test_functions
        self.mock_model.fit_selection.return_value = final_ws_list_state
        self.mock_view.fit_result_workspaces.return_value = orig_ws_list_state

        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        presenter._get_workspace_list = mock.MagicMock(return_value=([ws1, ws2], "func1"))
        # previous test verifies this is correct on construction
        self.mock_view.set_output_results_button_enabled.reset_mock()
        # add a new fit_ws
        fit_info = mock_fit_info(fit_ws)

        presenter.on_new_fit_performed(fit_info)

        self.mock_model.fit_functions.assert_called_once_with()
        # ince ws1 has updated it is reselected
        self.mock_model.fit_selection.assert_called_once_with([ws1])
        self.mock_view.set_fit_function_names.assert_called_once_with(
            test_functions)
        self.mock_view.set_fit_result_workspaces.assert_called_once_with(
            final_ws_list_state)
        self.mock_view.set_output_results_button_enabled.assert_called_once_with(
            True)

    def test_if_no_fits_in_context_then_output_results_is_disabled(self):
        self.mock_model._fit_context.clear()
        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        fit_info = mock_fit_info("ws1")
        presenter.on_new_fit_performed(fit_info)
        expected_calls = [mock.call(False), mock.call(True)]  # Called once in init of view and once on new fit

        self.mock_view.set_output_results_button_enabled.assert_has_calls(expected_calls)

    def test_adding_new_fit_updates_log_values(self):
        existing_selection = {
            'run_number': [0, False, True],
            'run_start': [1, True, True]
        }
        self.mock_view.log_values.return_value = existing_selection
        final_selection = {
            'run_number': [0, False, True],
            'run_start': [1, True, True],
            'magnetic_field': [2, True, True]
        }
        self.mock_model.log_selection.return_value = final_selection

        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        presenter._update_logs_view()

        self.mock_view.log_values.assert_called_once_with()
        self.mock_model.log_selection.assert_called_once_with(
            existing_selection=existing_selection)
        final_selection = {
            'run_number': [0, False, True],
            'run_start': [1, True, True],
            'magnetic_field': [2, True, True]
        }
        self.mock_view.set_log_values.assert_called_once_with(final_selection)

    def test_results_table_request_calls_table_creation_on_model(self):
        fit_selection = ['ws1']
        log_selection = []
        self.mock_view.selected_result_workspaces.return_value = fit_selection
        self.mock_view.selected_log_values.return_value = log_selection
        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        self.assertEqual(self.mock_view.set_output_results_button_warning.call_count, 0)
        # previous test verifies this is correct on construction
        self.mock_view.set_output_results_button_enabled.reset_mock()

        presenter.on_output_results_request()

        self.mock_model.create_results_table.assert_called_once_with(
            log_selection, fit_selection)
        self.assertEqual(self.mock_view.set_output_results_button_warning.call_count, 1)

    def test_results_table_request_with_empty_results_does_nothing(self):
        self.mock_view.selected_result_workspaces.return_value = []

        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        # previous test verifies this is correct on construction
        self.mock_view.set_output_results_button_enabled.reset_mock()

        presenter.on_output_results_request()

        self.assertEqual(0, self.mock_model.create_results_table.call_count)

    def test_that_updates_function_name_in_model_when_new_fit_performed(self):
        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        presenter._update_fit_results_view_on_new_fit = mock.MagicMock()

        # Calling private method to avoid event loop
        presenter._on_new_fit_performed_impl()

        self.mock_model.on_new_fit_performed.assert_called_once_with()

    def test_that_disable_observer_calls_on_view_when_triggered(self):
        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        disable_notifier = GenericObservable()
        disable_notifier.add_subscriber(presenter.disable_tab_observer)

        disable_notifier._notify_subscribers_impl(arg=None)
        self.mock_view.setEnabled.assert_called_once_with(False)

    def test_that_enable_observer_calls_on_view_when_triggered(self):
        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        enable_notifier = GenericObservable()
        enable_notifier.add_subscriber(presenter.enable_tab_observer)

        enable_notifier._notify_subscribers_impl(arg=None)
        self.mock_view.setEnabled.assert_called_once_with(True)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
