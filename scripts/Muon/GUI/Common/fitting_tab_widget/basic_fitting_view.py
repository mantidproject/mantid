# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui
from mantidqt.widgets.functionbrowser import FunctionBrowser

from Muon.GUI.Common.utilities import table_utils
from Muon.GUI.Common.message_box import warning

from qtpy import QtWidgets, QtCore

ui_basic_fitting, _ = load_ui(__file__, "basic_fitting.ui")

ALLOWED_MINIMIZERS = ["Levenberg-Marquardt", "BFGS", "Conjugate gradient (Fletcher-Reeves imp.)",
                      "Conjugate gradient (Polak-Ribiere imp.)", "Damped GaussNewton",
                      "Levenberg-MarquardtMD", "Simplex", "SteepestDescent", "Trust Region"]
DEFAULT_FREQUENCY_FIT_END_X = 250
FIT_START_TABLE_ROW = 0
FIT_END_TABLE_ROW = 1
MINIMIZER_TABLE_ROW = 2
RAW_DATA_TABLE_ROW = 3
EVALUATE_AS_TABLE_ROW = 4


class BasicFittingView(QtWidgets.QWidget, ui_basic_fitting):

    def __init__(self, parent=None, is_frequency_domain=False):
        super(BasicFittingView, self).__init__(parent)
        self.setupUi(self)

        self.time_start = None
        self.time_end = None
        self.minimizer_combo = None
        self.fit_to_raw_data_checkbox = None
        self.evaluation_combo = None

        self._setup_fit_options_table()

        self.undo_fit_button.setEnabled(False)

        self.function_browser = FunctionBrowser(self, True)
        self.function_browser_layout.addWidget(self.function_browser)
        self.function_browser.setErrorsEnabled(True)
        self.function_browser.hideGlobalCheckbox()

        if is_frequency_domain:
            self.fit_options_table.hideRow(RAW_DATA_TABLE_ROW)
            table_utils.setRowName(self.fit_options_table, FIT_START_TABLE_ROW, "Start X")
            table_utils.setRowName(self.fit_options_table, FIT_END_TABLE_ROW, "End X")
            self.end_time = DEFAULT_FREQUENCY_FIT_END_X

        # Comment out this line to show the 'Fit Generator' button
        self.fit_generator_button.hide()

    def set_datasets_in_function_browser(self, data_set_name_list):
        """Sets the datasets stored in the FunctionBrowser."""
        index_list = range(self.function_browser.getNumberOfDatasets())
        self.function_browser.removeDatasets(index_list)
        self.function_browser.addDatasets(data_set_name_list)

    def update_with_fit_outputs(self, fit_function, output_status, output_chi_squared):
        """Updates the view to show the status and results from a fit."""
        if not fit_function:
            self.clear_fit_status(output_chi_squared)
            return

        self.update_function_browser_parameters(False, fit_function)
        self.update_fit_status_labels(output_status, output_chi_squared)

    def update_fit_status_labels(self, output_status, output_chi_squared):
        """Updates the fit status labels."""
        if output_status == "success":
            self.fit_status_success_failure.setText("Success")
            self.fit_status_success_failure.setStyleSheet("color: green")
        elif output_status is None:
            self.fit_status_success_failure.setText("No Fit")
            self.fit_status_success_failure.setStyleSheet("color: black")
        else:
            self.fit_status_success_failure.setText(f"Failure: {output_status}")
            self.fit_status_success_failure.setStyleSheet("color: red")
        self.fit_status_chi_squared.setText(f"Chi squared: {output_chi_squared:.4g}")

    def update_function_browser_parameters(self, is_simultaneous_fit, fit_function):
        """Updates the parameters in the function browser."""
        if is_simultaneous_fit:
            self.function_browser.blockSignals(True)
            self.function_browser.updateMultiDatasetParameters(fit_function)
            self.function_browser.blockSignals(False)
        else:
            self.function_browser.blockSignals(True)
            self.function_browser.updateParameters(fit_function)
            self.function_browser.blockSignals(False)
        self.function_browser.setErrorsEnabled(True)

    def clear_fit_status(self, output_chi_squared):
        """Clears the fit status label."""
        self.fit_status_success_failure.setText("No Fit")
        self.fit_status_success_failure.setStyleSheet("color: black")
        self.fit_status_chi_squared.setText(f"Chi squared: {output_chi_squared}")

    def update_global_fit_state(self, output_list):
        """Updates the global fit status label."""
        self.update_global_fit_status_label([output == "success" for output in output_list if output])

    def update_global_fit_status_label(self, success_list):
        """Updates the global fit status label."""
        if not success_list:
            self.global_fit_status_label.setText("No Fit")
            self.global_fit_status_label.setStyleSheet("color: black")
            return

        if all(success_list):
            self.global_fit_status_label.setText("Fit Successful")
            self.global_fit_status_label.setStyleSheet("color: green")
        else:
            self.global_fit_status_label.setText(
                f"{len(success_list) - sum(success_list)} of {len(success_list)} fits failed")
            self.global_fit_status_label.setStyleSheet("color: red")

    # def set_slot_for_fit_generator_clicked(self, slot):
    #     self.fit_generator_button.clicked.connect(slot)
    #
    # def set_slot_for_display_workspace_changed(self, slot):
    #     self.parameter_display_combo.currentIndexChanged.connect(slot)
    #
    # def set_slot_for_use_raw_changed(self, slot):
    #     self.fit_to_raw_data_checkbox.stateChanged.connect(slot)
    #
    # def set_slot_for_fit_type_changed(self, slot):
    #     self.simul_fit_checkbox.toggled.connect(slot)
    #
    # def set_slot_for_fit_button_clicked(self, slot):
    #     self.fit_button.clicked.connect(slot)
    #
    # def set_slot_for_start_x_updated(self, slot):
    #     self.time_start.editingFinished.connect(slot)
    #
    # def set_slot_for_end_x_updated(self, slot):
    #     self.time_end.editingFinished.connect(slot)
    #
    # def set_slot_for_simul_fit_by_changed(self, slot):
    #     self.simul_fit_by_combo.currentIndexChanged.connect(slot)
    #
    # def set_slot_for_simul_fit_specifier_changed(self, slot):
    #     self.simul_fit_by_specifier.currentIndexChanged.connect(slot)
    #
    # def set_slot_for_minimiser_changed(self, slot):
    #     self.minimizer_combo.currentIndexChanged.connect(slot)
    #
    # def set_slot_for_evaluation_type_changed(self, slot):
    #     self.evaluation_combo.currentIndexChanged.connect(slot)

    @property
    def fit_object(self):
        """Returns the global fitting function."""
        return self.function_browser.getGlobalFunction()

    @property
    def minimizer(self):
        """Returns the selected minimizer."""
        return str(self.minimizer_combo.currentText())

    @property
    def start_time(self):
        """Returns the selected start X."""
        return float(self.time_start.text())

    @start_time.setter
    def start_time(self, value):
        """Sets the selected start X."""
        self.time_start.setText(str(value))

    @property
    def end_time(self):
        """Returns the selected end X."""
        return float(self.time_end.text())

    @end_time.setter
    def end_time(self, value):
        """Sets the selected end X."""
        self.time_end.setText(str(value))

    @property
    def evaluation_type(self):
        """Returns the selected evaluation type."""
        return str(self.evaluation_combo.currentText())

    @property
    def fit_to_raw(self):
        """Returns whether or not fitting to raw data is ticked."""
        return self.fit_to_raw_data_checkbox.isChecked()

    @fit_to_raw.setter
    def fit_to_raw(self, value):
        """Sets whether or not you are fitting to raw data."""
        self.fit_to_raw_data_checkbox.setCheckState(QtCore.Qt.Checked if value else QtCore.Qt.Unchecked)

    @property
    def plot_guess(self):
        """Returns true if plot guess is ticked."""
        return self.plot_guess_checkbox.isChecked()

    @plot_guess.setter
    def plot_guess(self, value):
        """Sets whether or not plot guess is ticked."""
        self.plot_guess_checkbox.setCheckState(QtCore.Qt.Checked if value else QtCore.Qt.Unchecked)

    @property
    def function_name(self):
        """Returns the function name being used."""
        return str(self.function_name_line_edit.text())

    @function_name.setter
    def function_name(self, function_name):
        """Sets the function name being used."""
        self.function_name_line_edit.blockSignals(True)
        self.function_name_line_edit.setText(function_name)
        self.function_name_line_edit.blockSignals(False)

    def warning_popup(self, message):
        """Displays a warning message."""
        warning(message, parent=self)

    def get_global_parameters(self):
        """Returns a list of global parameters."""
        return self.function_browser.getGlobalParameters()

    def switch_to_simultaneous(self):
        """Switches the view to simultaneous mode."""
        self.function_browser.showGlobalCheckbox()

    def switch_to_single(self):
        """Switches the view to single mode."""
        self.function_browser.hideGlobalCheckbox()

    def _setup_fit_options_table(self):
        """Setup the fit options table with the appropriate options."""
        self.fit_options_table.setRowCount(5)
        self.fit_options_table.setColumnCount(2)
        self.fit_options_table.setColumnWidth(0, 150)
        self.fit_options_table.setColumnWidth(1, 300)
        self.fit_options_table.verticalHeader().setVisible(False)
        self.fit_options_table.horizontalHeader().setStretchLastSection(True)
        self.fit_options_table.setHorizontalHeaderLabels(["Property", "Value"])

        table_utils.setRowName(self.fit_options_table, FIT_START_TABLE_ROW, "Time Start")
        self.time_start = table_utils.addDoubleToTable(self.fit_options_table, 0.0, FIT_START_TABLE_ROW, 1)

        table_utils.setRowName(self.fit_options_table, FIT_END_TABLE_ROW, "Time End")
        self.time_end = table_utils.addDoubleToTable(self.fit_options_table, 15.0, FIT_END_TABLE_ROW, 1)

        table_utils.setRowName(self.fit_options_table, MINIMIZER_TABLE_ROW, "Minimizer")
        self.minimizer_combo = table_utils.addComboToTable(self.fit_options_table, MINIMIZER_TABLE_ROW, [])
        self.minimizer_combo.addItems(ALLOWED_MINIMIZERS)

        table_utils.setRowName(self.fit_options_table, RAW_DATA_TABLE_ROW, "Fit To Raw Data")
        self.fit_to_raw_data_checkbox = table_utils.addCheckBoxWidgetToTable(
            self.fit_options_table, True, RAW_DATA_TABLE_ROW)

        table_utils.setRowName(self.fit_options_table, EVALUATE_AS_TABLE_ROW, "Evaluate Function As")
        self.evaluation_combo = table_utils.addComboToTable(self.fit_options_table, EVALUATE_AS_TABLE_ROW,
                                                            ['CentrePoint', 'Histogram'])
