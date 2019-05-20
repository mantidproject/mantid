import unittest
from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_widget import FittingTabWidget
from Muon.GUI.Common.test_helpers.context_setup import setup_context


def retrieve_combobox_info(combo_box):
    output_list = []
    for i in range(combo_box.count()):
        output_list.append(str(combo_box.itemText(i)))

    return output_list


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

    def test_that_changeing_fitting_type_to_multiple_fit_enables_workspace_selection_button_and_changes_workspace_selector_combo_label(
            self):
        self.view.fit_type_combo.setCurrentIndex(1)

        self.assertEqual(self.view.workspace_combo_box_label.text(), 'Display parameters for')
        self.assertTrue(self.view.select_workspaces_to_fit_button.isEnabled())

    def test_that_changeing_fit_type_to_single_fit_disables_workspace_selection_and_updates_label(self):
        self.view.fit_type_combo.setCurrentIndex(1)
        self.view.fit_type_combo.setCurrentIndex(0)

        self.assertEqual(self.view.workspace_combo_box_label.text(), 'Select Workspace')
        self.assertFalse(self.view.select_workspaces_to_fit_button.isEnabled())

    def test_when_fit_is_clicked_in_single_mode_the_fit_function_string_workspace_and_additional_options_are_passed_to_fit(
            self):
        self.presenter.selected_data = ['Input Workspace Name']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.view.fit_button.clicked.emit(True)

        self.presenter.model.do_single_fit.assert_called_once_with(
            {'Function': 'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0', 'InputWorkspace': 'Input Workspace Name',
             'Minimizer': 'Levenberg-Marquardt', 'StartX': 0.0, 'EndX': 15.0, 'EvaluationType': 'CentrePoint'})

    def test_get_parameters_for_single_fit_returns_correctly(self):
        self.presenter.selected_data = ['Input Workspace Name']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        result = self.presenter.get_parameters_for_single_fit()

        self.assertEqual(result, {'Function': 'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0', 'InputWorkspace': 'Input Workspace Name',
             'Minimizer': 'Levenberg-Marquardt', 'StartX': 0.0, 'EndX': 15.0, 'EvaluationType': 'CentrePoint'}
                         )

    def test_for_single_fit_mode_when_display_workspace_changes_updates_fitting_browser_with_new_name(self):
        self.presenter.selected_data = ['Input Workspace Name']

        self.presenter.handle_display_workspace_changed()

        self.assertEqual(self.view.function_browser.getDatasetNames(), ['Input Workspace Name'])

    def test_fit_clicked_with_simultaneous_selected(self):
        self.view.fit_type_combo.setCurrentIndex(2)
        self.presenter.selected_data = ['Input Workspace Name_1', 'Input Workspace Name 2']
        self.view.function_browser.setFunction('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.view.fit_button.clicked.emit(True)

        self.presenter.model.do_simultaneous_fit.assert_called_once_with(
            {'Function': 'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0', 'InputWorkspace': ['Input Workspace Name_1', 'Input Workspace Name 2'],
             'Minimizer': 'Levenberg-Marquardt', 'StartX': [0.0, 0.0], 'EndX': [15.0, 15.0], 'EvaluationType': 'CentrePoint'})


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
