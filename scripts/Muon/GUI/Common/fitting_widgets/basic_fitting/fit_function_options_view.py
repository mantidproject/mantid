# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction
from mantidqt.utils.qt import load_ui
from mantidqt.widgets.functionbrowser import FunctionBrowser

from Muon.GUI.Common.utilities import table_utils

from qtpy.QtWidgets import QWidget

ui_form, base_widget = load_ui(__file__, "fit_function_options.ui")

ALLOWED_MINIMIZERS = ["Levenberg-Marquardt", "BFGS", "Conjugate gradient (Fletcher-Reeves imp.)",
                      "Conjugate gradient (Polak-Ribiere imp.)", "Damped GaussNewton",
                      "Levenberg-MarquardtMD", "Simplex", "SteepestDescent", "Trust Region"]
START_X_TABLE_ROW = 0
END_X_TABLE_ROW = 1
MINIMIZER_TABLE_ROW = 2
RAW_DATA_TABLE_ROW = 3
EVALUATE_AS_TABLE_ROW = 4


class FitFunctionOptionsView(ui_form, base_widget):
    """
    The FitFunctionOptionsView includes the Function Name line edit, FunctionBrowser and the fitting options table
    widget. It also holds the Fit Status and Chi Squared labels.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the FitFunctionOptionsView and sets up the fit options table and FunctionBrowser."""
        super(FitFunctionOptionsView, self).__init__(parent)
        self.setupUi(self)

        self.start_x_line_edit = None
        self.start_x_validator = None
        self.end_x_line_edit = None
        self.end_x_validator = None
        self.minimizer_combo = None
        self.fit_to_raw_data_checkbox = None
        self.evaluation_combo = None

        self._setup_fit_options_table()

        self.function_browser = FunctionBrowser(self, True)
        self.function_browser_layout.addWidget(self.function_browser)
        self.function_browser.setErrorsEnabled(True)
        self.function_browser.hideGlobalCheckbox()
        self.function_browser.setStretchLastColumn(True)

    def set_slot_for_fit_name_changed(self, slot) -> None:
        """Connect the slot for the fit name being changed by the user."""
        self.function_name_line_edit.textChanged.connect(slot)

    def set_slot_for_function_structure_changed(self, slot) -> None:
        """Connect the slot for the function structure changing."""
        self.function_browser.functionStructureChanged.connect(slot)

    def set_slot_for_function_parameter_changed(self, slot) -> None:
        """Connect the slot for a function parameter changing."""
        self.function_browser.parameterChanged.connect(slot)

    def set_slot_for_start_x_updated(self, slot) -> None:
        """Connect the slot for the start x option."""
        self.start_x_line_edit.editingFinished.connect(slot)

    def set_slot_for_end_x_updated(self, slot) -> None:
        """Connect the slot for the end x option."""
        self.end_x_line_edit.editingFinished.connect(slot)

    def set_slot_for_minimizer_changed(self, slot) -> None:
        """Connect the slot for changing the Minimizer."""
        self.minimizer_combo.currentIndexChanged.connect(slot)

    def set_slot_for_evaluation_type_changed(self, slot) -> None:
        """Connect the slot for changing the Evaluation type."""
        self.evaluation_combo.currentIndexChanged.connect(slot)

    def set_slot_for_use_raw_changed(self, slot) -> None:
        """Connect the slot for the Use raw option."""
        self.fit_to_raw_data_checkbox.stateChanged.connect(slot)

    def update_fit_status_labels(self, fit_status: str, chi_squared: float) -> None:
        """Updates the fit status and chi squared label."""
        if fit_status == "success":
            self.fit_status_success_failure.setText("Success")
            self.fit_status_success_failure.setStyleSheet("color: green")
        elif fit_status is None:
            self.fit_status_success_failure.setText("No Fit")
            self.fit_status_success_failure.setStyleSheet("color: black")
        else:
            self.fit_status_success_failure.setText(f"Failure: {fit_status}")
            self.fit_status_success_failure.setStyleSheet("color: red")
        self.fit_status_chi_squared.setText(f"Chi squared: {chi_squared:.4g}")

    def clear_fit_status(self) -> None:
        """Clears the fit status and chi squared label."""
        self.fit_status_success_failure.setText("No Fit")
        self.fit_status_success_failure.setStyleSheet("color: black")
        self.fit_status_chi_squared.setText(f"Chi squared: 0.0")

    def set_datasets_in_function_browser(self, dataset_names: list) -> None:
        """Sets the datasets stored in the FunctionBrowser."""
        index_list = range(self.function_browser.getNumberOfDatasets())
        self.function_browser.removeDatasets(index_list)
        self.function_browser.addDatasets(dataset_names)

    def set_current_dataset_index(self, dataset_index: int) -> None:
        """Sets the index of the current dataset."""
        self.function_browser.setCurrentDataset(dataset_index)

    def set_fit_function(self, fit_function: IFunction) -> None:
        """Set the fit function shown in the view."""
        self.function_browser.blockSignals(True)
        if fit_function is None:
            self.function_browser.setFunction("")
        else:
            self.function_browser.setFunction(str(fit_function))
            # Required to update the parameter errors as they are not stored in the function string
            self.function_browser.updateParameters(fit_function)
        self.function_browser.blockSignals(False)
        self.function_browser.setErrorsEnabled(True)

    def update_function_browser_parameters(self, is_simultaneous_fit: bool, fit_function: IFunction,
                                           global_parameters: list = []) -> None:
        """Updates the parameters in the function browser."""
        self.function_browser.blockSignals(True)

        if fit_function is None:
            self.function_browser.setFunction("")
        elif is_simultaneous_fit:
            self.function_browser.updateMultiDatasetParameters(fit_function.clone())
            self.global_parameters = global_parameters
        else:
            self.function_browser.updateParameters(fit_function)

        self.function_browser.blockSignals(False)
        self.function_browser.setErrorsEnabled(True)

    @property
    def fit_object(self) -> IFunction:
        """Returns the global fitting function."""
        return self.function_browser.getGlobalFunction()

    def current_fit_function(self) -> IFunction:
        """Returns the current fitting function in the view."""
        return self.function_browser.getFunction()

    @property
    def minimizer(self) -> str:
        """Returns the selected minimizer."""
        return str(self.minimizer_combo.currentText())

    @property
    def start_x(self) -> float:
        """Returns the selected start X."""
        return float(self.start_x_line_edit.text())

    @start_x.setter
    def start_x(self, value: float) -> None:
        """Sets the selected start X."""
        self.start_x_validator.last_valid_value = f"{value:.3f}"
        self.start_x_line_edit.setText(f"{value:.3f}")

    @property
    def end_x(self) -> float:
        """Returns the selected end X."""
        return float(self.end_x_line_edit.text())

    @end_x.setter
    def end_x(self, value: float) -> None:
        """Sets the selected end X."""
        self.end_x_validator.last_valid_value = f"{value:.3f}"
        self.end_x_line_edit.setText(f"{value:.3f}")

    @property
    def evaluation_type(self) -> str:
        """Returns the selected evaluation type."""
        return str(self.evaluation_combo.currentText())

    @property
    def fit_to_raw(self) -> bool:
        """Returns whether or not fitting to raw data is ticked."""
        return self.fit_to_raw_data_checkbox.isChecked()

    @fit_to_raw.setter
    def fit_to_raw(self, check: bool) -> None:
        """Sets whether or not you are fitting to raw data."""
        self.fit_to_raw_data_checkbox.setChecked(check)

    @property
    def function_name(self) -> str:
        """Returns the function name being used."""
        return str(self.function_name_line_edit.text())

    @function_name.setter
    def function_name(self, function_name: str) -> None:
        """Sets the function name being used."""
        self.function_name_line_edit.blockSignals(True)
        self.function_name_line_edit.setText(function_name)
        self.function_name_line_edit.blockSignals(False)

    def number_of_datasets(self) -> int:
        """Returns the number of domains in the FunctionBrowser."""
        return self.function_browser.getNumberOfDatasets()

    @property
    def global_parameters(self) -> list:
        """Returns a list of global parameters."""
        return self.function_browser.getGlobalParameters()

    @global_parameters.setter
    def global_parameters(self, global_parameters: list) -> None:
        """Sets the global parameters in the function browser."""
        self.function_browser.setGlobalParameters(global_parameters)

    def parameter_value(self, full_parameter: str) -> float:
        """Returns the value of the specified parameter."""
        return self.function_browser.getParameter(full_parameter)

    def switch_to_simultaneous(self) -> None:
        """Switches the view to simultaneous mode."""
        self.function_browser.showGlobalCheckbox()
        self.function_browser.setGlobalParameters([])

    def switch_to_single(self) -> None:
        """Switches the view to single mode."""
        self.function_browser.hideGlobalCheckbox()
        self.function_browser.setGlobalParameters([])

    def hide_fit_raw_checkbox(self) -> None:
        """Hides the Fit Raw checkbox in the fitting options."""
        self.fit_options_table.hideRow(RAW_DATA_TABLE_ROW)

    def set_start_and_end_x_labels(self, start_x_label: str, end_x_label: str) -> None:
        """Sets the labels to use for the start and end X labels in the fit options table."""
        table_utils.setRowName(self.fit_options_table, START_X_TABLE_ROW, start_x_label)
        table_utils.setRowName(self.fit_options_table, END_X_TABLE_ROW, end_x_label)

    def _setup_fit_options_table(self) -> None:
        """Setup the fit options table with the appropriate options."""
        self.fit_options_table.setRowCount(5)
        self.fit_options_table.setColumnCount(2)
        self.fit_options_table.setColumnWidth(0, 150)
        self.fit_options_table.verticalHeader().setVisible(False)
        self.fit_options_table.horizontalHeader().setStretchLastSection(True)
        self.fit_options_table.setHorizontalHeaderLabels(["Property", "Value"])

        table_utils.setRowName(self.fit_options_table, START_X_TABLE_ROW, "Start X")
        self.start_x_line_edit, self.start_x_validator = table_utils.addDoubleToTable(self.fit_options_table, 0.0,
                                                                                      START_X_TABLE_ROW, 1)

        table_utils.setRowName(self.fit_options_table, END_X_TABLE_ROW, "End X")
        self.end_x_line_edit, self.end_x_validator = table_utils.addDoubleToTable(self.fit_options_table, 15.0,
                                                                                  END_X_TABLE_ROW, 1)

        table_utils.setRowName(self.fit_options_table, MINIMIZER_TABLE_ROW, "Minimizer")
        self.minimizer_combo = table_utils.addComboToTable(self.fit_options_table, MINIMIZER_TABLE_ROW, [])
        self.minimizer_combo.addItems(ALLOWED_MINIMIZERS)

        table_utils.setRowName(self.fit_options_table, RAW_DATA_TABLE_ROW, "Fit To Raw Data")
        self.fit_to_raw_data_checkbox = table_utils.addCheckBoxWidgetToTable(
            self.fit_options_table, True, RAW_DATA_TABLE_ROW)

        table_utils.setRowName(self.fit_options_table, EVALUATE_AS_TABLE_ROW, "Evaluate Function As")
        self.evaluation_combo = table_utils.addComboToTable(self.fit_options_table, EVALUATE_AS_TABLE_ROW,
                                                            ['CentrePoint', 'Histogram'])
