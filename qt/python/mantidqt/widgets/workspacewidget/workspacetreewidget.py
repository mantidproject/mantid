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
from __future__ import (absolute_import)

# std imports

# 3rd party imports
from qtpy.QtWidgets import QDialogButtonBox

# local imports
from mantidqt.utils.qt import import_qtlib, load_ui


WorkspaceTreeWidget = import_qtlib('_widgetscore', 'mantidqt.widgets.workspacewidget',
                                   'WorkspaceTreeWidgetSimple')

PlotSelectionDialogUI, PlotSelectionDialogUIBase = load_ui(__file__, 'plotselectiondialog.ui')


class PlotSelectionDialog(PlotSelectionDialogUIBase):

    def __init__(self, workspaces,
                 parent=None):
        super(PlotSelectionDialog, self).__init__(parent)
        # attributes
        self._workspaces = workspaces

        # layout
        ui = PlotSelectionDialogUI()
        ui.setupUi(self)
        self._ui = ui
        # overwrite the "Yes to All" button text
        ui.buttonBox.button(QDialogButtonBox.YesToAll).setText('Plot All')

        # populate details
        self._fill_workspace_details()

    # ------------------- Private -------------------------
    def _fill_workspace_details(self):
        """Sets placeholder text to indicate the ranges possible"""
        workspaces = self._workspaces
        # workspace index range
        wi_max = min([ws.getNumberHistograms() - 1 for ws in workspaces])
        self._ui.wkspIndices.setPlaceholderText("{}-{}".format(0, wi_max))

        # spectra range
        ws_spectra = [{ws.getSpectrum(i).getSpectrumNo() for i in range(ws.getNumberHistograms())} for ws in workspaces]
        plottable =  ws_spectra[0]
        if len(ws_spectra) > 1:
            for sp_set in ws_spectra[1:]:
                plottable = plottable.intersection(sp_set)
        plottable = sorted(plottable)
        spec_min, spec_max = min(plottable), max(plottable)
        self._ui.specNums.setPlaceholderText("{}-{}".format(spec_min, spec_max))
