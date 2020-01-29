# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore
from Muon.GUI.Common.utilities import table_utils
from Muon.GUI.Common.message_box import warning
from mantidqt.utils.qt import load_ui
from mantidqt.widgets.functionbrowser import FunctionBrowser

ui_fitting_tab, _ = load_ui(__file__, "fitting_tab.ui")
allowed_minimizers = ['Levenberg-Marquardt', 'BFGS', 'Conjugate gradient (Fletcher-Reeves imp.)',
                      'Conjugate gradient (Polak-Ribiere imp.)',
                      'Damped GaussNewton', 'Levenberg-MarquardtMD', 'Simplex',
                      'SteepestDescent', 'Trust Region']


class FittingTabView(QtWidgets.QWidget, ui_fitting_tab):
    def __init__(self, parent=None):
        super(FittingTabView, self).__init__(parent)
        self.setupUi(self)
        self.setup_fit_options_table()
        self.setup_simul_fit_combo_box()
        self.undo_fit_button.setEnabled(False)

        self.function_browser = FunctionBrowser(self, False)
        self.function_browser_multi = FunctionBrowser(self, True)
        self.function_browser_multi.hide()
        self.function_browser_layout.addWidget(self.function_browser)
        self.function_browser_layout.addWidget(self.function_browser_multi)
        self.function_browser.setErrorsEnabled(True)
        self.function_browser_multi.setErrorsEnabled(True)

        self.increment_parameter_display_button.clicked.connect(self.increment_display_combo_box)
        self.decrement_parameter_display_button.clicked.connect(self.decrement_display_combo_box)

        self.select_workspaces_to_fit_button.setEnabled(False)

    def update_displayed_data_combo_box(self, data_list):
        self.parameter_display_combo.blockSignals(True)
        name = self.parameter_display_combo.currentText()
        self.parameter_display_combo.clear()

        self.parameter_display_combo.addItems(data_list)

        index = self.parameter_display_combo.findText(name)
        self.parameter_display_combo.blockSignals(False)

        if index != -1:
            self.parameter_display_combo.setCurrentIndex(index)
        else:
            self.parameter_display_combo.setCurrentIndex(0)

    def increment_display_combo_box(self):
        index = self.parameter_display_combo.currentIndex()
        count = self.parameter_display_combo.count()

        if index < count - 1:
            self.parameter_display_combo.setCurrentIndex(index + 1)
        else:
            self.parameter_display_combo.setCurrentIndex(0)

    def decrement_display_combo_box(self):
        index = self.parameter_display_combo.currentIndex()
        count = self.parameter_display_combo.count()

        if index != 0:
            self.parameter_display_combo.setCurrentIndex(index - 1)
        else:
            self.parameter_display_combo.setCurrentIndex(count - 1)

    def setup_simul_fit_combo_box(self):
        self.simul_fit_by_combo.addItem("Run")
        self.simul_fit_by_combo.addItem("Group/Pair")
        self.simul_fit_by_combo.addItem("Custom")
        self.simul_fit_by_specifier.setEnabled(False)

    def set_datasets_in_function_browser(self, data_set_name_list):
        number_of_data_sets = self.function_browser.getNumberOfDatasets()
        index_list = range(number_of_data_sets)
        self.function_browser.removeDatasets(index_list)
        self.function_browser.addDatasets(data_set_name_list)

    def set_datasets_in_function_browser_multi(self, data_set_name_list):
        number_of_data_sets = self.function_browser_multi.getNumberOfDatasets()
        index_list = range(number_of_data_sets)
        self.function_browser_multi.removeDatasets(index_list)
        self.function_browser_multi.addDatasets(data_set_name_list)

    def update_with_fit_outputs(self, fit_function, output_status, output_chi_squared):
        if not fit_function:
            self.fit_status_success_failure.setText('No Fit')
            self.fit_status_success_failure.setStyleSheet('color: black')
            self.fit_status_chi_squared.setText('Chi squared: {}'.format(output_chi_squared))
            return

        if self.fit_type != self.simultaneous_fit:
            self.function_browser.blockSignals(True)
            self.function_browser.updateMultiDatasetParameters(fit_function)
            self.function_browser.blockSignals(False)
        else:
            self.function_browser_multi.blockSignals(True)
            self.function_browser_multi.updateMultiDatasetParameters(fit_function)
            self.function_browser_multi.blockSignals(False)

        if output_status == 'success':
            self.fit_status_success_failure.setText('Success')
            self.fit_status_success_failure.setStyleSheet('color: green')
        elif output_status is None:
            self.fit_status_success_failure.setText('No Fit')
            self.fit_status_success_failure.setStyleSheet('color: black')
        else:
            self.fit_status_success_failure.setText('Failure: {}'.format(output_status))
            self.fit_status_success_failure.setStyleSheet('color: red')

        self.fit_status_chi_squared.setText('Chi squared: {:.4g}'.format(output_chi_squared))

    def update_global_fit_state(self, output_list):
        if self.fit_type == self.simultaneous_fit:
            indexed_fit = output_list[self.get_index_for_start_end_times()]
            boolean_list = [indexed_fit == 'success'] if indexed_fit else []
        else:
            boolean_list = [output == 'success' for output in output_list if output]

        if not boolean_list:
            self.global_fit_status_label.setText('No Fit')
            self.global_fit_status_label.setStyleSheet('color: black')
            return

        if all(boolean_list):
            self.global_fit_status_label.setText('Fit Successful')
            self.global_fit_status_label.setStyleSheet('color: green')
        else:
            self.global_fit_status_label.setText(
                '{} of {} fits failed'.format(len(boolean_list) - sum(boolean_list), len(boolean_list)))
            self.global_fit_status_label.setStyleSheet('color: red')

    def set_slot_for_select_workspaces_to_fit(self, slot):
        self.select_workspaces_to_fit_button.clicked.connect(slot)

    def set_slot_for_display_workspace_changed(self, slot):
        self.parameter_display_combo.currentIndexChanged.connect(slot)

    def set_slot_for_use_raw_changed(self, slot):
        self.fit_to_raw_data_checkbox.stateChanged.connect(slot)

    def set_slot_for_fit_type_changed(self, slot):
        self.simul_fit_checkbox.toggled.connect(slot)

    def set_slot_for_fit_button_clicked(self, slot):
        self.fit_button.clicked.connect(slot)

    def set_slot_for_start_x_updated(self, slot):
        self.time_start.editingFinished.connect(slot)

    def set_slot_for_end_x_updated(self, slot):
        self.time_end.editingFinished.connect(slot)

    def set_slot_for_simul_fit_by_changed(self, slot):
        self.simul_fit_by_combo.currentIndexChanged.connect(slot)

    def set_slot_for_simul_fit_specifier_changed(self, slot):
        self.simul_fit_by_specifier.currentIndexChanged.connect(slot)

    @property
    def display_workspace(self):
        return str(self.parameter_display_combo.currentText())

    @property
    def fit_object(self):
        if self.fit_type != self.simultaneous_fit:
            return self.function_browser.getGlobalFunction()
        else:
            return self.function_browser_multi.getGlobalFunction()

    @property
    def minimizer(self):
        return str(self.minimizer_combo.currentText())

    @property
    def start_time(self):
        return float(self.time_start.text())

    @start_time.setter
    def start_time(self, value):
        self.time_start.setText(str(value))

    @property
    def end_time(self):
        return float(self.time_end.text())

    @end_time.setter
    def end_time(self, value):
        self.time_end.setText(str(value))

    @property
    def evaluation_type(self):
        return str(self.evaluation_combo.currentText())

    @property
    def fit_type(self):
        if self.simul_fit_checkbox.isChecked():
            return self.simul_fit_checkbox.text()

    @property
    def simultaneous_fit(self):
        return self.simul_fit_checkbox.text()

    @property
    def simultaneous_fit_by(self):
        return self.simul_fit_by_combo.currentText()

    @property
    def simultaneous_fit_by_specifier(self):
        return self.simul_fit_by_specifier.currentText()

    @property
    def fit_to_raw(self):
        return self.fit_to_raw_data_checkbox.isChecked()

    @fit_to_raw.setter
    def fit_to_raw(self, value):
        state = QtCore.Qt.Checked if value else QtCore.Qt.Unchecked
        self.fit_to_raw_data_checkbox.setCheckState(state)

    @property
    def plot_guess(self):
        return self.plot_guess_checkbox.isChecked()

    @plot_guess.setter
    def plot_guess(self, value):
        state = QtCore.Qt.Checked if value else QtCore.Qt.Unchecked
        self.plot_guess_checkbox.setCheckState(state)

    @property
    def tf_asymmetry_mode(self):
        return self.tf_asymmetry_mode_checkbox.isChecked()

    @tf_asymmetry_mode.setter
    def tf_asymmetry_mode(self, value):
        state = QtCore.Qt.Checked if value else QtCore.Qt.Unchecked
        self.tf_asymmetry_mode_checkbox.setCheckState(state)

    @property
    def function_name(self):
        return str(self.function_name_line_edit.text())

    @function_name.setter
    def function_name(self, function_name):
        self.function_name_line_edit.blockSignals(True)
        self.function_name_line_edit.setText(function_name)
        self.function_name_line_edit.blockSignals(False)

    def warning_popup(self, message):
        warning(message, parent=self)

    def get_index_for_start_end_times(self):
        current_index = self.parameter_display_combo.currentIndex()
        return current_index if current_index != -1 else 0

    def get_global_parameters(self):
        return self.function_browser_multi.getGlobalParameters()

    def switch_to_simultaneous(self):
        self.function_browser_multi.show()
        self.function_browser.hide()

    def switch_to_single(self):
        self.function_browser_multi.hide()
        self.function_browser.show()

    def setup_fit_by_specifier(self, choices):
        self.simul_fit_by_specifier.blockSignals(True)
        self.simul_fit_by_specifier.clear()
        self.simul_fit_by_specifier.addItems(choices)
        self.simul_fit_by_specifier.blockSignals(False)
        self.simul_fit_by_specifier.currentIndexChanged.emit(0)

    def setup_fit_options_table(self):
        self.fit_options_table.setRowCount(6)
        self.fit_options_table.setColumnCount(2)
        self.fit_options_table.setColumnWidth(0, 300)
        self.fit_options_table.setColumnWidth(1, 300)
        self.fit_options_table.verticalHeader().setVisible(False)
        self.fit_options_table.horizontalHeader().setStretchLastSection(True)
        self.fit_options_table.setHorizontalHeaderLabels(
            ("Property;Value").split(";"))

        table_utils.setRowName(self.fit_options_table, 0, "Time Start")
        self.time_start = table_utils.addDoubleToTable(self.fit_options_table, 0.0, 0, 1)

        table_utils.setRowName(self.fit_options_table, 1, "Time End")
        self.time_end = table_utils.addDoubleToTable(self.fit_options_table, 15.0, 1, 1)

        table_utils.setRowName(self.fit_options_table, 2, "Minimizer")
        self.minimizer_combo = table_utils.addComboToTable(self.fit_options_table, 2, [])

        self.minimizer_combo.addItems(allowed_minimizers)

        table_utils.setRowName(self.fit_options_table, 3, "Fit To Raw Data")
        self.fit_to_raw_data_checkbox = table_utils.addCheckBoxWidgetToTable(
            self.fit_options_table, True, 3)

        table_utils.setRowName(self.fit_options_table, 4, "TF Asymmetry Mode")
        self.tf_asymmetry_mode_checkbox = table_utils.addCheckBoxWidgetToTable(
            self.fit_options_table, False, 4)

        table_utils.setRowName(self.fit_options_table, 5, "Evaluate Function As")
        self.evaluation_combo = table_utils.addComboToTable(self.fit_options_table, 5, ['CentrePoint', 'Histogram'])
