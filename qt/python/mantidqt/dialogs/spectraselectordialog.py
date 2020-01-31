# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialogButtonBox, QMessageBox, QWidget

from mantid.kernel import logger
from mantid.api import MatrixWorkspace
from mantid.simpleapi import AnalysisDataService as ads

from mantidqt.icons import get_icon
from mantidqt.utils.qt import load_ui

# Constants
RANGE_SPECIFIER = '-'
PLACEHOLDER_FORMAT = 'valid range: {}' + RANGE_SPECIFIER + '{}'
PLACEHOLDER_FORMAT_SINGLE_INPUT = 'valid range: {}'
RED_ASTERISK = None


def red_asterisk():
    global RED_ASTERISK
    if RED_ASTERISK is None:
        RED_ASTERISK = get_icon('mdi.asterisk', 'red', 0.6)
    return RED_ASTERISK


SpectraSelectionDialogUI, SpectraSelectionDialogUIBase = load_ui(__file__, 'spectraselectordialog.ui')


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
        self.log_value = None
        self.custom_log_values = None
        self.label = None


class SpectraSelectionDialog(SpectraSelectionDialogUIBase):

    @staticmethod
    def get_compatible_workspaces(workspaces):
        matrix_workspaces = []
        for ws in workspaces:
            if isinstance(ws, MatrixWorkspace):
                matrix_workspaces.append(ws)
            else:
                # Log an error but carry on so valid workspaces can be plotted.
                logger.warning("{}: Expected MatrixWorkspace, found {}".format(ws.name(), ws.__class__.__name__))

        return matrix_workspaces

    def __init__(self, workspaces, parent=None, show_colorfill_btn=False, overplot=False, advanced=False):
        super(SpectraSelectionDialog, self).__init__(parent)
        self.icon = self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
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

        self._init_ui()
        self._set_placeholder_text()
        self._setup_connections()
        self._on_specnums_changed()
        self._on_wkspindices_changed()

    def on_ok_clicked(self):
        if self._check_number_of_plots(self.selection):
            self.accept()

    def on_plot_all_clicked(self):
        selection = SpectraSelection(self._workspaces)
        selection.wksp_indices = range(self.wi_min, self.wi_max + 1)
        selection.plot_type = self._ui.plotType.currentIndex()

        if self._advanced:
            advanced_selections = self._get_advanced_selections()
            for option in advanced_selections:
                selection.option = advanced_selections[option]

        if self._check_number_of_plots(selection):
            self.selection = selection
            self.accept()

    def on_colorfill_clicked(self):
        self.selection = 'colorfill'
        self.accept()

    # ------------------- Private -------------------------
    def _check_number_of_plots(self, selection):
        index_length = len(selection.wksp_indices) if selection.wksp_indices else len(selection.spectra)
        number_of_lines_to_plot = len(selection.workspaces) * index_length
        if selection.plot_type == SpectraSelection.Tiled and number_of_lines_to_plot > 12:
            response = QMessageBox.warning(self, 'Mantid Workbench',
                                           'Are you sure you want to plot {} subplots?'.format(number_of_lines_to_plot),
                                           QMessageBox.Ok | QMessageBox.Cancel)
            return response == QMessageBox.Ok
        elif selection.plot_type != SpectraSelection.Tiled and number_of_lines_to_plot > 10:
            response = QMessageBox.warning(self, 'Mantid Workbench', 'You selected {} spectra to plot. Are you sure '
                                           'you want to plot this many?'.format(number_of_lines_to_plot),
                                           QMessageBox.Ok | QMessageBox.Cancel)
            return response == QMessageBox.Ok

        return True

    def _init_ui(self):
        ui = SpectraSelectionDialogUI()
        ui.setupUi(self)

        self._ui = ui
        ui.colorfillButton.setVisible(self._show_colorfill_button)
        # overwrite the "Yes to All" button text
        ui.buttonBox.button(QDialogButtonBox.YesToAll).setText('Plot All')
        # ok disabled by default
        ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        # validity markers
        ui.wkspIndicesValid.setIcon(red_asterisk())
        ui.specNumsValid.setIcon(red_asterisk())

        if self._advanced:
            ui.advanced_options_widget = AdvancedPlottingOptionsWidget(parent=self)
            ui.layout.replaceWidget(ui.advanced_plots_dummy_widget, ui.advanced_options_widget)
            if len(self._workspaces) > 2:
                ui.plotType.addItem("Surface")
                ui.plotType.addItem("Contour")
            self.setWindowTitle("Plot Advanced")

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
            raise Exception('Error: Workspaces have no common spectra.')
        #store the plottable list for use later
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
        return ', '.join(ranges)

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
        else:
            ui.wkspIndicesValid.setVisible(True)
            ui.wkspIndicesValid.setToolTip("Not in " + ui.wkspIndices.placeholderText())
        ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(self._is_input_valid())

    def _on_specnums_changed(self):
        ui = self._ui
        ui.wkspIndices.clear()
        ui.wkspIndicesValid.hide()

        self._parse_spec_nums()
        ui.specNumsValid.setVisible(not self._is_input_valid())
        if self._is_input_valid() or ui.specNums.text() == "":
            ui.specNumsValid.setVisible(False)
            ui.specNumsValid.setToolTip("")
        else:
            ui.specNumsValid.setVisible(True)
            ui.specNumsValid.setToolTip("Not in " + ui.specNums.placeholderText())
        ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(self._is_input_valid())

    def _on_plot_type_changed(self, new_index):
        if self._overplot:
            self._ui.plotType.setCurrentIndex(0)
            return

        if self.selection:
            self.selection.plot_type = new_index

        new_text = self._ui.plotType.itemText(new_index)
        contour_or_surface = new_text == "Surface" or new_text == "Contour"
        if contour_or_surface:
            self._ui.advanced_options_widget.ui.error_bars_check_box.setChecked(False)
            self._ui.advanced_options_widget.ui.error_bars_check_box.setEnabled(False)
            self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.setEnabled(True)
        else:
            self._ui.advanced_options_widget.ui.error_bars_check_box.setEnabled(True)
            self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.setEnabled(False)

        if new_text == "Tiled":
            self._ui.advanced_options_widget.ui.log_value_combo_box.setEnabled(False)
        else:
            self._ui.advanced_options_widget.ui.log_value_combo_box.setEnabled(True)
            if contour_or_surface:
                self._ui.advanced_options_widget.ui.log_value_combo_box.setItemText(0, "Workspace index")
                if self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.text() == "Workspace name":
                    self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.setText("Workspace index")
            else:
                self._ui.advanced_options_widget.ui.log_value_combo_box.setItemText(0, "Workspace name")

    def _parse_wksp_indices(self):
        if self._ui.plotType.currentText() == "Contour" or self._ui.plotType.currentText() == "Surface":
            text = self._ui.wkspIndices.text()
            if text.isdigit() and self.wi_min <= int(text) <= self.wi_max:
                wksp_indices = text
            else:
                wksp_indices = None
        else:
            wksp_indices = parse_selection_str(self._ui.wkspIndices.text(), self.wi_min, self.wi_max)

        if wksp_indices:
            selection = SpectraSelection(self._workspaces)
            selection.wksp_indices = wksp_indices
            selection.plot_type = self._ui.plotType.currentIndex()

            if self._advanced:
                advanced_selections = self._get_advanced_selections()
                for option in advanced_selections:
                    selection.option = advanced_selections[option]
        else:
            selection = None
        self.selection = selection

    def _parse_spec_nums(self):
        if self._ui.plotType.currentText() == "Contour" or self._ui.plotType.currentText() == "Surface":
            text = self._ui.specNums.text()
            if text.isdigit() and int(text) in self._plottable_spectra:
                spec_nums = text
            else:
                spec_nums = None
        else:
            spec_nums = parse_selection_str(self._ui.specNums.text(), allowed_values=self._plottable_spectra)

        if spec_nums:
            selection = SpectraSelection(self._workspaces)
            selection.spectra = spec_nums
            selection.plot_type = self._ui.plotType.currentIndex()

            if self._advanced:
                advanced_selections = self._get_advanced_selections()
                for option in advanced_selections:
                    selection.option = advanced_selections[option]
        else:
            selection = None
        self.selection = selection

    def _is_input_valid(self):
        return self.selection is not None

    def _get_advanced_selections(self):
        selection = {'errors': self._ui.advanced_options_widget.ui.error_bars_check_box.isChecked()}

        if self._ui.advanced_options_widget.ui.log_value_combo_box.isEnabled():
            selection['log_value'] = self._ui.advanced_options_widget.ui.log_value_combo_box.currentText()
        else:
            selection['log_value'] = None

        if self._ui.advanced_options_widget.ui.custom_log_line_edit.isEnabled():
            selection['custom_log_values'] = self._ui.advanced_options_widget.ui.custom_log_line_edit.text()
        else:
            selection['custom_log_values'] = None

        if self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.isEnabled():
            selection['label'] = self._ui.advanced_options_widget.ui.plot_axis_label_line_edit.text()
        else:
            selection['label'] = None

        return selection


AdvancedPlottingOptionsWidgetUI, AdvancedPlottingOptionsWidgetUIBase = load_ui(__file__, 'advancedplotswidget.ui')


class AdvancedPlottingOptionsWidget(AdvancedPlottingOptionsWidgetUIBase):
    def __init__(self, parent=None):
        super(AdvancedPlottingOptionsWidget, self).__init__(parent=parent)
        ui = AdvancedPlottingOptionsWidgetUI()
        ui.setupUi(self)

        ui.plot_axis_label_line_edit.setText(ui.log_value_combo_box.currentText())

        ui.log_value_combo_box.currentTextChanged.connect(self._log_value_changed)
        ui.error_bars_check_box.clicked.connect(self._toggle_errors)

        self.ui = ui
        self._parent = parent
        self._populate_log_combo_box()

    def _log_value_changed(self, text):
        self.ui.custom_log_line_edit.setEnabled(text == "Custom")
        self.ui.custom_log_line_edit.clear()
        self.ui.plot_axis_label_line_edit.setText(text)

    def _toggle_errors(self, enable):
        if self._parent.selection:
            self._parent.selection.errors = enable

    def _populate_log_combo_box(self):
        self.ui.log_value_combo_box.addItem("Workspace index")

        usable_logs = {}
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

        for ws in self._parent._workspaces:
            if ws:
                run_object = ws.getRun()
                for log_name in list(usable_logs):
                    if run_object.hasProperty(log_name):
                        if usable_logs[log_name][1]:
                            value = run_object.getPropertyAsSingleValueWithTimeAveragedMean(log_name)
                            usable_logs[log_name][1] = value == usable_logs[log_name][0]
                    else:
                        usable_logs.pop(log_name)

        for log_name in usable_logs:
            if not usable_logs[log_name][1]:
                self.ui.log_value_combo_box.addItem(log_name)

        self.ui.log_value_combo_box.addItem("Custom")



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
    comma_separated = txt.split(',')
    # find and expand ranges
    parsed_numbers = set()
    valid = True
    for cs_item in comma_separated:
        post_split = cs_item.split('-')
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
