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
from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup
from mantidqt.dialogs.spectraselectordialog import get_spectra_selection
from mantidqt.widgets.workspacewidget.workspacetreewidget import WorkspaceTreeWidget
from qtpy.QtWidgets import QMessageBox, QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget
from workbench.plotting.figuretype import figure_type, FigureType
from workbench.plotting.functions import current_figure_or_none, pcolormesh, plot


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
            compatible, error_msg = self._can_overplot()
            if not compatible:
                QMessageBox.warning(self, "", error_msg)
                return

        try:
            workspaces = self._ads.retrieveWorkspaces(names, unrollGroups=True)
            selection = get_spectra_selection(workspaces, self)
            if selection is not None:
                plot(selection.workspaces, spectrum_nums=selection.spectra,
                     wksp_indices=selection.wksp_indices,
                     errors=errors, overplot=overplot)
        except BaseException:
            import traceback
            traceback.print_exc()

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

    def _can_overplot(self):
        """
        Checks if overplotting can proceed with the given options

        :return: A 2-tuple of boolean indicating compatability and
        a string containing an error message if the current figure is not
        compatible.
        """
        compatible = False
        msg = "Unable to overplot on currently active plot type.\n" \
              "Please select another plot."
        fig = current_figure_or_none()
        if fig is not None:
            figtype = figure_type(fig)
            if figtype is FigureType.Line or figtype is FigureType.Errorbar:
                compatible, msg = True, None

        return compatible, msg
