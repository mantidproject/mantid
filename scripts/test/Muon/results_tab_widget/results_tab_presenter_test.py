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
        self.mock_model.results_table_name.return_value='default_table'
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

    def test_adding_new_fit_populates_tables(self):
        test_functions = ['func1', 'func2']
        expected_ws_list_state = {
            name: [index, True, True]
            for index, name in enumerate(('ws1', 'ws2'))
        }
        self.mock_model.fit_functions.return_value = test_functions
        self.mock_model.fit_input_workspaces.return_value = expected_ws_list_state
        presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
        # previous test verifies this is correct on construction
        self.mock_view.set_output_results_button_enabled.reset_mock()

        presenter.on_new_fit_performed()

        self.mock_model.fit_functions.assert_called_once_with()
        self.mock_view.set_fit_function_names.assert_called_once_with(test_functions)
        self.mock_view.set_fit_result_workspaces.assert_called_once_with(expected_ws_list_state)
        self.mock_view.set_output_results_button_enabled.assert_called_once_with(True)

    # def test_changing_function_name(self):
    #     # new_name = 'edited_name'
    #     # self.mock_view.results_table_name.return_value = new_name
    #     presenter = ResultsTabPresenter(self.mock_view, self.mock_model)
    #     presenter.on_function_selection_changed()
    #
    #     self.mock_view.selected_fit_function.assert_called_once()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
