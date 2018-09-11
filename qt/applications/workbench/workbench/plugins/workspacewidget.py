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
from mantidqt.widgets.workspacewidget.workspacetreewidget import WorkspaceTreeWidget
from qtpy.QtWidgets import QMessageBox, QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget
from workbench.plotting.functions import can_overplot, pcolormesh, plot_from_names


class WorkspaceWidget(PluginWidget):
    """Provides a Workspace Widget for workspace manipulation"""

    def __init__(self, parent):
        super(WorkspaceWidget, self).__init__(parent)

        self._ads = AnalysisDataService.Instance()

        # layout
        self.workspacewidget = WorkspaceTreeWidget()
        layout = QVBoxLayout()
        layout.addWidget(self.workspacewidget)
        self.setLayout(layout)

        # behaviour
        self.workspacewidget.plotSpectrumClicked.connect(functools.partial(self._do_plot_spectrum,
                                                                           errors=False, overplot=False))
        self.workspacewidget.overplotSpectrumClicked.connect(functools.partial(self._do_plot_spectrum,
                                                                               errors=False, overplot=True))
        self.workspacewidget.plotSpectrumWithErrorsClicked.connect(functools.partial(self._do_plot_spectrum,
                                                                                     errors=True, overplot=False))
        self.workspacewidget.overplotSpectrumWithErrorsClicked.connect(functools.partial(self._do_plot_spectrum,
                                                                                         errors=True, overplot=True))
        self.workspacewidget.plotColorfillClicked.connect(self._do_plot_colorfill)

    # ----------------- Plugin API --------------------

    def register_plugin(self):
        self.main.add_dockwidget(self)

    def get_plugin_title(self):
        return "Workspaces"

    def read_user_settings(self, _):
        pass

    # ----------------- Behaviour --------------------

    def _do_plot_spectrum(self, names, errors, overplot):
        """
        Plot spectra from the selected workspaces

        :param names: A list of workspace names
        :param errors: If true then error bars will be plotted on the points
        :param overplot: If true then the add to the current figure if one
                         exists and it is a compatible figure
        """
        if overplot:
            compatible, error_msg = can_overplot()
            if not compatible:
                QMessageBox.warning(self, "", error_msg)
                return

        plot_from_names(names, errors, overplot)

    def _do_plot_colorfill(self, names):
        """
        Plot a colorfill from the selected workspaces

        :param names: A list of workspace names
        """
        try:
            pcolormesh(self._ads.retrieveWorkspaces(names, unrollGroups=True))
        except BaseException:
            import traceback
            traceback.print_exc()
