# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.py3compat import mock
from Muon.GUI.Common.results_tab_widget.results_tab_presenter import ResultsTabPresenter

RESULTS_TAB_MODEL_CLS = 'Muon.GUI.Common.results_tab_widget.results_tab_model.ResultsTabModel'
RESULTS_TAB_VIEW_CLS = 'Muon.GUI.Common.results_tab_widget.results_tab_widget.ResultsTabView'


class ResultsTabPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model_patcher = mock.patch(RESULTS_TAB_MODEL_CLS, autospec=True)
        self.view_patcher = mock.patch(RESULTS_TAB_VIEW_CLS, autospec=True)

        self.mock_model = self.model_patcher.start()
        self.mock_view = self.view_patcher.start()
        self.mock_view.function_selection_changed.connect = mock.MagicMock()
        self.mock_view.results_name_edited.connect = mock.MagicMock()

    def tearDown(self):
        self.view_patcher.stop()
        self.model_patcher.stop()

    def test_presenter_sets_up_view_correctly(self):
        self.mock_model.results_table_name.return_value = 'default_table'
        self.mock_view.function_selection_changed.connect = mock.MagicMock()
        self.mock_view.results_name_edited.connect = mock.MagicMock()

        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        self.mock_view.set_results_table_name.assert_called_once_with(
            'default_table')
        self.mock_view.results_name_edited.connect.assert_called_once_with(
            presenter.on_results_table_name_edited)
        self.mock_view.set_output_results_button_enabled.assert_called_once_with(
            False)

    def test_editing_results_name_updates_model_value(self):
        new_name = 'edited_name'
        self.mock_view.results_table_name.return_value = new_name
        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        presenter.on_results_table_name_edited()

        self.mock_view.results_table_name.assert_called_once_with()
        self.mock_model.set_results_table_name.assert_called_once_with(
            new_name)

    def test_adding_new_fit_to_existing_fits_preserves_current_selections(
            self):
        orig_ws_list_state = {
            name: [index, checked, True]
            for index, (name, checked) in enumerate((('ws1', True), ('ws2',
                                                                     False)))
        }
        final_ws_list_state = {
            name: [index, checked, True]
            for index, (name, checked) in enumerate((('ws1', True),
                                                     ('ws2', False), ('ws3',
                                                                      True)))
        }
        test_functions = ['func1', 'func2']
        self.mock_model.fit_functions.return_value = test_functions
        self.mock_model.fit_selection.return_value = final_ws_list_state
        self.mock_view.fit_result_workspaces.return_value = orig_ws_list_state

        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        # previous test verifies this is correct on construction
        self.mock_view.set_output_results_button_enabled.reset_mock()
        presenter.on_new_fit_performed()

        self.mock_model.fit_functions.assert_called_once_with()
        self.mock_model.fit_selection.assert_called_once_with(
            orig_ws_list_state)
        self.mock_view.set_fit_function_names.assert_called_once_with(
            test_functions)
        self.mock_view.fit_result_workspaces.assert_called_once_with()
        self.mock_view.set_fit_result_workspaces.assert_called_once_with(
            final_ws_list_state)
        self.mock_view.set_output_results_button_enabled.assert_called_once_with(
            True)

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
        # previous test verifies this is correct on construction
        self.mock_view.set_output_results_button_enabled.reset_mock()
        presenter.on_new_fit_performed()

        self.mock_view.log_values.assert_called_once_with()
        self.mock_model.log_selection.assert_called_once_with(
            existing_selection)
        final_selection = {
            'run_number': [0, False, True],
            'run_start': [1, True, True],
            'magnetic_field': [2, True, True]
        }
        self.mock_view.set_log_values.assert_called_once_with(final_selection)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
