# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (absolute_import, unicode_literals)

# std imports

# 3rd party imports
from mantid.api import MatrixWorkspace
from qtpy.QtWidgets import QDialogButtonBox

# local imports
from mantidqt.icons import get_icon
from mantidqt.utils.qt import load_ui

# Constants
RANGE_SPECIFIER = '-'
PLACEHOLDER_FORMAT = 'valid range: {}' + RANGE_SPECIFIER + '{}'
RED_ASTERISK = None


def red_asterisk():
    global RED_ASTERISK
    if RED_ASTERISK is None:
        RED_ASTERISK = get_icon('mdi.asterisk', 'red', 0.6)
    return RED_ASTERISK


SpectraSelectionDialogUI, SpectraSelectionDialogUIBase = load_ui(__file__, 'spectraselectordialog.ui')


class SpectraSelection(object):

    Individual = 0

    def __init__(self, workspaces):
        self.workspaces = workspaces
        self.wksp_indices = None
        self.spectra = None
        self.plot_type = SpectraSelection.Individual


class SpectraSelectionDialog(SpectraSelectionDialogUIBase):

    @staticmethod
    def raise_error_if_workspaces_not_compatible(workspaces):
        for ws in workspaces:
            if not isinstance(ws, MatrixWorkspace):
                raise ValueError("Expected MatrixWorkspace, found {}.".format(ws.__class__.__name__))

    def __init__(self, workspaces,
                 parent=None):
        super(SpectraSelectionDialog, self).__init__(parent)
        self.raise_error_if_workspaces_not_compatible(workspaces)

        # attributes
        self._workspaces = workspaces
        self.spec_min, self.spec_max = None, None
        self.wi_min, self.wi_max = None, None
        self.selection = None
        self._ui = None

        self._init_ui()
        self._set_placeholder_text()
        self._setup_connections()

    def on_ok_clicked(self):
        self.accept()

    def on_plot_all_clicked(self):
        selection = SpectraSelection(self._workspaces)
        selection.wksp_indices = range(self.wi_min, self.wi_max + 1)
        self.selection = selection
        self.accept()

    # ------------------- Private -------------------------

    def _init_ui(self):
        ui = SpectraSelectionDialogUI()
        ui.setupUi(self)
        self._ui = ui
        # overwrite the "Yes to All" button text
        ui.buttonBox.button(QDialogButtonBox.YesToAll).setText('Plot All')
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
        spec_min, spec_max = min(plottable), max(plottable)
        self._ui.specNums.setPlaceholderText(PLACEHOLDER_FORMAT.format(spec_min, spec_max))
        self.spec_min, self.spec_max = spec_min, spec_max

    def _setup_connections(self):
        ui = self._ui
        # button actions
        ui.buttonBox.button(QDialogButtonBox.Ok).clicked.connect(self.on_ok_clicked)
        ui.buttonBox.button(QDialogButtonBox.Cancel).clicked.connect(self.reject)
        ui.buttonBox.button(QDialogButtonBox.YesToAll).clicked.connect(self.on_plot_all_clicked)

        # line edits are mutually exclusive
        ui.wkspIndices.textChanged.connect(self._on_wkspindices_changed)
        ui.specNums.textChanged.connect(self._on_specnums_changed)

    def _on_wkspindices_changed(self):
        ui = self._ui
        ui.specNums.clear()
        ui.specNumsValid.hide()

        self._parse_wksp_indices()
        ui.wkspIndicesValid.setVisible(not self._is_input_valid())
        ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(self._is_input_valid())

    def _on_specnums_changed(self):
        ui = self._ui
        ui.wkspIndices.clear()
        ui.wkspIndicesValid.hide()

        self._parse_spec_nums()
        ui.specNumsValid.setVisible(not self._is_input_valid())
        ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(self._is_input_valid())

    def _parse_wksp_indices(self):
        wksp_indices = parse_selection_str(self._ui.wkspIndices.text(), self.wi_min, self.wi_max)
        if wksp_indices:
            selection = SpectraSelection(self._workspaces)
            selection.wksp_indices = wksp_indices
        else:
            selection = None
        self.selection = selection

    def _parse_spec_nums(self):
        spec_nums = parse_selection_str(self._ui.specNums.text(), self.spec_min, self.spec_max)
        if spec_nums:
            selection = SpectraSelection(self._workspaces)
            selection.spectra = spec_nums
        else:
            selection = None
        self.selection = selection

    def _is_input_valid(self):
        return self.selection is not None


def get_spectra_selection(workspaces, parent_widget=None):
    """Decides whether it is necessary to request user input
    when asked to plot a list of workspaces. The input
    dialog will only be shown in the case where all workspaces
    have more than 1 spectrum

    :param workspaces: A list of MatrixWorkspaces that will be plotted
    :param parent_widget: An optional parent_widget to use for the input selection dialog
    :returns: Either a SpectraSelection object containing the details of workspaces to plot or None indicating
    the request was cancelled
    :raises ValueError: if the workspaces are not of type MatrixWorkspace
    """
    SpectraSelectionDialog.raise_error_if_workspaces_not_compatible(workspaces)
    single_spectra_ws = [wksp.getNumberHistograms() for wksp in workspaces if wksp.getNumberHistograms() == 1]
    if len(single_spectra_ws) > 0:
        # At least 1 workspace contains only a single spectrum so this is all
        # that is possible to plot for all of them
        selection = SpectraSelection(workspaces)
        selection.wksp_indices = [0]
        return selection
    else:
        selection_dlg = SpectraSelectionDialog(workspaces, parent=parent_widget)
        res = selection_dlg.exec_()
        if res == SpectraSelectionDialog.Rejected:
            # cancelled
            return None
        else:
            user_selection = selection_dlg.selection
            # the dialog should guarantee that only 1 of spectrum/indices is supplied
            assert user_selection.spectra is None or user_selection.wksp_indices is None
            return user_selection


def parse_selection_str(txt, min_val, max_val):
    """Parse an input string containing plot index selection.

    :param txt: A single line of text containing a comma-separated list of values or range of values, i.e.
    3-4,5,6,8,10-11
    :param min_val: The minimum allowed value
    :param max_val: The maximum allowed value
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
                    parsed_numbers = parsed_numbers.union(set(range(beg, end+1)))
                else:
                    valid = False
        else:
            valid = False
        if not valid:
            break

    return list(parsed_numbers) if valid > 0 else None
