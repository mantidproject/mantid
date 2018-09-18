from __future__ import (absolute_import, division, print_function)
from six import iteritems

from mantid import plots
from collections import OrderedDict

from qtpy import QtWidgets

from matplotlib.figure import Figure
from matplotlib import gridspec
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas

# pyplot should not be imported:
# https://stackoverflow.com/posts/comments/26295260

from Muon.GUI.ElementalAnalysis.Plotting.navigation_toolbar import myToolbar

from Muon.GUI.ElementalAnalysis.Plotting import plotting_utils as putils
from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_presenter import AxisChangerPresenter
from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_view import AxisChangerView

class PlotView(QtWidgets.QWidget):
    def __init__(self):
        super(PlotView, self).__init__()
        self.plots = OrderedDict({})
        self.errors_list = set()
        self.workspaces = {}
        self.workspace_plots = {}  # stores the plotted 'graphs' for deletion
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

        self.plot_selector = QtWidgets.QComboBox()
        self._update_plot_selector()
        self.plot_selector.currentIndexChanged[str].connect(self._set_bounds)

        button_layout = QtWidgets.QHBoxLayout()
        self.x_axis_changer = AxisChangerPresenter(AxisChangerView("X"))
        self.x_axis_changer.on_upper_bound_changed(self._update_x_axis_upper)
        self.x_axis_changer.on_lower_bound_changed(self._update_x_axis_lower)

        self.y_axis_changer = AxisChangerPresenter(AxisChangerView("Y"))
        self.y_axis_changer.on_upper_bound_changed(self._update_y_axis_upper)
        self.y_axis_changer.on_lower_bound_changed(self._update_y_axis_lower)

        self.errors = QtWidgets.QCheckBox("Errors")
        self.errors.stateChanged.connect(self._errors_changed)

        button_layout.addWidget(self.plot_selector)
        button_layout.addWidget(self.x_axis_changer.view)
        button_layout.addWidget(self.y_axis_changer.view)
        button_layout.addWidget(self.errors)

        grid = QtWidgets.QGridLayout()

        self.toolbar = myToolbar(self.canvas,self)
        self.toolbar.update()

        grid.addWidget(self.toolbar,0,0)
        grid.addWidget(self.canvas, 1, 0)
        grid.addLayout(button_layout, 2, 0)
        self.setLayout(grid)

    def setAddConnection(self,slot):
        self.toolbar.setAddConnection(slot)

    def setRmConnection(self,slot):
        self.toolbar.setRmConnection(slot)

    def _redo_layout(func):
        """
        Simple decorator (@_redo_layout) to call tight_layout() on plots
         and to redraw the canvas.
        (https://www.python.org/dev/peps/pep-0318/)
        """

        def wraps(self, *args, **kwargs):
            output = func(self, *args, **kwargs)
            if len(self.plots):
                self.figure.tight_layout()
            self.canvas.draw()
            return output
        return wraps

    def _silent_checkbox_check(self, state):
        """ Checks a checkbox without emitting a checked event. """
        self.errors.blockSignals(True)
        self.errors.setChecked(state)
        self.errors.blockSignals(False)

    def _set_plot_bounds(self, name, plot):
        """
        Sets AxisChanger bounds to the given plot bounds and updates
            the plot-specific error checkbox.
        """
        self.x_axis_changer.set_bounds(plot.get_xlim())
        self.y_axis_changer.set_bounds(plot.get_ylim())
        self._silent_checkbox_check(name in self.errors_list)

    def _set_bounds(self, new_plot):
        """
        Sets AxisChanger bounds if a new plot is added, or removes the AxisChanger
            fields if a plot is removed.
        """
        new_plot = str(new_plot)
        if new_plot and new_plot != "All":
            plot = self.get_subplot(new_plot)
            self._set_plot_bounds(new_plot, plot)
        elif not new_plot:
            self.x_axis_changer.clear_bounds()
            self.y_axis_changer.clear_bounds()

    def _get_current_plot_name(self):
        """ Returns the 'current' plot name based on the dropdown selector. """
        return str(self.plot_selector.currentText())

    def _get_current_plots(self):
        """
        Returns a list of the current plot, or all plots if 'All' is selected.
        """
        name = self._get_current_plot_name()
        return self.plots.values() if name == "All" else [
            self.get_subplot(name)]

    @_redo_layout
    def _update_x_axis(self, bound):
        """ Updates the plot's x limits with the specified bound. """
        try:
            for plot in self._get_current_plots():
                plot.set_xlim(**bound)
        except KeyError:
            return

    def _update_x_axis_lower(self, bound):
        """ Updates the lower x axis limit. """
        self._update_x_axis({"left": bound})

    def _update_x_axis_upper(self, bound):
        """ Updates the upper x axis limit. """
        self._update_x_axis({"right": bound})

    @_redo_layout
    def _update_y_axis(self, bound):
        """ Updates the plot's y limits with the specified bound. """
        try:
            for plot in self._get_current_plots():
                plot.set_ylim(**bound)
        except KeyError:
            return

    def _update_y_axis_lower(self, bound):
        """ Updates the lower y axis limit. """
        self._update_y_axis({"bottom": bound})

    def _update_y_axis_upper(self, bound):
        """ Updates the upper y axis limit. """
        self._update_y_axis({"top": bound})

    def _modify_errors_list(self, name, state):
        """
        Adds/Removes a plot name to the errors set depending on the 'state' bool.
        """
        if state:
            self.errors_list.add(name)
        else:
            try:
                self.errors_list.remove(name)
            except KeyError:
                return

    def _change_plot_errors(self, name, plot, state):
        """
        Removes the previous plot and redraws with/without errors depending on the state.
        """
        self._modify_errors_list(name, state)
        workspaces = self.workspaces[name]
        self.workspaces[name] = []
        # get the limits before replotting, so they appear unchanged.
        x, y = plot.get_xlim(), plot.get_ylim()
        for old_plot in self.workspace_plots[name]:
            old_plot.remove()
            del old_plot
        self.workspace_plots[name] = []
        for workspace in workspaces:
            self.plot(name, workspace)
        plot.set_xlim(x)
        plot.set_ylim(y)
        self._set_bounds(name)  # set AxisChanger bounds again.

    @_redo_layout
    def _errors_changed(self, state):
        """ Replots subplots with errors depending on the current selection. """
        current_name = self._get_current_plot_name()
        if current_name == "All":
            for name, plot in iteritems(self.plots):
                self._change_plot_errors(name, plot, state)
        else:
            self._change_plot_errors(
                current_name, self.get_subplot(current_name), state)

    def _set_positions(self, positions):
        """ Moves all subplots based on a gridspec change. """
        for plot, pos in zip(self.plots.values(), positions):
            grid_pos = self.current_grid[pos[0], pos[1]]
            plot.set_position(
                grid_pos.get_position(
                    self.figure))  # sets plot position, magic?
            # required because tight_layout() is used.
            plot.set_subplotspec(grid_pos)

    @_redo_layout
    def _update_gridspec(self, new_plots, last=None):
        """ Updates the gridspec; adds a 'last' subplot if one is supplied. """
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
        """ Updates plot selector (dropdown). """
        self.plot_selector.clear()
        self.plot_selector.addItem("All")
        self.plot_selector.addItems(list(self.plots.keys()))

    def _add_workspace_name(self, name, workspace):
        """ Adds a workspace to a plot's list of workspaces. """
        try:
            if workspace not in self.workspaces[name]:
                self.workspaces[name].append(workspace)
        except KeyError:
            self.workspaces[name] = [workspace]

    @_redo_layout
    def plot(self, name, workspace):
        """ Plots a workspace to a subplot (with errors, if necessary). """
        self._add_workspace_name(name, workspace)
        if name in self.errors_list:
            self.plot_workspace_errors(name, workspace)
        else:
            self.plot_workspace(name, workspace)
        self._set_bounds(name)

    def _add_plotted_line(self, name, lines):
        """ Appends plotted lines to the related subplot list. """
        try:
            self.workspace_plots[name].extend(lines)
        except KeyError:
            self.workspace_plots[name] = lines

    def plot_workspace_errors(self, name, workspace):
        """ Plots a workspace with errrors, and appends caps/bars to the subplot list. """
        subplot = self.get_subplot(name)
        line, cap_lines, bar_lines = plots.plotfunctions.errorbar(
            subplot, workspace, specNum=1)
        all_lines = [line]
        all_lines.extend(cap_lines)
        all_lines.extend(bar_lines)
        self._add_plotted_line(name, all_lines)

    def plot_workspace(self, name, workspace):
        """ Plots a workspace normally. """
        subplot = self.get_subplot(name)
        line, = plots.plotfunctions.plot(subplot, workspace, specNum=1)
        self._add_plotted_line(name, [line])

    def get_subplot(self, name):
        """ Returns the subplot corresponding to a given name """
        return self.plots[name]

    def get_subplots(self):
        """ Returns all subplots. """
        return self.plots

    def add_subplot(self, name):
        """ will raise KeyError if: plots exceed 4 """
        self._update_gridspec(len(self.plots) + 1, last=name)
        return self.get_subplot(name)

    def remove_subplot(self, name):
        """ will raise KeyError if: 'name' isn't a plot; there are no plots """
        self.figure.delaxes(self.get_subplot(name))
        del self.plots[name]
        del self.workspaces[name]
        self._update_gridspec(len(self.plots))

    @_redo_layout
    def add_moveable_vline(self, plot_name, x_value, y_minx, y_max, **kwargs):
        pass

    @_redo_layout
    def add_moveable_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        pass
