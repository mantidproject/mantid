# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List, NamedTuple
from qtpy import QtWidgets
from Muon.GUI.Common.plotting_widget.dockable_plot_toolbar import DockablePlotToolbar
from Muon.GUI.Common.plotting_widget.external_plotting_model import PlotInformation
from mantid import AnalysisDataService
from mantid.plots import MantidAxes
from mantid.plots.plotfunctions import get_plot_fig, create_subplots
from matplotlib.figure import Figure
import matplotlib.pyplot as plt
import numpy as np

from matplotlib.backends.qt_compat import is_pyqt5

if is_pyqt5():
    from matplotlib.backends.backend_qt5agg import FigureCanvas
else:
    from matplotlib.backends.backend_qt4agg import FigureCanvas

DEFAULT_X_LIMITS = [0, 15]


def _do_single_plot(ax, workspace, index, errors, plot_kwargs):
    plot_fn = ax.errorbar if errors else ax.plot
    plot_kwargs['wkspIndex'] = index
    plot_fn(workspace, **plot_kwargs)
    ax.legend(prop=dict(size=5))


class PlottingCanvasView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super().__init__(parent)

        # create the figure
        self.fig = Figure()
        self.fig.canvas = FigureCanvas(self.fig)
        self.toolBar = DockablePlotToolbar(self.fig.canvas, self)

        # Create a set of Mantid axis for the figure
        self.fig, axes = get_plot_fig(overplot=False, ax_properties=None, axes_num=1,
                                      fig=self.fig)
        self._number_of_axes = 1
        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.toolBar)
        layout.addWidget(self.fig.canvas)
        self.setLayout(layout)

        self.plotted_workspace_information = []  # type : List[PlotInformation}

    def clear_workspaces_from_plot(self):
        print(self.plotted_workspace_information)
        for plot_information in reversed(self.plotted_workspace_information):
            axis = self.fig.axes[plot_information.axis]
            workspace = plot_information.workspace
            axis.remove_workspace_artists(workspace)
            self.plotted_workspace_information.remove(plot_information)

        self.plotted_workspace_information = []  # type : List[PlotInformation}

    def add_workspaces_to_plot(self, workspace_plot_info: PlotInformation):

        workspace = workspace_plot_info.workspace
        errors = workspace_plot_info.errors
        ws_index = workspace_plot_info.specNum - 1
        axis_number = workspace_plot_info.axis
        ax = self.fig.axes[axis_number]
        self.plotted_workspace_information.append(workspace_plot_info)
        _do_single_plot(ax, workspace, ws_index, errors=errors,
                        plot_kwargs=self._get_plot_kwargs())

    def _remove_workspace_from_plot(self, workspace):
        for axis in self.fig.axes:
            axis.remove_workspace_artists(workspace)
            axis.legend(prop=dict(size=5))

    def redraw_figure(self):
        self.fig.canvas.toolbar.update()
        self.fig.tight_layout()
        self.fig.canvas.draw()

    def set_axis_limits(self, axis_number, xlims, ylims):
        pass

    def set_axes_limits(self, xlim, ylim):
        plt.setp(self.fig.axes, xlim=xlim, ylim=ylim)

    def _get_plot_kwargs(self):
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False}
        return plot_kwargs
