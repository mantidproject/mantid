# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from typing import Union

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialogButtonBox, QMessageBox

from mantid.kernel import logger
from mantid.api import MatrixWorkspace

from mantidqt.icons import get_icon
from mantidqt.utils.qt import load_ui

# Constants
RANGE_SPECIFIER = "-"
PLACEHOLDER_FORMAT = "valid range: {}" + RANGE_SPECIFIER + "{}"
PLACEHOLDER_FORMAT_SINGLE_INPUT = "valid range: {}"
RED_ASTERISK = None

WORKSPACE_NAME = "Workspace name"
# A title to refer to the workspace, but as a number for use on an axis label
WORKSPACE_REFERENCE_NUMBER = "Workspace"
CUSTOM = "Custom"
CONTOUR = "Contour"
SURFACE = "Surface"


def red_asterisk():
    global RED_ASTERISK
    if RED_ASTERISK is None:
        RED_ASTERISK = get_icon("mdi.asterisk", "red", 0.6)
    return RED_ASTERISK


SpectraSelectionDialogUI, SpectraSelectionDialogUIBase = load_ui(__file__, "spectraselectordialog.ui")


class SpectraSelection(object):
    Individual = 0
    Waterfall = 1
    Tiled = 2
    Surface = 3
    Contour = 4

    def __init__(self, workspaces):
        self.workspaces = workspaces
        self.wksp_indices = None
        self.spectra = None
        self.plot_type = SpectraSelection.Individual

        self.errors = False
        self.log_name = None
        self.custom_log_values = None
        self.axis_name = None


class SpectraSelectionDialog(SpectraSelectionDialogUIBase):
    @staticmethod
    def get_compatible_workspaces(workspaces):
        matrix_workspaces = []
        for ws in workspaces:
            if isinstance(ws, MatrixWorkspace):
                matrix_workspaces.append(ws)
            else:
                # Log an error but carry on so valid workspaces can be plotted.
                logger.warning("{}: ** Expected MatrixWorkspace, found {}".format(ws.name(), ws.__class__.__name__))

        return matrix_workspaces

    def __init__(self, workspaces, parent=None, show_colorfill_btn=False, overplot=False, advanced=False):
        super(SpectraSelectionDialog, self).__init__(parent)
        self.icon = self.setWindowIcon(QIcon(":/images/MantidIcon.ico"))
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        workspaces = self.get_compatible_workspaces(workspaces)

        # attributes
        self._workspaces = workspaces
        self.wi_min, self.wi_max = None, None
        self.selection = None
        self._ui = None
        self._show_colorfill_button = show_colorfill_btn
        self._overplot = overplot
        self._plottable_spectra = None
        self._advanced = advanced

        # This is used as a flag to workaround the case in which the error bars checkbox is set before a selection is
        # instantiated, causing it to have no effect. The update is then done in the parse_wksp / parse_spec functions
        self._errorsset = False

        self._init_ui()
        self._set_placeholder_text()
        self._setup_connections()
        self._on_specnums_changed()
        self._on_wkspindices_changed()

    # Check if workspace only has a single spectra, if true set it as first valid spectrum number
    def update_spectrum_number_text(self):
        if self.selection and (
            (self.selection.wksp_indices and len(self.selection.wksp_indices) > 0)
            or (self.selection.spectra and len(self.selection.spectra) > 0)
        ):
            return

        if not self._ui.specNums.text():
            for ws in self._workspaces:
                if ws.getNumberHistograms() == 1:
                    spectrum_number = ws.getSpectrum(0).getSpectrumNo()
                    self._ui.specNums.setText(str(spectrum_number))
                    self._parse_spec_nums()
                    break

    def on_ok_clicked(self):
        self.update_spectrum_number_text()

        if self.selection is None:
            QMessageBox.warning(self, "Invalid Input", "Please enter a valid workspace index or spectrum number.")
            return

        if self._check_number_of_plots(self.selection):
            self.accept()

    def on_plot_all_clicked(self):
        self.update_spectrum_number_text()

        selection = SpectraSelection(self._workspaces)
        selection.spectra = self._plottable_spectra
        selection.plot_type = self._ui.plotType.currentIndex()

        if self._advanced:
            selection.errors = self._ui.advanced_options_widget.ui.error_bars_check_box.isChecked()
            selection.log_name = self._ui.advanced_options_widget.ui.log_value_combo_box.currentText()
            selection.axis_name = self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.text()

            if selection.log_name == CUSTOM:
                custom_log_text = self._ui.advanced_options_widget.ui.custom_log_line_edit.text()
                if self._ui.advanced_options_widget._validate_custom_logs(custom_log_text, plot_all=True):
                    selection.custom_log_values = [float(value) for value in custom_log_text.split(",")]
                else:
                    return

        if self._check_number_of_plots(selection):
            self.selection = selection
            self.accept()

    def on_colorfill_clicked(self):
        self.selection = "colorfill"
        self.accept()

    # ------------------- Private -------------------------
    def _check_number_of_plots(self, selection):
        index_length = len(selection.wksp_indices) if selection.wksp_indices else len(selection.spectra)
        number_of_lines_to_plot = len(selection.workspaces) * index_length
        if selection.plot_type == SpectraSelection.Tiled and number_of_lines_to_plot > 12:
            response = QMessageBox.warning(
                self,
                "Mantid Workbench",
                "Are you sure you want to plot {} subplots?".format(number_of_lines_to_plot),
                QMessageBox.Ok | QMessageBox.Cancel,
            )
            return response == QMessageBox.Ok
        elif selection.plot_type != SpectraSelection.Tiled and number_of_lines_to_plot > 10:
            response = QMessageBox.warning(
                self,
                "Mantid Workbench",
                "You selected {} spectra to plot. Are you sure " "you want to plot this many?".format(number_of_lines_to_plot),
                QMessageBox.Ok | QMessageBox.Cancel,
            )
            return response == QMessageBox.Ok

        return True

    def _init_ui(self):
        ui = SpectraSelectionDialogUI()
        ui.setupUi(self)

        if self._advanced:
            ui.advanced_options_widget = AdvancedPlottingOptionsWidget(parent=self)
            ui.layout.replaceWidget(ui.advanced_plots_dummy_widget, ui.advanced_options_widget)
            if len(self._workspaces) > 2:
                ui.plotType.addItem(SURFACE)
                ui.plotType.addItem(CONTOUR)
            self.setWindowTitle("Plot Advanced")

        self._ui = ui
        ui.colorfillButton.setVisible(self._show_colorfill_button)
        # overwrite the "Yes to All" button text
        ui.buttonBox.button(QDialogButtonBox.YesToAll).setText("Plot All")
        # ok disabled by default
        ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        # validity markers
        ui.wkspIndicesValid.setIcon(red_asterisk())
        ui.specNumsValid.setIcon(red_asterisk())

    def _set_placeholder_text(self):
        """Sets placeholder text to indicate the ranges possible"""
        workspaces = self._workspaces
        # workspace index range
        wi_max = min([ws.getNumberHistograms() - 1 for ws in workspaces])
        self._ui.wkspIndices.setPlaceholderText(PLACEHOLDER_FORMAT.format(0, wi_max))
        self.wi_min, self.wi_max = 0, wi_max

        # spectra range
        ws_spectra = [{ws.getSpectrum(i).getSpectrumNo() for i in range(ws.getNumberHistograms())} for ws in workspaces]
        plottable = ws_spectra[0]
        if len(ws_spectra) > 1:
            for sp_set in ws_spectra[1:]:
                plottable = plottable.intersection(sp_set)
        plottable = sorted(plottable)
        if len(plottable) == 0:
            raise Exception("Error: Workspaces have no common spectra.")
        # store the plottable list for use later
        self._plottable_spectra = plottable
        self._ui.specNums.setPlaceholderText(PLACEHOLDER_FORMAT_SINGLE_INPUT.format(self._list_to_ranges(plottable)))

    def _list_to_ranges(self, input):
        ranges = []
        first = last = None  # first and last number of current consecutive range
        for item in sorted(input):
            if first is None:
                first = last = item  # bootstrap
            elif item == last + 1:  # consecutive
                last = item  # extend the range
            else:  # not consecutive
                ranges.append("{0}{1}{2}".format(first, RANGE_SPECIFIER, last) if last > first else "{0}".format(first))
                first = last = item
        # the last range ended by iteration end
        ranges.append("{0}{1}{2}".format(first, RANGE_SPECIFIER, last) if last > first else "{0}".format(first))
        return ", ".join(ranges)

    def _setup_connections(self):
        ui = self._ui
        # button actions
        ui.buttonBox.button(QDialogButtonBox.Ok).clicked.connect(self.on_ok_clicked)
        ui.buttonBox.button(QDialogButtonBox.Cancel).clicked.connect(self.reject)
        ui.buttonBox.button(QDialogButtonBox.YesToAll).clicked.connect(self.on_plot_all_clicked)
        ui.colorfillButton.clicked.connect(self.on_colorfill_clicked)

        # line edits are mutually exclusive
        ui.wkspIndices.textChanged.connect(self._on_wkspindices_changed)
        ui.specNums.textChanged.connect(self._on_specnums_changed)

        # combobox changed
        ui.plotType.currentIndexChanged.connect(self._on_plot_type_changed)

    def _on_wkspindices_changed(self):
        ui = self._ui
        ui.specNums.clear()
        ui.specNumsValid.hide()

        self._parse_wksp_indices()
        ui.wkspIndicesValid.setVisible(not self._is_input_valid())
        if self._is_input_valid() or ui.wkspIndices.text() == "":
            ui.wkspIndicesValid.setVisible(False)
            ui.wkspIndicesValid.setToolTip("")
        elif ui.plotType.currentText() == SURFACE or ui.plotType.currentText() == CONTOUR:
            ui.wkspIndicesValid.setVisible(True)
            ui.wkspIndicesValid.setToolTip("Enter one workspace index in " + ui.wkspIndices.placeholderText())
        else:
            ui.wkspIndicesValid.setVisible(True)
            ui.wkspIndicesValid.setToolTip("Not in " + ui.wkspIndices.placeholderText())
        ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(self._is_input_valid())

        if self._advanced:
            ui.advanced_options_widget._validate_custom_logs(self._ui.advanced_options_widget.ui.custom_log_line_edit.text())

    def _on_specnums_changed(self):
        ui = self._ui
        ui.wkspIndices.clear()
        ui.wkspIndicesValid.hide()

        try:
            self._parse_spec_nums()
        except Exception as e:
            logger.error(f"Error parsing spectrum numbers: {e}")
            ui.specNumsValid.setToolTip("Invalid spectrum number input.")
            ui.specNumsValid.setVisible(True)
            ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        spectrum_numbers = [ws.getSpectrumNumbers() for ws in self._workspaces]
        unique_spectra = {tuple(numbers) for numbers in spectrum_numbers}

        if len(unique_spectra) > 1:
            ui.specNums.setEnabled(False)
            ui.specNumsValid.setToolTip("Spectrum numbers differ across workspaces. Use 'Plot All' or workspace indices instead.")
        else:
            ui.specNums.setEnabled(True)
            ui.specNumsValid.setToolTip("")

        ui.specNumsValid.setVisible(not self._is_input_valid())
        ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(self._is_input_valid())

        if self._advanced:
            ui.advanced_options_widget._validate_custom_logs(ui.advanced_options_widget.ui.custom_log_line_edit.text())

    def _on_plot_type_changed(self, new_index):
        if self._overplot:
            self._ui.plotType.setCurrentIndex(0)
            return

        if self.selection:
            self.selection.plot_type = new_index

        if self._advanced:
            new_text = self._ui.plotType.itemText(new_index)
            contour_or_surface = new_text == SURFACE or new_text == CONTOUR
            if contour_or_surface:
                self._ui.advanced_options_widget.ui.error_bars_check_box.setChecked(False)
                self._ui.advanced_options_widget.ui.error_bars_check_box.setEnabled(False)
                self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.setEnabled(True)
                self._ui.spec_num_label.setText("Spectrum Number")
                self._ui.wksp_indices_label.setText("Workspace Index")
                self._ui.advanced_options_widget.ui.log_value_combo_box.setItemText(0, WORKSPACE_REFERENCE_NUMBER)

                if self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.text() == WORKSPACE_NAME:
                    self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.setText(WORKSPACE_REFERENCE_NUMBER)

                self._ui.buttonBox.button(QDialogButtonBox.YesToAll).setEnabled(True)

            else:
                self._ui.advanced_options_widget.ui.error_bars_check_box.setEnabled(True)
                self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.setEnabled(False)
                self._ui.spec_num_label.setText("Spectrum Numbers")
                self._ui.wksp_indices_label.setText("Workspace Indices")
                self._ui.advanced_options_widget.ui.log_value_combo_box.setItemText(0, WORKSPACE_NAME)
                self._ui.buttonBox.button(QDialogButtonBox.YesToAll).setEnabled(True)
                self._ui.advanced_options_widget.ui.log_value_combo_box.setEnabled(new_text != "Tiled")

                if self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.text() == WORKSPACE_REFERENCE_NUMBER:
                    self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.setText(WORKSPACE_NAME)

                if new_text == "Tiled":
                    self._ui.advanced_options_widget.ui.log_value_combo_box.setCurrentIndex(0)

            # Changing plot type may change what a valid input for spectrum numbers / workspace indices is so whichever
            # currently contains input is rechecked.
            if self._ui.specNums.text() != "":
                self._on_specnums_changed()
            elif self._ui.wkspIndices.text() != "":
                self._on_wkspindices_changed()
            else:
                self._ui.advanced_options_widget._validate_custom_logs(self._ui.advanced_options_widget.ui.custom_log_line_edit.text())

    def _parse_wksp_indices(self):
        if self._ui.plotType.currentText() == CONTOUR or self._ui.plotType.currentText() == SURFACE:
            text = self._ui.wkspIndices.text()
            if text.isdigit() and self.wi_min <= int(text) <= self.wi_max:
                wksp_indices = [int(text)]
            else:
                wksp_indices = None
        else:
            wksp_indices = parse_selection_str(self._ui.wkspIndices.text(), self.wi_min, self.wi_max)

        if wksp_indices:
            selection = SpectraSelection(self._workspaces)
            selection.wksp_indices = wksp_indices
            selection.plot_type = self._ui.plotType.currentIndex()

            selection.errors = self._errorsset

            if self._advanced:
                selection.log_name = self._ui.advanced_options_widget.ui.log_value_combo_box.currentText()
                selection.axis_name = self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.text()
        else:
            selection = None
        self.selection = selection

    def _parse_spec_nums(self):
        if self._ui.plotType.currentText() == CONTOUR or self._ui.plotType.currentText() == SURFACE:
            text = self._ui.specNums.text()
            if text.isdigit() and int(text) in self._plottable_spectra:
                spec_nums = [int(text)]
            else:
                spec_nums = None
        else:
            spec_nums = parse_selection_str(self._ui.specNums.text(), allowed_values=self._plottable_spectra)

        if spec_nums:
            selection = SpectraSelection(self._workspaces)
            selection.spectra = spec_nums
            selection.plot_type = self._ui.plotType.currentIndex()

            selection.errors = self._errorsset

            if self._advanced:
                selection.log_name = self._ui.advanced_options_widget.ui.log_value_combo_box.currentText()
                selection.axis_name = self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.text()
        else:
            selection = None
        self.selection = selection

    def _is_input_valid(self):
        return self.selection is not None


AdvancedPlottingOptionsWidgetUI, AdvancedPlottingOptionsWidgetUIBase = load_ui(__file__, "advancedplotswidget.ui")


class AdvancedPlottingOptionsWidget(AdvancedPlottingOptionsWidgetUIBase):
    def __init__(self, parent=None):
        super(AdvancedPlottingOptionsWidget, self).__init__(parent=parent)
        ui = AdvancedPlottingOptionsWidgetUI()
        ui.setupUi(self)

        ui.logs_valid.setIcon(red_asterisk())
        ui.logs_valid.hide()

        ui.log_value_combo_box.currentIndexChanged.connect(self._log_value_changed)
        ui.error_bars_check_box.clicked.connect(self._toggle_errors)
        ui.custom_log_line_edit.textEdited.connect(self._validate_custom_logs)
        ui.plot_axis_label_line_edit.textEdited.connect(self._axis_name_changed)

        self.ui = ui
        self._parent = parent
        self._populate_log_combo_box()
        ui.plot_axis_label_line_edit.setText(ui.log_value_combo_box.currentText())

    def _log_value_changed(self) -> None:
        # Don't need to do anything while the combo box is being populated.
        if not self._parent._ui:
            return

        text = self.ui.log_value_combo_box.currentText()

        self.ui.custom_log_line_edit.setEnabled(text == CUSTOM)
        self.ui.custom_log_line_edit.clear()
        self.ui.plot_axis_label_line_edit.setText(text)

        if text != CUSTOM:
            self.ui.logs_valid.hide()
            # If the custom log values were the only thing stopping the input from being valid then the OK button can
            # be re-enabled
            if (
                not self._parent._ui.specNumsValid.isVisible()
                and not self._parent._ui.wkspIndicesValid.isVisible()
                and (self._parent._ui.specNums.text() or self._parent._ui.wkspIndices.text())
            ):
                self._parent._ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)
        else:
            self._validate_custom_logs(self.ui.custom_log_line_edit.text())

        if self._parent.selection:
            self._parent.selection.log_name = text
            self._parent.selection.axis_name = self.ui.plot_axis_label_line_edit.text()

    def _toggle_errors(self, enable: bool) -> None:
        if self._parent.selection:
            self._parent.selection.errors = enable
        else:
            self._parent._errorsset = enable

    def _axis_name_changed(self, text: str) -> None:
        if self._parent.selection:
            self._parent.selection.axis_name = text

    def _populate_log_combo_box(self) -> None:
        self.ui.log_value_combo_box.addItem(WORKSPACE_NAME)

        # Create a map of all logs and their double representation.
        # Only logs that can be converted to a double and are not all equal will be used.
        # It is a map of log_name -> [value, is_constant]
        usable_logs = {}
        # Add the logs that are present in the first workspace
        ws = self._parent._workspaces[0]
        if ws:
            run_object = ws.getRun()
            log_data = run_object.getLogData()

            for log in log_data:
                name = log.name

                try:
                    value = run_object.getPropertyAsSingleValueWithTimeAveragedMean(name)
                    usable_logs[name] = [value, True]
                except:
                    pass

        # Loop over all of the workspaces to see if each log value has changed.
        for ws in self._parent._workspaces:
            if ws:
                run_object = ws.getRun()
                for log_name in list(usable_logs):
                    if run_object.hasProperty(log_name):
                        if usable_logs[log_name][1]:
                            try:
                                value = run_object.getPropertyAsSingleValueWithTimeAveragedMean(log_name)
                            except:  # the log doesn't have a numeric value for every workspace so remove it
                                usable_logs.pop(log_name)
                                continue
                            # Set the bool to whether the value is the same
                            usable_logs[log_name][1] = value == usable_logs[log_name][0]
                    else:
                        usable_logs.pop(log_name)

        # Add the log names to the combo box if they appear in all workspaces and are non-constant.
        for log_name in usable_logs:
            if not usable_logs[log_name][1]:
                self.ui.log_value_combo_box.addItem(log_name)

        self.ui.log_value_combo_box.addItem(CUSTOM)

    def _validate_custom_logs(self, text: str, plot_all: bool = False) -> Union[bool, None]:
        if self.ui.log_value_combo_box.currentText() == CUSTOM:
            valid_options = True
            values = text.split(",")
            first_value = True
            previous_value = 0.0

            for value in values:
                try:
                    current_value = float(value)
                except ValueError:
                    self.ui.logs_valid.show()
                    self.ui.logs_valid.setToolTip(f"A custom log value is not valid: {value}")
                    valid_options = False
                    break

                if first_value:
                    first_value = False
                    previous_value = current_value
                else:
                    if previous_value < current_value:
                        previous_value = current_value
                    else:
                        self.ui.logs_valid.setToolTip("The custom log values must be in numerical order and distinct.")
                        valid_options = False
                        break

            if valid_options:
                number_of_custom_log_values = len(values)
                number_of_workspaces = len(self._parent._workspaces)

                plot_type = self._parent._ui.plotType.currentText()
                if (plot_type == SURFACE or plot_type == CONTOUR) and number_of_custom_log_values != number_of_workspaces:
                    self.ui.logs_valid.show()
                    self.ui.logs_valid.setToolTip(
                        f"The number of custom log values ({number_of_custom_log_values}) is "
                        f"not equal to the number of workspaces ({number_of_workspaces})."
                    )
                    valid_options = False
                else:
                    if plot_all or not (self._parent._ui.specNums.text() or self._parent._ui.wkspIndices.text()):
                        # If the user has not entered spectrum numbers or workspace indices assume they are going to
                        # plot all and validate for this.
                        index_length = self._parent.wi_max - self._parent.wi_min + 1
                    else:
                        if self._parent.selection:
                            if self._parent.selection.wksp_indices:
                                index_length = len(self._parent.selection.wksp_indices)
                            elif self._parent.selection.spectra:
                                index_length = len(self._parent.selection.spectra)
                        else:
                            index_length = 0

                    number_of_plots = number_of_workspaces * index_length

                    if number_of_custom_log_values != number_of_plots:
                        self.ui.logs_valid.show()
                        self.ui.logs_valid.setToolTip(
                            f"The number of custom log values ({number_of_custom_log_values}) "
                            f"is not equal to the number of plots ({number_of_plots})."
                        )

                        valid_options = False

            if valid_options:
                self.ui.logs_valid.hide()
                self.ui.logs_valid.setToolTip("")

                if self._parent.selection and not plot_all:
                    values = [float(value) for value in values]
                    self._parent.selection.custom_log_values = values

                if self._parent._ui.specNums.text() or self._parent._ui.wkspIndices.text():
                    self._parent._ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)
            else:
                self._parent._ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

            return valid_options


def parse_selection_str(txt, min_val=None, max_val=None, allowed_values=None):
    """Parse an input string containing plot index selection.

    :param txt: A single line of text containing a comma-separated list of values or range of values, i.e.
    3-4,5,6,8,10-11
    :param min_val: The minimum allowed value
    :param max_val: The maximum allowed value
    :param allowed_values: The list of allowed values, if this is provided then max and min will be ignored
    :returns A list containing each value in the range or None if the string is invalid
    """

    def append_if_valid(out, val):
        try:
            val = int(val)
            if is_in_range(val):
                out.add(val)
            else:
                return False
        except ValueError:
            return False
        return True

    def is_in_range(val):
        if allowed_values is not None:
            return val in allowed_values
        else:
            return min_val <= val <= max_val

    # split up any commas
    comma_separated = txt.split(",")
    # find and expand ranges
    parsed_numbers = set()
    valid = True
    for cs_item in comma_separated:
        post_split = cs_item.split("-")
        if len(post_split) == 1:
            valid = append_if_valid(parsed_numbers, post_split[0])
        elif len(post_split) == 2:
            # parse as range
            try:
                beg, end = int(post_split[0]), int(post_split[1])
            except ValueError:
                valid = False
            else:
                if is_in_range(beg) and is_in_range(end):
                    parsed_numbers = parsed_numbers.union(set(range(beg, end + 1)))
                else:
                    valid = False
        else:
            valid = False
        if not valid:
            break

    return list(parsed_numbers) if valid > 0 else None
