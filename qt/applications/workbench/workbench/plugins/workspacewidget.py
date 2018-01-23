#    This file is part of the mantid workbench.
#
#    Copyright (C) 2017 mantidproject
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, unicode_literals)

# system imports
import functools

# third-party library imports
from mantid.api import AnalysisDataService
from mantidqt.widgets.workspacewidget.mantidtreemodel import MantidTreeModel
from mantidqt.widgets.workspacewidget.plotselectiondialog import get_plot_selection
from mantidqt.widgets.workspacewidget.workspacetreewidget import WorkspaceTreeWidget
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget
from workbench.plotting.functions import plot


class WorkspaceWidget(PluginWidget):
    """Provides a Workspace Widget for workspace manipulation"""

    def __init__(self, parent):
        super(WorkspaceWidget, self).__init__(parent)

        # layout
        workspacewidget = WorkspaceTreeWidget(MantidTreeModel())
        self.workspacewidget = workspacewidget
        layout = QVBoxLayout()
        layout.addWidget(self.workspacewidget)
        self.setLayout(layout)

        # behaviour
        workspacewidget.plotSpectrumClicked.connect(functools.partial(self._do_plot1d, errors=False))
        workspacewidget.plotSpectrumWithErrorsClicked.connect(functools.partial(self._do_plot1d, errors=True))

    # ----------------- Plugin API --------------------

    def register_plugin(self):
        self.main.add_dockwidget(self)

    def get_plugin_title(self):
        return "Workspaces"

    # ----------------- Behaviour --------------------

    def _do_plot1d(self, workspace_names, errors):
        try:
            ads = AnalysisDataService.Instance()
            workspaces = [ads[name.encode('utf-8')] for name in workspace_names]
            selection = get_plot_selection(workspaces, self)
            if selection is not None:
                plot(selection.workspaces, spectrum_nums=selection.spectra,
                     wksp_indices=selection.wksp_indices,
                     errors=errors)
        except BaseException:
            import traceback
            traceback.print_exc()
