# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialogButtonBox

from mantid.api import MatrixWorkspace
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

    def __init__(self, workspaces, parent=None, show_colorfill_btn=False):
        super(SpectraSelectionDialog, self).__init__(parent)
        self.icon = self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.raise_error_if_workspaces_not_compatible(workspaces)

        # attributes
        self._workspaces = workspaces
        self.spec_min, self.spec_max = None, None
        self.wi_min, self.wi_max = None, None
        self.selection = None
        self._ui = None
        self._show_colorfill_button = show_colorfill_btn

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

    def on_colorfill_clicked(self):
        self.selection = 'colorfill'
        self.accept()

    # ------------------- Private -------------------------

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
        ui.colorfillButton.clicked.connect(self.on_colorfill_clicked)

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
                    parsed_numbers = parsed_numbers.union(set(range(beg, end + 1)))
                else:
                    valid = False
        else:
            valid = False
        if not valid:
            break

    return list(parsed_numbers) if valid > 0 else None
