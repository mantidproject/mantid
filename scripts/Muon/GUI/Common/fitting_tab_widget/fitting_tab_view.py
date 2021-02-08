# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
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
FIT_START_TABLE_ROW = 0
FIT_END_TABLE_ROW = 1
RAW_DATA_TABLE_ROW = 3
TF_ASYMMETRY_MODE_TABLE_ROW = 4
DEFAULT_FREQUENCY_FIT_END_X = 250


class FittingTabView(QtWidgets.QWidget, ui_fitting_tab):
    def __init__(self, simultaneous_item_list, is_frequency_domain=False, parent=None):
        super(FittingTabView, self).__init__(parent)
        self.setupUi(self)
        self.setup_fit_options_table()
        self.setup_simul_fit_combo_box(simultaneous_item_list)
        self.undo_fit_button.setEnabled(False)

        self.function_browser = FunctionBrowser(self, True)
        self.function_browser_layout.addWidget(self.function_browser)
        self.function_browser.setErrorsEnabled(True)
        self.function_browser.hideGlobalCheckbox()

        self.increment_parameter_display_button.clicked.connect(self.increment_display_combo_box)
        self.decrement_parameter_display_button.clicked.connect(self.decrement_display_combo_box)

        self.disable_simul_fit_options()

        if is_frequency_domain:
            self.hide_simultaneous_fit_options()
            self.fit_options_table.hideRow(RAW_DATA_TABLE_ROW)
            self.fit_options_table.hideRow(TF_ASYMMETRY_MODE_TABLE_ROW)
            table_utils.setRowName(self.fit_options_table, FIT_START_TABLE_ROW, "Start X")
            table_utils.setRowName(self.fit_options_table, FIT_END_TABLE_ROW, "End X")
            self.end_time = DEFAULT_FREQUENCY_FIT_END_X

        # Comment out this line to show the 'Fit Generator' button
        self.fit_generator_button.hide()

    def update_displayed_data_combo_box(self, data_list):
        self.parameter_display_combo.blockSignals(True)
        name = self.parameter_display_combo.currentText()
        self.parameter_display_combo.clear()
        self.parameter_display_combo.addItems(data_list)

        index = self.parameter_display_combo.findText(name)

        if index != -1:
            self.parameter_display_combo.setCurrentIndex(index)
        else:
            self.parameter_display_combo.setCurrentIndex(0)

        self.parameter_display_combo.blockSignals(False)

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

    def setup_simul_fit_combo_box(self, item_list):
        for item in item_list:
            self.simul_fit_by_combo.addItem(item)

    def set_datasets_in_function_browser(self, data_set_name_list):
        number_of_data_sets = self.function_browser.getNumberOfDatasets()
        index_list = range(number_of_data_sets)
        self.function_browser.removeDatasets(index_list)
        self.function_browser.addDatasets(data_set_name_list)

    def update_with_fit_outputs(self, fit_function, output_status, output_chi_squared):
        if not fit_function:
            self.fit_status_success_failure.setText('No Fit')
            self.fit_status_success_failure.setStyleSheet('color: black')
            self.fit_status_chi_squared.setText('Chi squared: {}'.format(output_chi_squared))
            return

        if self.is_simul_fit:
            self.function_browser.blockSignals(True)
            self.function_browser.updateMultiDatasetParameters(fit_function)
            self.function_browser.blockSignals(False)
        else:
            self.function_browser.blockSignals(True)
            self.function_browser.updateParameters(fit_function)
            self.function_browser.blockSignals(False)

        if output_status == 'success':
            self.fit_status_success_failure.setText('Success')
            self.fit_status_success_failure.setStyleSheet('color: green')
        elif output_status is None:
            self.fit_status_success_failure.setText('No Fit')
            self.fit_status_success_failure.setStyleSheet('color: black')
        else:
            self.fit_status_success_failure.setText('Failure: {}'.format(output_status))
            self.fit_status_success_failure.setStyleSheet('color: red')
        self.function_browser.setErrorsEnabled(True)
        self.fit_status_chi_squared.setText('Chi squared: {:.4g}'.format(output_chi_squared))

    def update_global_fit_state(self, output_list, index):
        if self.fit_type == self.simultaneous_fit:
            indexed_fit = output_list[index]
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

    def set_slot_for_fit_generator_clicked(self, slot):
        self.fit_generator_button.clicked.connect(slot)

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

    def set_slot_for_minimiser_changed(self, slot):
        self.minimizer_combo.currentIndexChanged.connect(slot)

    def set_slot_for_evaluation_type_changed(self, slot):
        self.evaluation_combo.currentIndexChanged.connect(slot)

    @property
    def display_workspace(self):
        return str(self.parameter_display_combo.currentText())

    @property
    def loaded_workspaces(self):
        return [self.parameter_display_combo.itemText(i) for i in range(self.parameter_display_combo.count())]

    @display_workspace.setter
    def display_workspace(self, value):
        self.parameter_display_combo.blockSignals(True)
        index = self.parameter_display_combo.findText(value)
        self.parameter_display_combo.setCurrentIndex(index)
        self.parameter_display_combo.blockSignals(False)

    @property
    def fit_object(self):
        return self.function_browser.getGlobalFunction()

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

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, fit_by_text):
        index = self.simul_fit_by_combo.findText(fit_by_text)
        if index != -1:
            self.simul_fit_by_combo.blockSignals(True)
            self.simul_fit_by_combo.setCurrentIndex(index)
            self.simul_fit_by_combo.blockSignals(False)

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
        return self.function_browser.getGlobalParameters()

    def switch_to_simultaneous(self):
        self.function_browser.showGlobalCheckbox()

    def switch_to_single(self):
        self.function_browser.hideGlobalCheckbox()

    def disable_simul_fit_options(self):
        self.simul_fit_by_combo.setEnabled(False)
        self.simul_fit_by_specifier.setEnabled(False)

    def hide_simultaneous_fit_options(self):
        self.simul_fit_checkbox.hide()
        self.simul_fit_by_combo.hide()
        self.simul_fit_by_specifier.hide()

    def enable_simul_fit_options(self):
        self.simul_fit_by_combo.setEnabled(True)
        self.simul_fit_by_specifier.setEnabled(True)

    @property
    def is_simul_fit(self):
        return self.simul_fit_checkbox.isChecked()

    @is_simul_fit.setter
    def is_simul_fit(self, simultaneous):
        self.simul_fit_checkbox.blockSignals(True)
        self.simul_fit_checkbox.setChecked(simultaneous)
        self.simul_fit_checkbox.blockSignals(False)

    def setup_fit_by_specifier(self, choices):
        self.simul_fit_by_specifier.blockSignals(True)
        self.simul_fit_by_specifier.clear()
        self.simul_fit_by_specifier.addItems(choices)
        self.simul_fit_by_specifier.blockSignals(False)
        self.simul_fit_by_specifier.currentIndexChanged.emit(0)

    def setup_fit_options_table(self):
        self.fit_options_table.setRowCount(6)
        self.fit_options_table.setColumnCount(2)
        self.fit_options_table.setColumnWidth(0, 150)
        self.fit_options_table.setColumnWidth(1, 300)
        self.fit_options_table.verticalHeader().setVisible(False)
        self.fit_options_table.horizontalHeader().setStretchLastSection(True)
        self.fit_options_table.setHorizontalHeaderLabels(
            ("Property;Value").split(";"))

        table_utils.setRowName(self.fit_options_table, FIT_START_TABLE_ROW, "Time Start")
        self.time_start = table_utils.addDoubleToTable(self.fit_options_table, 0.0, FIT_START_TABLE_ROW, 1)

        table_utils.setRowName(self.fit_options_table, FIT_END_TABLE_ROW, "Time End")
        self.time_end = table_utils.addDoubleToTable(self.fit_options_table, 15.0, FIT_END_TABLE_ROW, 1)

        table_utils.setRowName(self.fit_options_table, 2, "Minimizer")
        self.minimizer_combo = table_utils.addComboToTable(self.fit_options_table, 2, [])
        self.minimizer_combo.addItems(allowed_minimizers)

        table_utils.setRowName(self.fit_options_table, RAW_DATA_TABLE_ROW, "Fit To Raw Data")
        self.fit_to_raw_data_checkbox = table_utils.addCheckBoxWidgetToTable(
            self.fit_options_table, True, RAW_DATA_TABLE_ROW)

        table_utils.setRowName(self.fit_options_table, TF_ASYMMETRY_MODE_TABLE_ROW, "TF Asymmetry Mode")
        self.tf_asymmetry_mode_checkbox = table_utils.addCheckBoxWidgetToTable(
            self.fit_options_table, False, TF_ASYMMETRY_MODE_TABLE_ROW)

        table_utils.setRowName(self.fit_options_table, 5, "Evaluate Function As")
        self.evaluation_combo = table_utils.addComboToTable(self.fit_options_table, 5, ['CentrePoint', 'Histogram'])
