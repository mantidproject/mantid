#  This file is part of the mantidqt package
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, unicode_literals)

# std imports

# 3rd party imports
from qtpy.QtWidgets import QDialogButtonBox

# local imports
from mantidqt.utils.qt import load_ui

# Constants
PLACEHOLDER_FORMAT = 'valid range: {}-{}'


PlotSelectionDialogUI, PlotSelectionDialogUIBase = load_ui(__file__, 'plotselectiondialog.ui')


class PlotSelection(object):

    Individual = 0

    def __init__(self, workspaces):
        self.workspaces = workspaces
        self.wksp_indices = None
        self.spectra = None
        self.plot_type = PlotSelection.Individual


class PlotSelectionDialog(PlotSelectionDialogUIBase):

    def __init__(self, workspaces,
                 parent=None):
        super(PlotSelectionDialog, self).__init__(parent)
        # attributes
        self._workspaces = workspaces
        self.spec_min, self.spec_max = None, None
        self.wi_min, self.wi_max = None, None
        self.user_selection = None

        # layout
        ui = PlotSelectionDialogUI()
        ui.setupUi(self)
        self._ui = ui
        # overwrite the "Yes to All" button text
        ui.buttonBox.button(QDialogButtonBox.YesToAll).setText('Plot All')

        self._set_placeholder_text()
        self._setup_mutually_exclusive_inputs()
        self._setup_connections()

        # disable okay
        ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

    def on_ok_clicked(self):
        user_selection = PlotSelection(self._workspaces)
        user_selection.wksp_indices = range(self.wi_min, self.wi_max)
        self.accept()

    def on_plot_all_clicked(self):
        user_selection = PlotSelection(self._workspaces)
        user_selection.wksp_indices = range(self.wi_min, self.wi_max + 1)
        self.user_selection = user_selection
        self.accept()

    # ------------------- Private -------------------------
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

    def _setup_mutually_exclusive_inputs(self):
        """Sets the workspace index and spectrum index boxes to be mutually exclusive"""
        self._ui.specNums.textEdited.connect(self._ui.wkspIndices.clear)
        self._ui.wkspIndices.textEdited.connect(self._ui.specNums.clear)

    def _setup_connections(self):
        ui = self._ui
        ui.buttonBox.button(QDialogButtonBox.Ok).clicked.connect(self.on_ok_clicked)
        ui.buttonBox.button(QDialogButtonBox.Cancel).clicked.connect(self.reject)
        ui.buttonBox.button(QDialogButtonBox.YesToAll).clicked.connect(self.on_plot_all_clicked)

