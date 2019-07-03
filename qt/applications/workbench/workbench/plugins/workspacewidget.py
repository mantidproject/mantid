# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
#    This file is part of the mantid workbench.
# SPDX - License - Identifier: GPL - 3.0 +
#
#
from __future__ import (absolute_import, unicode_literals)

import matplotlib.pyplot
from functools import partial
from qtpy.QtWidgets import QMessageBox, QVBoxLayout

from mantid.api import AnalysisDataService, WorkspaceGroup
from mantid.kernel import logger
from mantidqt.plotting.functions import can_overplot, pcolormesh, plot, plot_from_names
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter
from mantidqt.widgets.samplelogs.presenter import SampleLogs
from mantidqt.widgets.sliceviewer.presenter import SliceViewer
from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay
from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay
from mantidqt.widgets.workspacewidget.algorithmhistorywindow import AlgorithmHistoryWindow
from mantidqt.widgets.workspacewidget.workspacetreewidget import WorkspaceTreeWidget
from workbench.plugins.base import PluginWidget


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
        self.workspacewidget.sliceViewerClicked.connect(self._do_slice_viewer)
        self.workspacewidget.showDataClicked.connect(self._do_show_data)
        self.workspacewidget.showInstrumentClicked.connect(self._do_show_instrument)
        self.workspacewidget.showAlgorithmHistoryClicked.connect(self._do_show_algorithm_history)

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
            try:
                SampleLogs(ws=ws, parent=self)
            except Exception as exception:
                logger.warning("Could not open sample logs for workspace '{}'."
                               "".format(ws.name()))
                logger.debug("{}: {}".format(type(exception).__name__,
                                             exception))

    def _do_slice_viewer(self, names):
        """
        Show the sliceviewer window for the given workspaces

        :param names: A list of workspace names
        """
        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            try:
                SliceViewer(ws=ws, parent=self)
            except Exception as exception:
                logger.warning("Could not open slice viewer for workspace '{}'."
                               "".format(ws.name()))
                logger.debug("{}: {}".format(type(exception).__name__,
                                             exception))

    def _do_show_instrument(self, names):
        """
        Show an instrument widget for the given workspaces

        :param names: A list of workspace names
        """
        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            if ws.getInstrument().getName():
                try:
                    presenter = InstrumentViewPresenter(ws, parent=self)
                    presenter.show_view()
                except Exception as exception:
                    logger.warning("Could not show instrument for workspace "
                                   "'{}':\n{}.\n".format(ws.name(), exception))
            else:
                logger.warning("Could not show instrument for workspace '{}':"
                               "\nNo instrument available.\n"
                               "".format(ws.name()))

    def _do_show_data(self, names):
        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            try:
                MatrixWorkspaceDisplay.supports(ws)
                # the plot function is being injected in the presenter
                # this is done so that the plotting library is mockable in testing
                presenter = MatrixWorkspaceDisplay(ws, plot=plot, parent=self)
                presenter.show_view()
            except ValueError:
                try:
                    TableWorkspaceDisplay.supports(ws)
                    presenter = TableWorkspaceDisplay(ws, plot=matplotlib.pyplot, parent=self)
                    presenter.show_view()
                except ValueError:
                    logger.error(
                        "Could not open workspace: {0} with neither "
                        "MatrixWorkspaceDisplay nor TableWorkspaceDisplay."
                        "".format(ws.name()))

    def _do_show_algorithm_history(self, names):
        for name in names:
            if not isinstance(self._ads.retrieve(name), WorkspaceGroup):
                try:
                    AlgorithmHistoryWindow(self, name).show()
                except Exception as exception:
                    logger.warning("Could not open history of '{}'. "
                                   "".format(name))
                    logger.warning("{}: {}".format(type(exception).__name__, exception))

    def _action_double_click_workspace(self, name):
        plot_from_names([name], errors=False, overplot=False, show_colorfill_btn=True)

    def refresh_workspaces(self):
        self.workspacewidget.refreshWorkspaces()
