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
from mantid.kernel import Logger
from mantidqt.widgets.workspacewidget.mantidtreemodel import MantidTreeModel
from mantidqt.widgets.workspacewidget.plotselectiondialog import get_plot_selection
from mantidqt.widgets.workspacewidget.workspacetreewidget import WorkspaceTreeWidget
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget
from workbench.plotting.functions import pcolormesh, plot

LOGGER = Logger(b"workspacewidget")


def _workspaces_from_names(names):
    """
    Convert a list of workspace names to a list of MatrixWorkspace handles. Any WorkspaceGroup
    encountered is flattened and its members inserted into the list at this point

    Flattens any workspace groups with the list, preserving the order of the remaining elements
    :param names: A list of workspace names
    :return: A list where each element is a single MatrixWorkspace
    """
    ads = AnalysisDataService.Instance()
    flat = []
    for name in names:
        try:
            ws = ads[name.encode('utf-8')]
        except KeyError:
            LOGGER.warning("Skipping {} as it does not exist".format(name))
            continue

        if isinstance(ws, MatrixWorkspace):
            flat.append(ws)
        elif isinstance(ws, WorkspaceGroup):
            group_len = len(ws)
            for i in range(group_len):
                flat.append(ws[i])
        else:
            LOGGER.warning("{} it is not a MatrixWorkspace or WorkspaceGroup.".format(name))

    return flat


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
        workspacewidget.plotSpectrumClicked.connect(functools.partial(self._do_plot_spectrum,
                                                                      errors=False, overplot=False))
        workspacewidget.overplotSpectrumClicked.connect(functools.partial(self._do_plot_spectrum,
                                                                          errors=False, overplot=True))
        workspacewidget.plotSpectrumWithErrorsClicked.connect(functools.partial(self._do_plot_spectrum,
                                                                                errors=True, overplot=False))
        workspacewidget.overplotSpectrumWithErrorsClicked.connect(functools.partial(self._do_plot_spectrum,
                                                                                    errors=True, overplot=True))
        workspacewidget.plotColorfillClicked.connect(self._do_plot_colorfill)

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
                         exists
        """
        try:
            selection = get_plot_selection(_workspaces_from_names(names), self)
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
            pcolormesh(_workspaces_from_names(names))
        except BaseException:
            import traceback
            traceback.print_exc()
