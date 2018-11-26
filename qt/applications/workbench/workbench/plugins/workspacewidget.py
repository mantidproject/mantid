# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, unicode_literals)

# system imports
from functools import partial

# third-party library imports
from mantid.api import AnalysisDataService
from mantidqt.widgets.matrixworkspacedisplay.presenter import MatrixWorkspaceDisplay
from mantidqt.widgets.samplelogs.presenter import SampleLogs
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter
from mantidqt.widgets.workspacewidget.workspacetreewidget import WorkspaceTreeWidget
from qtpy.QtWidgets import QMessageBox, QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget
from workbench.plotting.functions import can_overplot, pcolormesh, plot_from_names, plot


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
        self.workspacewidget.plotSpectrumClicked.connect(partial(self._do_plot_spectrum,
                                                                 errors=False, overplot=False))
        self.workspacewidget.overplotSpectrumClicked.connect(partial(self._do_plot_spectrum,
                                                                     errors=False, overplot=True))
        self.workspacewidget.plotSpectrumWithErrorsClicked.connect(partial(self._do_plot_spectrum,
                                                                           errors=True, overplot=False))
        self.workspacewidget.overplotSpectrumWithErrorsClicked.connect(partial(self._do_plot_spectrum,
                                                                               errors=True, overplot=True))
        self.workspacewidget.plotColorfillClicked.connect(self._do_plot_colorfill)
        self.workspacewidget.sampleLogsClicked.connect(self._do_sample_logs)
        self.workspacewidget.showDataClicked.connect(self._do_show_data)
        self.workspacewidget.showInstrumentClicked.connect(self._do_show_instrument)

        self.workspacewidget.workspaceDoubleClicked.connect(self._action_double_click_workspace)

    # ----------------- Plugin API --------------------

    def register_plugin(self):
        self.main.add_dockwidget(self)

    def get_plugin_title(self):
        return "Workspaces"

    def readSettings(self, _):
        pass

    def writeSettings(self, _):
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

    def _do_sample_logs(self, names):
        """
        Show the sample log window for the given workspaces

        :param names: A list of workspace names
        """
        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            SampleLogs(ws=ws, parent=self)

    def _do_show_instrument(self, names):
        """
        Show an instrument widget for the given workspaces

        :param names: A list of workspace names
        """
        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            presenter = InstrumentViewPresenter(ws, parent=self)
            presenter.view.show()

    def _do_show_data(self, names):
        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            # the plot function is being injected in the presenter
            # this is done so that the plotting library is mockable in testing
            presenter = MatrixWorkspaceDisplay(ws, plot=plot, parent=self)
            presenter.view.show()

    def _action_double_click_workspace(self, name):
        self._do_show_data([name])
