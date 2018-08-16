from collections import OrderedDict

from six import iteritems

from mantid import plots

from PyQt4 import QtGui

from collections import OrderedDict

from matplotlib.figure import Figure
from matplotlib import gridspec
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
# pyplot should not be imported:
# https://stackoverflow.com/posts/comments/26295260

from Muon.GUI.ElementalAnalysis.Plotting import plotting_utils as putils
from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_presenter import AxisChangerPresenter
from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_view import AxisChangerView

from random import choice, randint
from time import time


class PlotView(QtGui.QWidget):
    def __init__(self):
        super(PlotView, self).__init__()
        self.plots = OrderedDict({})
        self.workspaces = {}
        self.plot_additions = {}
        self.current_grid = None
        self.gridspecs = {
            1: gridspec.GridSpec(1, 1),
            2: gridspec.GridSpec(1, 2),
            3: gridspec.GridSpec(3, 1),
            4: gridspec.GridSpec(2, 2)
        }
        self.figure = Figure()
        self.figure.set_facecolor("none")
        self.canvas = FigureCanvas(self.figure)
        self.last_positions = []

        self.plot_selector = QtGui.QComboBox()
        self.plot_selector.currentIndexChanged[str].connect(self._set_bounds)

        button_layout = QtGui.QHBoxLayout()
        self.x_axis_changer = AxisChangerPresenter(AxisChangerView("X"))
        self.x_axis_changer.on_bounds_changed(self._update_x_axis)

        self.y_axis_changer = AxisChangerPresenter(AxisChangerView("Y"))
        self.y_axis_changer.on_bounds_changed(self._update_y_axis)

        self.errors = QtGui.QCheckBox("Errors")
        self.errors.stateChanged.connect(self._errors_changed)

        button_layout.addWidget(self.plot_selector)
        button_layout.addWidget(self.x_axis_changer.view)
        button_layout.addWidget(self.y_axis_changer.view)
        button_layout.addWidget(self.errors)

        grid = QtGui.QGridLayout()
        grid.addWidget(self.canvas, 0, 0)
        grid.addLayout(button_layout, 1, 0)
        self.setLayout(grid)

    def _redo_layout(func):
        """
        Simple decorator to call tight_layout() on plots, and to redraw the canvas.
        """

        def wraps(self, *args, **kwargs):
            func(self, *args, **kwargs)
            if len(self.plots):
                self.figure.tight_layout()
            self.canvas.draw()
        return wraps

    def _save_addition(func):
        """
        'Saves' the function call to be replayed later when the plots are cleared.
        """

        def wraps(self, name, *args, **kwargs):
            try:
                self.plot_additions[name].append((func, name, args, kwargs))
            except KeyError:
                self.plot_additions[name] = [(func, name, args, kwargs)]
            func(self, name, *args, **kwargs)
        return wraps

    def _set_bounds(self, new_plot):
        if new_plot:
            plot = self.plots[str(new_plot)]
            self.x_axis_changer.set_bounds(plot.get_xlim())
            self.y_axis_changer.set_bounds(plot.get_ylim())
        else:
            self.x_axis_changer.clear_bounds()
            self.y_axis_changer.clear_bounds()

    def _get_current_plot(self):
        return self.plots[str(self.plot_selector.currentText())]

    @_redo_layout
    def _update_x_axis(self, bounds):
        try:
            self._get_current_plot().set_xlim(bounds)
        except KeyError:
            return

    @_redo_layout
    def _update_y_axis(self, bounds):
        try:
            self._get_current_plot().set_ylim(bounds)
        except KeyError:
            return

    @_redo_layout
    def _errors_changed(self, state):
        for name, plot in iteritems(self.plots):
            workspaces = self.workspaces[name]
            self.workspaces[name] = []
            x, y = plot.get_xlim(), plot.get_ylim()
            plot.clear()
            for workspace in workspaces:
                self.plot(name, workspace)
            plot.set_xlim(x)
            plot.set_ylim(y)
            self._replay_additions(name)

    def _replay_additions(self, name):
        for func, name, args, kwargs in self.plot_additions[name]:
            func(self, name, *args, **kwargs)

    def _set_positions(self, positions):
        for plot, pos in zip(self.plots.values(), positions):
            grid_pos = self.current_grid[pos[0], pos[1]]
            plot.set_position(grid_pos.get_position(self.figure))
            plot.set_subplotspec(grid_pos)

    @_redo_layout
    def _update_gridspec(self, new_plots, last=None):
        if new_plots:
            self.current_grid = self.gridspecs[new_plots]
            positions = putils.get_layout(new_plots)
            self._set_positions(positions)
            if last is not None:
                # label is necessary to fix
                # https://github.com/matplotlib/matplotlib/issues/4786
                pos = self.current_grid[positions[-1][0], positions[-1][1]]
                self.plots[last] = self.figure.add_subplot(pos, label=last)
                self.plots[last].set_subplotspec(pos)
        self._update_plot_selector()

    def _update_plot_selector(self):
        self.plot_selector.clear()
        self.plot_selector.addItems(self.plots.keys())

    def _add_workspace_name(self, name, workspace):
        try:
            if workspace not in self.workspaces[name]:
                self.workspaces[name].append(workspace)
        except KeyError:
            self.workspaces[name] = [workspace]

    @_redo_layout
    def plot(self, name, workspace):
        self._add_workspace_name(name, workspace)
        if self.errors.isChecked():
            self.plot_workspace_errors(name, workspace)
        else:
            self.plot_workspace(name, workspace)
        self._set_bounds(name)

    def plot_workspace_errors(self, name, workspace):
        subplot = self.plots[name]
        plots.plotfunctions.errorbar(subplot, workspace, specNum=1)

    def plot_workspace(self, name, workspace):
        subplot = self.plots[name]
        plots.plotfunctions.plot(subplot, workspace, specNum=1)

    def get_subplot(self, name):
        return self.plots[name]

    def get_subplots(self):
        return self.plots

    def add_subplot(self, name):
        """ will raise KeyError if: plots exceed 4 """
        self._update_gridspec(len(self.plots) + 1, last=name)
        return self.plots[name]

    def remove_subplot(self, name):
        """ will raise KeyError if: 'name' isn't a plot; there are no plots """
        self.figure.delaxes(self.plots[name])
        del self.plots[name]
        del self.workspaces[name]
        del self.plot_additions[name]
        self._update_gridspec(len(self.plots))

    @_redo_layout
    @_save_addition
    def call_plot_method(self, name, func, *args, **kwargs):
        return func(*args, **kwargs)

    @_redo_layout
    @_save_addition
    def add_vline(self, plot_name, x_value, y_min, y_max, **kwargs):
        return self.plots[plot_name].axvline(x_value, y_min, y_max, **kwargs)

    @_redo_layout
    @_save_addition
    def add_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        return self.plots[plot_name].axhline(y_value, x_min, x_max, **kwargs)

    @_redo_layout
    @_save_addition
    def add_moveable_vline(self, plot_name, x_value, y_minx, y_max, **kwargs):
        pass

    @_redo_layout
    @_save_addition
    def add_moveable_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        pass
