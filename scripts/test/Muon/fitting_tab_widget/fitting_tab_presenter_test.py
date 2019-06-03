import unittest
from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest
from qtpy import QtWidgets
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_widget import FittingTabWidget
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantid.api import FunctionFactory


def retrieve_combobox_info(combo_box):
    output_list = []
    for i in range(combo_box.count()):
        output_list.append(str(combo_box.itemText(i)))

    return output_list


def wait_for_thread(thread_model):
    if thread_model:
        thread_model._thread.wait()
        QtWidgets.QApplication.instance().processEvents()


class FittingTabPresenterTest(GuiTest):
    def setUp(self):
        self.context = setup_context()
        self.context.data_context.current_runs = [[62260]]
        self.context.data_context.instrument = 'MUSR'
        self.widget = FittingTabWidget(self.context, parent=None)
        self.presenter = self.widget.fitting_tab_presenter
        self.view = self.widget.fitting_tab_view
        self.presenter.model = mock.MagicMock()

    @mock.patch('Muon.GUI.Common.fitting_tab_widget.fitting_tab_presenter.WorkspaceSelectorView.get_selected_data')
    def test_handle_select_fit_data_clicked_updates_current_run_list(self, dialog_mock):
        dialog_mock.return_value = (['MUSR62260; Group; bkwd; Asymmetry; #1', 'MUSR62260; Group; bottom; Asymmetry; #1',
                                     'MUSR62260; Group; fwd; Asymmetry; #1', 'MUSR62260; Group; top; Asymmetry; #1',
                                     'MUSR62260; Pair Asym; long; #1', 'MUSR62260; PhaseQuad; PhaseTable MUSR62260',
                                     'MUSR62260; PhaseQuad; PhaseTable MUSR62261'], True)

        self.presenter.handle_select_fit_data_clicked()

        dialog_mock.assert_called_once_with([[62260]], 'MUSR', [], True, self.context, self.view)

        self.assertEqual(retrieve_combobox_info(self.view.parameter_display_combo),
                         ['MUSR62260; Group; bkwd; Asymmetry; #1', 'MUSR62260; Group; bottom; Asymmetry; #1',
                          'MUSR62260; Group; fwd; Asymmetry; #1', 'MUSR62260; Group; top; Asymmetry; #1',
                          'MUSR62260; Pair Asym; long; #1', 'MUSR62260; PhaseQuad; PhaseTable MUSR62260',
                          'MUSR62260; PhaseQuad; PhaseTable MUSR62261'])

    def test_that_changeing_fitting_type_to_multiple_fit_changes_workspace_selector_combo_label(
            self):
        self.assertEqual(self.view.workspace_combo_box_label.text(), 'Select Workspace')
        self.view.sequential_fit_radio.toggle()

        self.assertEqual(self.view.workspace_combo_box_label.text(), 'Display parameters for')

    def test_that_changeing_fit_type_to_single_fit_updates_label(self):
        self.view.sequential_fit_radio.toggle()
        self.view.single_fit_radio.toggle()

        self.assertEqual(self.view.workspace_combo_box_label.text(), 'Select Workspace')

    def test_when_fit_is_clicked_in_single_mode_the_fit_function_string_workspace_and_additional_options_are_passed_to_fit(
            self):
        self.presenter.selected_data = ['Input Workspace Name']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

        self.view.fit_button.clicked.emit(True)
        wait_for_thread(self.presenter.calculation_thread)

        self.presenter.model.do_single_fit.assert_called_once_with(
            {'Function': mock.ANY, 'InputWorkspace': 'Input Workspace Name',
             'Minimizer': 'Levenberg-Marquardt', 'StartX': 0.0, 'EndX': 15.0, 'EvaluationType': 'CentrePoint'})

        self.assertEqual(str(self.presenter.model.do_single_fit.call_args[0][0]['Function']), 'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

    def test_get_parameters_for_single_fit_returns_correctly(self):
        self.presenter.selected_data = ['Input Workspace Name']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        result = self.presenter.get_parameters_for_single_fit()

        self.assertEqual(result, {'Function': mock.ANY,
                                  'InputWorkspace': 'Input Workspace Name',
                                  'Minimizer': 'Levenberg-Marquardt', 'StartX': 0.0, 'EndX': 15.0,
                                  'EvaluationType': 'CentrePoint'}
                         )

    def test_for_single_fit_mode_when_display_workspace_changes_updates_fitting_browser_with_new_name(self):
        self.presenter.selected_data = ['Input Workspace Name']

        self.presenter.handle_display_workspace_changed()

        self.assertEqual(self.view.function_browser.getDatasetNames(), ['Input Workspace Name'])

    def test_fit_clicked_with_simultaneous_selected(self):
        self.view.simul_fit_radio.toggle()
        self.presenter.selected_data = ['Input Workspace Name_1', 'Input Workspace Name 2']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

        self.view.fit_button.clicked.emit(True)
        wait_for_thread(self.presenter.calculation_thread)

        call_args_dict = self.presenter.model.do_simultaneous_fit.call_args[0][0]

        self.assertEqual(call_args_dict['InputWorkspace'], ['Input Workspace Name_1', 'Input Workspace Name 2'])
        self.assertEqual(call_args_dict['Minimizer'], 'Levenberg-Marquardt')
        self.assertEqual(call_args_dict['StartX'], [0.0, 0.0])
        self.assertEqual(call_args_dict['EndX'], [15.0, 15.0])

    def test_when_new_data_is_selected_clear_out_old_fits_and_information(self):
        self.presenter._fit_status = ['success', 'success', 'success']
        self.presenter._fit_chi_squared = [12.3, 3.4, 0.35]
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.presenter_fit_function = [fit_function, fit_function, fit_function]
        self.presenter.manual_selection_made = True
        self.presenter._start_x = [0.15, 0.45, 0.67]
        self.presenter._end_x = [0.56, 0.78, 0.34]
        self.view.end_time = 0.56
        self.view.start_time = 0.15
        self.presenter.retrieve_first_good_data_from_run_name = mock.MagicMock(return_value=0.15)
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']

        self.presenter.selected_data = new_workspace_list

        self.assertEqual(self.presenter._fit_status, [None, None, None])
        self.assertEqual(self.presenter._fit_chi_squared, [0.0, 0.0, 0.0])
        self.assertEqual(self.presenter._fit_function, [None, None, None])
        self.assertEqual(self.presenter._selected_data, new_workspace_list)
        self.assertEqual(self.presenter.manual_selection_made, True)
        self.assertEqual(self.presenter.start_x, [0.15, 0.15, 0.15])
        self.assertEqual(self.presenter.end_x, [0.56, 0.56, 0.56])

    def test_when_new_data_is_selected_updates_combo_box_on_view(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']

        self.presenter.selected_data = new_workspace_list

        self.assertEqual(retrieve_combobox_info(self.view.parameter_display_combo), new_workspace_list)

    def test_when_new_data_is_selected_updates_fit_property_browser_appropriately_for_single_and_sequential(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']

        self.presenter.selected_data = new_workspace_list

        self.assertEqual(self.view.function_browser.getDatasetNames(), ['MUSR22725; Group; top; Asymmetry'])
        self.assertEqual(self.view.function_browser.getNumberOfDatasets(), 1)

    def test_when_new_data_is_selected_updates_fit_property_browser_appropriately_for_simultaneous(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        self.view.simul_fit_radio.toggle()

        self.presenter.selected_data = new_workspace_list

        self.assertEqual(self.view.function_browser.getDatasetNames(), new_workspace_list)
        self.assertEqual(self.view.function_browser.getNumberOfDatasets(), 3)

    def test_when_switching_to_simultaneous_function_browser_setup_correctly(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        self.presenter.selected_data = new_workspace_list
        self.presenter.context.get_names_of_workspaces_to_fit = mock.MagicMock(return_value=['MUSR22725; Group; top; Asymmetry'])

        self.view.simul_fit_radio.toggle()

        self.assertEqual(self.view.function_browser.getDatasetNames(), ['MUSR22725; Group; top; Asymmetry'])
        self.assertEqual(self.view.function_browser.getNumberOfDatasets(), 1)

    def test_when_switching_to_simultaneous_with_manual_selection_on_function_browser_setup_correctly(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        self.presenter.selected_data = new_workspace_list
        self.presenter.manual_selection_made = True

        self.view.simul_fit_radio.toggle()

        self.assertEqual(self.view.function_browser.getDatasetNames(), new_workspace_list)
        self.assertEqual(self.view.function_browser.getNumberOfDatasets(), 3)

    def test_when_switching_to_from_simultaneous_with_manual_selection_on_function_browser_setup_correctly(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        self.presenter.selected_data = new_workspace_list
        self.presenter.manual_selection_made = True
        self.view.simul_fit_radio.toggle()

        self.view.sequential_fit_radio.toggle()

        self.assertEqual(self.view.function_browser.getDatasetNames(), ['MUSR22725; Group; top; Asymmetry'])
        self.assertEqual(self.view.function_browser.getNumberOfDatasets(), 1)

    def test_display_workspace_changed_for_single_fit_updates_function_browser(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        self.presenter.selected_data = new_workspace_list
        self.presenter.manual_selection_made = True
        self.presenter._start_x = [0.15, 0.45, 0.67]
        self.presenter._end_x = [0.56, 0.78, 0.34]

        self.view.parameter_display_combo.setCurrentIndex(1)

        self.assertEqual(self.view.function_browser.getDatasetNames(), ['MUSR22725; Group; bottom; Asymmetry'])
        self.assertEqual(self.view.function_browser.getNumberOfDatasets(), 1)
        self.assertEqual(self.view.end_time, 0.78)
        self.assertEqual(self.view.start_time, 0.45)

    def test_display_workspace_changed_for_sequential_fit_updates_function_browser(self):
        self.view.sequential_fit_radio.toggle()
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        self.presenter.selected_data = new_workspace_list
        self.presenter.manual_selection_made = True
        self.presenter._start_x = [0.15, 0.45, 0.67]
        self.presenter._end_x = [0.56, 0.78, 0.34]

        self.view.parameter_display_combo.setCurrentIndex(1)

        self.assertEqual(self.view.function_browser.getDatasetNames(), ['MUSR22725; Group; bottom; Asymmetry'])
        self.assertEqual(self.view.function_browser.getNumberOfDatasets(), 1)
        self.assertEqual(self.view.end_time, 0.78)
        self.assertEqual(self.view.start_time, 0.45)

    def test_display_workspace_changed_for_simultaneous_fit_updates_function_browser(self):
        self.view.simul_fit_radio.toggle()
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        self.presenter.selected_data = new_workspace_list
        self.presenter.manual_selection_made = True
        self.presenter._start_x = [0.15, 0.45, 0.67]
        self.presenter._end_x = [0.56, 0.78, 0.34]

        self.view.parameter_display_combo.setCurrentIndex(1)

        self.assertEqual(self.view.function_browser.getDatasetNames(), new_workspace_list)
        self.assertEqual(self.view.function_browser.getNumberOfDatasets(), 3)
        # self.assertEqual(self.view.function_browser.getCurrentDataset(), 1) TODO FunctionBrowser seems to have an issue here
        self.assertEqual(self.view.end_time, 0.78)
        self.assertEqual(self.view.start_time, 0.45)

    def test_for_single_and_sequential_handle_display_workspace_changed_updates_the_displayed_function(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        fit_function_1 = FunctionFactory.createInitialized('name=GausOsc,A=0.6,Sigma=0.6,Frequency=0.6,Phi=0')
        self.view.function_browser.setFunction(str(fit_function))
        self.presenter.selected_data = new_workspace_list
        self.presenter.manual_selection_made = True
        self.presenter._start_x = [0.15, 0.45, 0.67]
        self.presenter._end_x = [0.56, 0.78, 0.34]
        self.presenter._fit_status = ['success', 'failure with message', 'success']
        self.presenter._fit_chi_squared = [12.3, 3.4, 0.35]
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        fit_function_1 = FunctionFactory.createInitialized('name=GausOsc,A=0.6,Sigma=0.6,Frequency=0.6,Phi=0')
        self.presenter._fit_function = [fit_function, fit_function_1, fit_function]

        self.view.parameter_display_combo.setCurrentIndex(1)

        self.assertEqual(self.view.function_browser.getFitFunctionString(), 'name=GausOsc,A=0.6,Sigma=0.6,Frequency=0.6,Phi=0')
        self.assertEqual(self.view.fit_status_success_failure.text(), 'Failure: failure with message')

    def test_get_first_good_data_for_workspace_retrieves_correct_first_good_data(self):
        self.presenter.context.first_good_data = mock.MagicMock(return_value=0.34)

        first_good_data = self.presenter.retrieve_first_good_data_from_run_name('MUSR62260; Group; top; Asymmetry; #1')

        self.assertEqual(first_good_data, 0.34)
        self.presenter.context.first_good_data.assert_called_once_with([62260])

    def test_setting_selected_list_empty_list_handled_correctly(self):
        self.presenter.selected_data = ['MUSR62260 top']

        self.presenter.selected_data = []

        self.assertEqual(self.view.fit_status_success_failure.text(), 'No Fit')


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
