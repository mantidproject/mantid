# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FunctionFactory
from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest
from qtpy import QtWidgets

from Muon.GUI.Common.fitting_tab_widget.fitting_tab_widget import FittingTabWidget
from Muon.GUI.Common.test_helpers.context_setup import setup_context

EXAMPLE_TF_ASYMMETRY_FUNCTION = '(composite=ProductFunction,NumDeriv=false;name=FlatBackground,A0=1.02709;' \
                                '(name=FlatBackground,A0=1,ties=(A0=1);name=ExpDecayOsc,A=0.2,Lambda=0.2,Frequency=0.1,Phi=0))' \
                                ';name=ExpDecayMuon,A=0,Lambda=-2.19698,ties=(A=0,Lambda=-2.19698)'


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
        self.view.warning_popup = mock.MagicMock()
        self.presenter.model = mock.MagicMock()
        self.presenter.model.get_function_name.return_value = 'GausOsc'

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
        self.presenter.model.do_single_fit.return_value = (self.view.function_browser.getGlobalFunction(),
                                                           'Fit Suceeded', 0.5)

        self.view.fit_button.clicked.emit(True)
        wait_for_thread(self.presenter.calculation_thread)

        self.presenter.model.do_single_fit.assert_called_once_with(
            {'Function': mock.ANY, 'InputWorkspace': 'Input Workspace Name',
             'Minimizer': 'Levenberg-Marquardt', 'StartX': 0.0, 'EndX': 15.0, 'EvaluationType': 'CentrePoint',
             'FitGroupName': 'Fitting Results'})

        self.assertEqual(str(self.presenter.model.do_single_fit.call_args[0][0]['Function']),
                         'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

    def test_get_parameters_for_single_fit_returns_correctly(self):
        self.presenter.selected_data = ['Input Workspace Name']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        result = self.presenter.get_parameters_for_single_fit()

        self.assertEqual(result, {'Function': mock.ANY,
                                  'InputWorkspace': 'Input Workspace Name',
                                  'Minimizer': 'Levenberg-Marquardt', 'StartX': 0.0, 'EndX': 15.0,
                                  'EvaluationType': 'CentrePoint', 'FitGroupName': 'Fitting Results'}
                         )

    def test_for_single_fit_mode_when_display_workspace_changes_updates_fitting_browser_with_new_name(self):
        self.presenter.selected_data = ['Input Workspace Name']

        self.presenter.handle_display_workspace_changed()

        self.assertEqual(self.view.function_browser.getDatasetNames(), ['Input Workspace Name'])

    def test_fit_clicked_with_simultaneous_selected_and_no_globals(self):
        self.presenter.model.get_function_name.return_value = 'GausOsc'
        self.presenter.selected_data = ['Input Workspace Name_1', 'Input Workspace Name 2']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.view.simul_fit_radio.toggle()
        self.presenter.model.do_simultaneous_fit.return_value = (self.view.function_browser_multi.getGlobalFunction(),
                                                                 'Fit Suceeded', 0.5)

        self.view.fit_button.clicked.emit(True)
        wait_for_thread(self.presenter.calculation_thread)

        simultaneous_call_args = self.presenter.model.do_simultaneous_fit.call_args
        call_args_dict = simultaneous_call_args[0][0]

        self.assertEqual(call_args_dict['InputWorkspace'], ['Input Workspace Name_1', 'Input Workspace Name 2'])
        self.assertEqual(call_args_dict['Minimizer'], 'Levenberg-Marquardt')
        self.assertEqual(call_args_dict['StartX'], [0.0, 0.0])
        self.assertEqual(call_args_dict['EndX'], [15.0, 15.0])

        call_args_globals = simultaneous_call_args[0][1]
        self.assertEqual(call_args_globals, [])

    def test_fit_clicked_with_simultaneous_selected_with_global_parameters(self):
        self.presenter.model.get_function_name.return_value = 'GausOsc'

        self.presenter.selected_data = ['Input Workspace Name_1', 'Input Workspace Name 2']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.view.simul_fit_radio.toggle()
        self.view.function_browser_multi.setGlobalParameters(['A'])
        self.presenter.model.do_simultaneous_fit.return_value = (self.view.function_browser_multi.getGlobalFunction(),
                                                                 'Fit Suceeded', 0.5)

        self.view.fit_button.clicked.emit(True)
        wait_for_thread(self.presenter.calculation_thread)

        simultaneous_call_args = self.presenter.model.do_simultaneous_fit.call_args

        call_args_dict = simultaneous_call_args[0][0]
        self.assertEqual(call_args_dict['InputWorkspace'], ['Input Workspace Name_1', 'Input Workspace Name 2'])
        self.assertEqual(call_args_dict['Minimizer'], 'Levenberg-Marquardt')
        self.assertEqual(call_args_dict['StartX'], [0.0, 0.0])
        self.assertEqual(call_args_dict['EndX'], [15.0, 15.0])

        call_args_globals = simultaneous_call_args[0][1]
        self.assertEqual(call_args_globals, ['A'])

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

        self.assertEqual(self.view.function_browser_multi.getDatasetNames(), new_workspace_list)
        self.assertEqual(self.view.function_browser_multi.getNumberOfDatasets(), 3)

    def test_when_switching_to_simultaneous_function_browser_setup_correctly(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        self.presenter.selected_data = new_workspace_list
        self.presenter.context.get_names_of_workspaces_to_fit = mock.MagicMock(
            return_value=['MUSR22725; Group; top; Asymmetry'])

        self.view.simul_fit_radio.toggle()

        self.assertEqual(self.view.function_browser.getDatasetNames(), ['MUSR22725; Group; top; Asymmetry'])
        self.assertEqual(self.view.function_browser.getNumberOfDatasets(), 1)

    def test_when_switching_to_simultaneous_with_manual_selection_on_function_browser_setup_correctly(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        self.presenter.selected_data = new_workspace_list
        self.presenter.manual_selection_made = True

        self.view.simul_fit_radio.toggle()

        self.assertEqual(self.view.function_browser_multi.getDatasetNames(), new_workspace_list)
        self.assertEqual(self.view.function_browser_multi.getNumberOfDatasets(), 3)

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

        self.assertEqual(self.view.function_browser_multi.getDatasetNames(), new_workspace_list)
        self.assertEqual(self.view.function_browser_multi.getNumberOfDatasets(), 3)
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

        self.presenter._fit_function = [fit_function, fit_function_1, fit_function]

        self.view.parameter_display_combo.setCurrentIndex(1)

        self.assertEqual(self.view.function_browser.getFitFunctionString(),
                         'name=GausOsc,A=0.6,Sigma=0.6,Frequency=0.6,Phi=0')
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

    def test_updating_function_updates_displayed_fit_name(self):
        self.presenter.model.get_function_name.return_value = 'GausOsc'

        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

        self.assertEqual(self.view.function_name, 'GausOsc')

    def test_fit_name_not_updated_if_already_changed_by_user(self):
        self.view.function_name = 'test function'
        self.view.function_name_line_edit.textChanged.emit('test function')

        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

        self.assertEqual(self.view.function_name, 'test function')

    def test_check_workspaces_are_tf_asymmetry_compliant(self):
        list_of_lists_to_test = [
            ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
             'MUSR22725; Group; fwd; Asymmetry'],
            ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
             'MUSR22725; Pair; long; Asymmetry'],
            ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
             'MUSR22725; PhaseQuad']
        ]

        expected_results = [True, False, False]

        for workspace_list, expected_result in zip(list_of_lists_to_test, expected_results):
            result = self.presenter.check_workspaces_are_tf_asymmetry_compliant(workspace_list)
            self.assertEqual(result, expected_result)

    def test_get_parameters_for_tf_function_calculation_for_turning_mode_on(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        self.view.tf_asymmetry_mode_checkbox.blockSignals(True)
        self.view.tf_asymmetry_mode = True
        self.view.tf_asymmetry_mode_checkbox.blockSignals(False)
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.presenter.selected_data = new_workspace_list

        result = self.presenter.get_parameters_for_tf_function_calculation(fit_function)

        self.assertEqual(result, {'InputFunction': fit_function, 'WorkspaceList': [new_workspace_list[0]],
                                  'Mode': 'Construct'})

    def test_get_parameters_for_tf_function_calculation_for_turning_mode_off(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; Asymmetry']
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.presenter.selected_data = new_workspace_list

        result = self.presenter.get_parameters_for_tf_function_calculation(fit_function)

        self.assertEqual(result, {'InputFunction': fit_function, 'WorkspaceList': [new_workspace_list[0]],
                                  'Mode': 'Extract'})

    def test_handle_asymmetry_mode_changed_reverts_changed_and_shows_error_if_non_group_selected(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Pair; fwd; Long']
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.view.function_browser.setFunction(str(fit_function))
        self.presenter.selected_data = new_workspace_list

        self.view.tf_asymmetry_mode = True

        self.view.warning_popup.assert_called_once_with(
            'Can only fit groups in tf asymmetry mode and need a function defined')

    def test_handle_asymmetry_mode_correctly_updates_function_for_a_single_fit(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry']
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        fit_function_2 = FunctionFactory.createInitialized('name=GausOsc,A=1.0,Sigma=2.5,Frequency=0.1,Phi=0')
        self.view.function_browser.setFunction(str(fit_function))
        self.presenter.selected_data = new_workspace_list
        self.presenter.model.calculate_tf_function.return_value = fit_function_2

        self.view.tf_asymmetry_mode = True

        self.assertEqual(str(self.view.fit_object), str(fit_function_2))
        self.assertEqual([str(item) for item in self.presenter._fit_function], [str(fit_function_2)])

    def test_handle_asymmetry_mode_correctly_updates_function_for_a_sequential_fit(self):
        self.view.sequential_fit_radio.toggle()
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; fwd']
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        fit_function_2 = FunctionFactory.createInitialized('name=GausOsc,A=1.0,Sigma=2.5,Frequency=0.1,Phi=0')
        self.view.function_browser.setFunction(str(fit_function))
        self.presenter.selected_data = new_workspace_list
        self.presenter.model.calculate_tf_function.return_value = fit_function_2

        self.view.tf_asymmetry_mode = True

        self.assertEqual([str(item) for item in self.presenter._fit_function], [str(fit_function_2)] * 3)
        self.assertEqual(str(self.view.fit_object), str(fit_function_2))

    def test_handle_asymmetry_mode_correctly_updates_function_for_sumultaneous_fit(self):
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.view.function_browser.setFunction(str(fit_function))
        self.view.simul_fit_radio.toggle()
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; fwd']
        self.presenter.selected_data = new_workspace_list
        fit_function_2 = self.view.fit_object.clone()
        self.presenter.model.calculate_tf_function.return_value = fit_function_2

        self.view.tf_asymmetry_mode = True

        self.assertEqual([str(item) for item in self.presenter._fit_function], [str(fit_function_2)] * 3)
        self.assertEqual(str(self.view.fit_object), str(fit_function_2))

    def test_handle_asymmetry_mode_correctly_keeps_globals_for_simultaneous_fit(self):
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.view.function_browser.setFunction(str(fit_function))
        self.view.simul_fit_radio.toggle()
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; fwd']
        self.presenter.selected_data = new_workspace_list
        fit_function_2 = self.view.fit_object.clone()
        self.presenter.model.calculate_tf_function.return_value = fit_function_2
        self.view.function_browser_multi.setGlobalParameters(['A'])

        self.view.tf_asymmetry_mode = True

        self.assertEqual(self.view.get_global_parameters(), ['f0.f1.f1.A'])

    def test_handle_tf_asymmetry_mode_succesfully_converts_back(self):
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                              'MUSR22725; Group; fwd; fwd']
        self.presenter.selected_data = new_workspace_list
        self.view.function_browser.setFunction(EXAMPLE_TF_ASYMMETRY_FUNCTION)
        self.view.tf_asymmetry_mode_checkbox.blockSignals(True)
        self.view.tf_asymmetry_mode = True
        self.presenter._tf_asymmetry_mode = True
        self.view.tf_asymmetry_mode_checkbox.blockSignals(False)
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.presenter.model.calculate_tf_function.return_value = fit_function

        self.view.tf_asymmetry_mode = False

        self.assertEqual([str(item) for item in self.presenter._fit_function],
                         ['name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0',
                          'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0',
                          'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0'])
        self.assertEqual(str(self.view.fit_object), 'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

    def test_handle_fit_with_tf_asymmetry_mode_calls_CalculateMuonAsymmetry(self):
        self.presenter.model.get_function_name.return_value = 'GausOsc'
        self.presenter.model.create_fitted_workspace_name.return_value = ('workspace', 'workspace directory')
        self.presenter.handle_finished = mock.MagicMock()
        self.view.sequential_fit_radio.toggle()
        new_workspace_list = ['MUSR22725; Group; top; Asymmetry']
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        fit_function_2 = FunctionFactory.createInitialized(EXAMPLE_TF_ASYMMETRY_FUNCTION)
        self.view.function_browser.setFunction(str(fit_function))
        self.presenter.selected_data = new_workspace_list
        self.presenter.model.calculate_tf_function.return_value = fit_function_2
        self.view.tf_asymmetry_mode = True
        self.presenter.handle_fit_clicked()
        wait_for_thread(self.presenter.calculation_thread)

        self.assertEqual(self.presenter.model.do_sequential_tf_fit.call_count, 1)
        self.assertEqual(self.presenter.handle_finished.call_count, 1)

    def test_get_parameters_for_tf_single_fit_calculation(self):
        self.presenter.model.create_fitted_workspace_name.return_value = ('workspace', 'workspace directory')
        self.context.group_pair_context.get_unormalisised_workspace_list = mock.MagicMock(return_value=
                                                                                          [
                                                                                              '__MUSR22725; Group; top; Asymmetry_unnorm',
                                                                                              '__MUSR22725; Group; bottom; Asymmetry_unnorm',
                                                                                              '__MUSR22725; Group; fwd; Asymmetry_unnorm'])

        result = self.presenter.get_parameters_for_tf_single_fit_calculation()

        self.assertEqual(result, {'EndX': 15.0,
                                  'InputFunction': None,
                                  'Minimizer': 'Levenberg-Marquardt',
                                  'OutputFitWorkspace': 'workspace',
                                  'ReNormalizedWorkspaceList': '',
                                  'StartX': 0.0,
                                  'UnNormalizedWorkspaceList': '__MUSR22725; Group; top; Asymmetry_unnorm'}
                         )

    def test_on_function_structure_changed_stores_current_fit_state_in_relevant_presenter(self):
        self.presenter.selected_data = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                                        'MUSR22725; Group; fwd; Asymmetry']

        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

        self.assertEqual([str(item) for item in self.presenter._fit_function], ['name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0'] * 3)

    def test_updating_function_parameters_updates_relevant_stored_function(self):
        self.presenter.selected_data = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                                        'MUSR22725; Group; fwd; Asymmetry']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

        self.view.function_browser.setParameter('A', 1.5)

        self.assertEqual([str(item) for item in self.presenter._fit_function],
                         ['name=GausOsc,A=1.5,Sigma=0.2,Frequency=0.1,Phi=0'] + ['name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0'] * 2)

    def test_handle_display_workspace_changed_updates_displayed_single_function(self):
        self.presenter.selected_data = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                                        'MUSR22725; Group; fwd; Asymmetry']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.view.function_browser.setParameter('A', 1.5)

        self.view.parameter_display_combo.setCurrentIndex(1)

        self.assertEqual(str(self.view.fit_object), 'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

    def test_setting_selected_data_resets_function_browser_datasets(self):
        self.assertEqual(self.view.function_browser.getDatasetNames(), [])
        self.presenter.selected_data = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                                        'MUSR22725; Group; fwd; Asymmetry']

        self.assertEqual(self.view.function_browser.getDatasetNames(), ['MUSR22725; Group; top; Asymmetry'])
        self.assertEqual(self.view.function_browser_multi.getDatasetNames(), ['MUSR22725; Group; top; Asymmetry',
                                                                              'MUSR22725; Group; bottom; Asymmetry',
                                                                              'MUSR22725; Group; fwd; Asymmetry'])

    def test_switching_to_simultaneous_keeps_stored_fit_functions_same_length(self):
        self.presenter.selected_data = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                                        'MUSR22725; Group; fwd; Asymmetry']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

        self.view.simul_fit_radio.toggle()

        self.assertEqual([str(item) for item in self.presenter._fit_function], ['composite=MultiDomainFunction,NumDeriv=true;name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0,$domains=i'
                                                                                ';name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0,$domains=i;'
                                                                                'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0,$domains=i']*3)

    def test_switching_from_simultaneous_to_single_fit_updates_fit_functions_appropriately(self):
        self.presenter.selected_data = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                                        'MUSR22725; Group; fwd; Asymmetry']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

        self.view.simul_fit_radio.toggle()
        self.view.single_fit_radio.toggle()

        self.assertEqual([str(item) for item in self.presenter._fit_function], [
            'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0'] * 3)

    def test_undo_fit_button_disabled_until_a_succesful_fit_is_performed(self):
        self.assertEqual(self.view.undo_fit_button.isEnabled(), False)
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.presenter.fitting_calculation_model = mock.MagicMock()
        self.presenter.fitting_calculation_model.result = (fit_function, 'Success', 1.07)

        self.presenter.handle_finished()

        self.assertEqual(self.view.undo_fit_button.isEnabled(), True)

    def test_after_fit_fit_cache_is_populated(self):
        self.presenter.selected_data = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                                        'MUSR22725; Group; fwd; Asymmetry']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.5,Sigma=0.5,Frequency=1,Phi=0')
        self.presenter.fitting_calculation_model = mock.MagicMock()
        self.presenter.model.do_single_fit.return_value = (fit_function,'Fit Suceeded', 0.5)

        self.presenter.handle_fit_clicked()
        wait_for_thread(self.presenter.calculation_thread)

        self.assertEqual([str(item) for item in self.presenter._fit_function_cache], ['name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0'] * 3)

    def test_undo_fit_resets_fit_in_view(self):
        self.presenter.selected_data = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                                        'MUSR22725; Group; fwd; Asymmetry']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.5,Sigma=0.5,Frequency=1,Phi=0')
        self.presenter.fitting_calculation_model = mock.MagicMock()
        self.presenter.model.do_single_fit.return_value = (fit_function, 'Fit Suceeded', 0.5)
        self.presenter.handle_fit_clicked()
        wait_for_thread(self.presenter.calculation_thread)

        self.view.undo_fit_button.clicked.emit(True)

        self.assertEqual(str(self.view.fit_object), 'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

    def test_removing_fit_and_then_switching_displayed_workspace_does_not_lead_to_error(self):
        self.presenter.selected_data = ['MUSR22725; Group; top; Asymmetry', 'MUSR22725; Group; bottom; Asymmetry',
                                        'MUSR22725; Group; fwd; Asymmetry']

        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.view.function_browser.clear()

        self.view.parameter_display_combo.setCurrentIndex(1)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
